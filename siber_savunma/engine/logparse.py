"""Log satirlarini ortak olay (event) bicimine cevirir.

Desteklenen kaynaklar: Linux sshd (auth.log), Cisco ASA (%ASA-...), Cisco IOS (%SEC_LOGIN...).
Olay: {ts, kind, src_ip, dst_ip, dst_port, user, raw}
"""
import re
from datetime import datetime

_SYSLOG_TS = re.compile(r"^\*?([A-Z][a-z]{2})\s+(\d{1,2})\s+(\d{2}:\d{2}:\d{2})")

_SSH_FAIL = re.compile(
    r"sshd\[\d+\]: Failed (?:password|publickey) for (?:invalid user )?(?P<user>\S+) "
    r"from (?P<src>\d{1,3}(?:\.\d{1,3}){3}) port (?P<port>\d+)"
)
_SSH_OK = re.compile(
    r"sshd\[\d+\]: Accepted \S+ for (?P<user>\S+) from (?P<src>\d{1,3}(?:\.\d{1,3}){3}) port (?P<port>\d+)"
)
_ASA_106023 = re.compile(
    r"%ASA-\d-106023: Deny (?P<proto>\w+) src \S+?:(?P<src>[\d.]+)/(?P<sport>\d+) "
    r"dst \S+?:(?P<dst>[\d.]+)/(?P<dport>\d+)"
)
_ASA_106100 = re.compile(
    r"%ASA-\d-106100: access-list \S+ denied (?P<proto>\w+) \S+?/(?P<src>[\d.]+)\((?P<sport>\d+)\) "
    r"-> \S+?/(?P<dst>[\d.]+)\((?P<dport>\d+)\)"
)
_IOS_LOGIN_FAIL = re.compile(
    r"%SEC_LOGIN-\d-LOGIN_FAILED: Login failed \[user: (?P<user>[^\]]*)\] \[Source: (?P<src>[\d.]+)\]"
)

_MONTHS = {m: i for i, m in enumerate(
    ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"], 1)}


def _ts(line, year):
    m = _SYSLOG_TS.match(line)
    if not m:
        return None
    mon, day, hms = m.groups()
    try:
        return datetime.strptime(f"{year} {_MONTHS[mon]} {day} {hms}", "%Y %m %d %H:%M:%S")
    except (KeyError, ValueError):
        return None  # bozuk/gecersiz zaman damgasi: satiri atla


def parse_line(line, year=None):
    """Tek satiri olaya cevirir; taninmazsa None doner."""
    year = year or datetime.now().year
    ts = _ts(line, year)
    if ts is None:
        return None
    ev = {"ts": ts, "kind": None, "src_ip": None, "dst_ip": None,
          "dst_port": None, "user": None, "raw": line.rstrip("\n")}
    for rx, kind in ((_SSH_FAIL, "ssh_fail"), (_SSH_OK, "ssh_success")):
        m = rx.search(line)
        if m:
            ev.update(kind=kind, src_ip=m["src"], user=m["user"])
            return ev
    for rx in (_ASA_106023, _ASA_106100):
        m = rx.search(line)
        if m:
            ev.update(kind="asa_deny", src_ip=m["src"], dst_ip=m["dst"], dst_port=int(m["dport"]))
            return ev
    m = _IOS_LOGIN_FAIL.search(line)
    if m:
        ev.update(kind="ios_login_fail", src_ip=m["src"], user=m["user"])
        return ev
    return None


def parse_lines(lines, year=None):
    events = []
    for line in lines:
        ev = parse_line(line, year)
        if ev:
            events.append(ev)
    events.sort(key=lambda e: e["ts"])
    return events
