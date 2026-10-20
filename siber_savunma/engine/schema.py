"""Kural bicimi dogrulayicisi. Cisco metninden uretilen her kural buradan gecmek zorunda."""

RULE_TYPES = {"threshold", "distinct", "sequence", "config"}
SEVERITIES = {"low", "medium", "high", "critical"}
ACTIONS = {"alert", "suggest_block"}


class RuleError(ValueError):
    pass


def _need(rule, key, typ):
    if key not in rule:
        raise RuleError(f"'{key}' eksik")
    if not isinstance(rule[key], typ):
        raise RuleError(f"'{key}' tipi hatali")


def validate_rule(rule):
    """Gecerliyse kurali dondurur, degilse RuleError firlatir."""
    if not isinstance(rule, dict):
        raise RuleError("kural bir sozluk olmali")
    for key in ("id", "name", "source", "attack", "severity"):
        _need(rule, key, str)
    if rule["severity"] not in SEVERITIES:
        raise RuleError("severity gecersiz")
    _need(rule, "type", str)
    if rule["type"] not in RULE_TYPES:
        raise RuleError("type gecersiz")
    actions = rule.get("actions", ["alert"])
    if not isinstance(actions, list) or not set(actions) <= ACTIONS:
        raise RuleError("actions gecersiz")
    t = rule["type"]
    if t in ("threshold", "distinct"):
        _need(rule, "where", dict)
        _need(rule, "group_by", str)
        _need(rule, "count", int)
        _need(rule, "window_s", int)
        if t == "distinct":
            _need(rule, "distinct_field", str)
    elif t == "sequence":
        for key in ("first", "then"):
            _need(rule, key, dict)
        for key in ("group_by",):
            _need(rule, key, str)
        for key in ("first_count", "window_s"):
            _need(rule, key, int)
    elif t == "config":
        if "present_regex" not in rule and "absent_regex" not in rule:
            raise RuleError("config kurali present_regex veya absent_regex ister")
    tests = rule.get("tests")
    if not isinstance(tests, list) or not tests:
        raise RuleError("en az bir gomulu test sart")
    for tc in tests:
        if not isinstance(tc, dict) or "lines" not in tc or "expect_alert" not in tc:
            raise RuleError("test {lines, expect_alert} icermeli")
    return rule
