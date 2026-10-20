import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

import run as cli  # noqa: E402
from engine.detect import run_rules  # noqa: E402
from engine.logparse import parse_line, parse_lines  # noqa: E402
from engine.rules import load_rules, selftest_all  # noqa: E402
from engine.schema import RuleError, validate_rule  # noqa: E402
from extractor import draft_rules  # noqa: E402


class ParseTests(unittest.TestCase):
    def test_ssh_fail(self):
        ev = parse_line("Oct 20 10:15:01 h sshd[1]: Failed password for invalid user a from 1.2.3.4 port 5 ssh2", 2026)
        self.assertEqual((ev["kind"], ev["src_ip"], ev["user"]), ("ssh_fail", "1.2.3.4", "a"))

    def test_unknown_line(self):
        self.assertIsNone(parse_line("Oct 20 10:15:01 h cron[1]: job ok", 2026))

    def test_asa_106100(self):
        ev = parse_line("Oct 20 10:15:01 fw : %ASA-4-106100: access-list OUT denied tcp outside/9.9.9.9(1) -> inside/10.0.0.1(443) hit-cnt 1 first hit", 2026)
        self.assertEqual((ev["kind"], ev["dst_port"]), ("asa_deny", 443))


class RuleTests(unittest.TestCase):
    def setUp(self):
        self.rules = load_rules(cli.APPROVED)

    def test_all_embedded_tests_pass(self):
        self.assertEqual(selftest_all(self.rules), [])

    def test_schema_rejects_rule_without_tests(self):
        bad = dict(self.rules[0]); bad.pop("tests")
        with self.assertRaises(RuleError):
            validate_rule(bad)

    def test_end_to_end_logs(self):
        lines = (
            [f"Oct 20 10:15:{i:02d} h sshd[1]: Failed password for root from 8.8.8.8 port 1 ssh2" for i in range(6)]
            + ["Oct 20 10:15:30 h sshd[1]: Accepted password for root from 8.8.8.8 port 1 ssh2"]
            + [f'Oct 20 10:16:{i:02d} fw : %ASA-4-106023: Deny tcp src outside:7.7.7.7/1 dst inside:10.0.0.5/{100+i} by access-group "X"'
               for i in range(12)])
        alerts = run_rules([r for r in self.rules if r["type"] != "config"], events=parse_lines(lines, 2026))
        ids = {a["rule_id"] for a in alerts}
        self.assertEqual(ids, {"R001", "R003", "R004"})

    def test_config_audit(self):
        text = "hostname R1\nip http server\nsnmp-server community public RO\nline vty 0 4\n transport input telnet\n"
        with tempfile.NamedTemporaryFile("w", suffix=".cfg", delete=False) as f:
            f.write(text)
        ids = {a["rule_id"] for a in cli.audit_config(f.name)}
        self.assertTrue({"C001", "C003", "C004", "C002", "C005", "C006"} <= ids)


class ExtractorTests(unittest.TestCase):
    GOOD = json.dumps({
        "id": "X001", "name": "test", "source": "s", "attack": "T1110", "severity": "high",
        "type": "threshold", "where": {"kind": "ssh_fail"}, "group_by": "src_ip", "count": 3, "window_s": 60,
        "tests": [
            {"lines": [f"Oct 20 10:15:0{i} h sshd[1]: Failed password for a from 1.1.1.1 port 1 ssh2" for i in range(3)], "expect_alert": True},
            {"lines": ["Oct 20 10:15:00 h sshd[1]: Failed password for a from 1.1.1.1 port 1 ssh2"], "expect_alert": False}]})

    def test_good_draft_accepted(self):
        status, rule = draft_rules.draft("metin", "kaynak", generate=lambda p: self.GOOD)
        self.assertEqual(status, "ok")

    def test_wrong_test_expectation_rejected(self):
        bad = json.loads(self.GOOD); bad["tests"][0]["expect_alert"] = False
        status, _ = draft_rules.draft("metin", "kaynak", generate=lambda p: json.dumps(bad))
        self.assertEqual(status, "rejected")

    def test_garbage_rejected(self):
        status, _ = draft_rules.draft("metin", "kaynak", generate=lambda p: "sacma")
        self.assertEqual(status, "rejected")


if __name__ == "__main__":
    unittest.main()
