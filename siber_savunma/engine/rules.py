"""Kurallari yukler ve gomulu testlerinden gecirir."""
import json
from pathlib import Path

from .detect import run_rule
from .logparse import parse_lines
from .schema import RuleError, validate_rule


def load_rules(directory):
    rules = []
    for path in sorted(Path(directory).glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for rule in (data if isinstance(data, list) else [data]):
            rules.append(validate_rule(rule))
    return rules


def selftest_rule(rule, year=2026):
    """Gomulu testleri calistirir; hata listesi dondurur (bos liste = gecti)."""
    errors = []
    for i, tc in enumerate(rule["tests"]):
        if rule["type"] == "config":
            alerts = run_rule(rule, config_text="\n".join(tc["lines"]))
        else:
            alerts = run_rule(rule, events=parse_lines(tc["lines"], year))
        if bool(alerts) != bool(tc["expect_alert"]):
            errors.append(f"{rule['id']} test#{i}: beklenen alert={tc['expect_alert']}, gelen={len(alerts)}")
    return errors


def selftest_all(rules, year=2026):
    errors = []
    for rule in rules:
        try:
            validate_rule(rule)
        except RuleError as e:
            errors.append(f"{rule.get('id', '?')}: sema hatasi: {e}")
            continue
        errors.extend(selftest_rule(rule, year))
    return errors
