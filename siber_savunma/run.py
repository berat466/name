#!/usr/bin/env python3
"""Savunma motoru komut satiri.

  python run.py selftest                       # tum kurallarin gomulu testleri
  python run.py logs  LOG_DOSYASI [--year Y]   # log analizi + onerilen mudahale
  python run.py audit CISCO_CONFIG_DOSYASI     # Cisco IOS sertlestirme denetimi
  python run.py report LOG_DOSYASI [CONFIG]    # HTML rapor (report.html)
"""
import argparse
import html
import sys
from pathlib import Path

from engine.detect import run_rules
from engine.logparse import parse_lines
from engine.playbook import run_actions
from engine.rules import load_rules, selftest_all

ROOT = Path(__file__).parent
APPROVED = ROOT / "rules" / "approved"


def _load():
    return load_rules(APPROVED)


def _split(rules):
    cfg = [r for r in rules if r["type"] == "config"]
    log = [r for r in rules if r["type"] != "config"]
    return log, cfg


def analyze_logs(path, year=None):
    lines = Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
    events = parse_lines(lines, year)
    log_rules, _ = _split(_load())
    return events, run_rules(log_rules, events=events)


def audit_config(path):
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    _, cfg_rules = _split(_load())
    return run_rules(cfg_rules, config_text=text)


def _print(alerts):
    order = {"critical": 0, "high": 1, "medium": 2, "low": 3}
    for a in sorted(alerts, key=lambda a: order[a["severity"]]):
        print(f"[{a['severity'].upper():8}] {a['rule_id']} {a['name']}  ({a['attack']})")
        print(f"           {a['detail']}")
        for out in run_actions(a):
            print("           " + out.replace("\n", "\n           "))


def make_report(alerts, out="report.html"):
    rows = "".join(
        f"<tr><td>{html.escape(a['severity'])}</td><td>{html.escape(a['rule_id'])}</td>"
        f"<td>{html.escape(a['name'])}</td><td>{html.escape(a['attack'])}</td>"
        f"<td>{html.escape(a['detail'])}</td></tr>" for a in alerts)
    Path(out).write_text(
        "<meta charset=utf-8><title>Savunma Raporu</title><h1>Savunma Raporu</h1>"
        "<table border=1 cellpadding=6><tr><th>Seviye</th><th>Kural</th><th>Ad</th>"
        f"<th>ATT&amp;CK</th><th>Detay</th></tr>{rows}</table>", encoding="utf-8")
    return out


def main(argv=None):
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("selftest")
    s = sub.add_parser("logs"); s.add_argument("path"); s.add_argument("--year", type=int)
    s = sub.add_parser("audit"); s.add_argument("path")
    s = sub.add_parser("report"); s.add_argument("logs"); s.add_argument("config", nargs="?")
    a = p.parse_args(argv)

    if a.cmd == "selftest":
        rules = _load()
        errors = selftest_all(rules)
        if errors:
            print("\n".join(errors)); return 1
        print(f"{len(rules)} kural, tum gomulu testler gecti")
        return 0
    if a.cmd == "logs":
        events, alerts = analyze_logs(a.path, a.year)
        print(f"{len(events)} olay okundu, {len(alerts)} uyari")
        _print(alerts); return 0
    if a.cmd == "audit":
        alerts = audit_config(a.path)
        print(f"{len(alerts)} bulgu"); _print(alerts); return 0
    if a.cmd == "report":
        _, alerts = analyze_logs(a.logs)
        if a.config:
            alerts += audit_config(a.config)
        print("rapor:", make_report(alerts)); return 0


if __name__ == "__main__":
    sys.exit(main())
