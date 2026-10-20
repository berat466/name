"""Kural motoru: olay akisi ve yapilandirma metni uzerinde kurallari calistirir."""
import re
from collections import defaultdict, deque
from datetime import timedelta


def _match(ev, where):
    return all(ev.get(k) == v for k, v in where.items())


def _alert(rule, key, ts, detail):
    return {"rule_id": rule["id"], "name": rule["name"], "severity": rule["severity"],
            "attack": rule["attack"], "source": rule["source"], "key": key,
            "ts": ts, "detail": detail, "actions": rule.get("actions", ["alert"])}


def _threshold(rule, events):
    win = timedelta(seconds=rule["window_s"])
    buckets = defaultdict(deque)
    fired = set()
    out = []
    for ev in events:
        if not _match(ev, rule["where"]):
            continue
        key = ev.get(rule["group_by"])
        if key is None:
            continue
        q = buckets[key]
        q.append(ev["ts"])
        while q and ev["ts"] - q[0] > win:
            q.popleft()
        if len(q) >= rule["count"] and key not in fired:
            fired.add(key)
            out.append(_alert(rule, key, ev["ts"],
                              f"{len(q)} olay / {rule['window_s']} sn ({rule['group_by']}={key})"))
    return out


def _distinct(rule, events):
    win = timedelta(seconds=rule["window_s"])
    field = rule["distinct_field"]
    buckets = defaultdict(deque)
    fired = set()
    out = []
    for ev in events:
        if not _match(ev, rule["where"]):
            continue
        key = ev.get(rule["group_by"])
        val = ev.get(field)
        if key is None or val is None:
            continue
        q = buckets[key]
        q.append((ev["ts"], val))
        while q and ev["ts"] - q[0][0] > win:
            q.popleft()
        distinct = {v for _, v in q}
        if len(distinct) >= rule["count"] and key not in fired:
            fired.add(key)
            out.append(_alert(rule, key, ev["ts"],
                              f"{len(distinct)} farkli {field} / {rule['window_s']} sn ({rule['group_by']}={key})"))
    return out


def _sequence(rule, events):
    win = timedelta(seconds=rule["window_s"])
    fails = defaultdict(deque)
    fired = set()
    out = []
    for ev in events:
        key = ev.get(rule["group_by"])
        if key is None:
            continue
        if _match(ev, rule["first"]):
            q = fails[key]
            q.append(ev["ts"])
            while q and ev["ts"] - q[0] > win:
                q.popleft()
        elif _match(ev, rule["then"]):
            q = fails[key]
            while q and ev["ts"] - q[0] > win:
                q.popleft()
            if len(q) >= rule["first_count"] and key not in fired:
                fired.add(key)
                out.append(_alert(rule, key, ev["ts"],
                                  f"{len(q)} basarisiz denemeden sonra basarili giris ({key})"))
    return out


def _config(rule, text):
    present = rule.get("present_regex")
    absent = rule.get("absent_regex")
    if present and re.search(present, text, re.MULTILINE):
        return [_alert(rule, "config", None, f"istenmeyen ayar bulundu: /{present}/")]
    if absent and not re.search(absent, text, re.MULTILINE):
        return [_alert(rule, "config", None, f"gerekli ayar yok: /{absent}/")]
    return []


def run_rule(rule, events=None, config_text=None):
    t = rule["type"]
    if t == "threshold":
        return _threshold(rule, events or [])
    if t == "distinct":
        return _distinct(rule, events or [])
    if t == "sequence":
        return _sequence(rule, events or [])
    if t == "config":
        return _config(rule, config_text or "")
    return []


def run_rules(rules, events=None, config_text=None):
    alerts = []
    for rule in rules:
        alerts.extend(run_rule(rule, events, config_text))
    return alerts
