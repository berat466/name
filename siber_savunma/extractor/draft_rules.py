"""Cisco metninden kural TASLAGI uretir (yerel acik agirlikli model, Ollama ile).

Akis: metin parcasi -> model -> JSON kural -> sema dogrulama -> gomulu testler.
Gecenler rules/pending/ altina yazilir; insan onayi olmadan approved/ altina GIRMEZ.
Model kurali yanlis yazabilir; bu yuzden gomulu test zorunlu ve onay insana birakildi.
"""
import json
import re
import urllib.request
from pathlib import Path

from engine.rules import selftest_rule
from engine.schema import RuleError, validate_rule

ROOT = Path(__file__).resolve().parent.parent
PENDING = ROOT / "rules" / "pending"

PROMPT = """Asagidaki savunma metninden, bir log analiz motorunun calistirabilecegi TEK bir kural uret.
SADECE gecerli JSON dondur. Biçim:
{{"id":"X001","name":"...","source":"{source}","attack":"T####","severity":"low|medium|high|critical",
 "type":"threshold|distinct|sequence|config", ...tipe ozel alanlar...,
 "actions":["alert"], "tests":[{{"lines":["..."],"expect_alert":true}},{{"lines":["..."],"expect_alert":false}}]}}
Kullanilabilir olay turleri (kind): ssh_fail, ssh_success, asa_deny, ios_login_fail.
Tipler: threshold(where,group_by,count,window_s), distinct(+distinct_field), sequence(first,first_count,then,window_s,group_by),
config(present_regex veya absent_regex). Testler hem alarm veren hem vermeyen ornek icermeli.
Metin yetersizse {{"skip":"neden"}} dondur.

METIN:
{text}
"""


def ollama_generate(prompt, model="qwen2.5:7b", url="http://localhost:11434/api/generate"):
    body = json.dumps({"model": model, "prompt": prompt, "stream": False, "format": "json"}).encode()
    req = urllib.request.Request(url, body, {"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=300) as r:
        return json.loads(r.read())["response"]


def draft(text, source, generate=ollama_generate, year=2026):
    """(durum, rule_veya_neden) dondurur. durum: 'ok' | 'skip' | 'rejected'."""
    raw = generate(PROMPT.format(text=text, source=source))
    m = re.search(r"\{.*\}", raw, re.DOTALL)
    if not m:
        return "rejected", "model JSON dondurmedi"
    try:
        rule = json.loads(m.group(0))
    except json.JSONDecodeError as e:
        return "rejected", f"JSON hatasi: {e}"
    if "skip" in rule:
        return "skip", rule["skip"]
    try:
        validate_rule(rule)
    except RuleError as e:
        return "rejected", f"sema: {e}"
    errors = selftest_rule(rule, year)
    if errors:
        return "rejected", "; ".join(errors)
    return "ok", rule


def draft_file(path, source=None, generate=ollama_generate, chunk_chars=1800):
    """Bir metin dosyasini parcalayip her parca icin taslak uretir; gecenleri pending/ altina yazar."""
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    source = source or Path(path).name
    chunks = [text[i:i + chunk_chars] for i in range(0, len(text), chunk_chars)]
    PENDING.mkdir(parents=True, exist_ok=True)
    report = {"ok": 0, "skip": 0, "rejected": []}
    for n, chunk in enumerate(chunks):
        status, result = draft(chunk, f"{source} parca {n}", generate)
        if status == "ok":
            (PENDING / f"{result['id']}.json").write_text(
                json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
            report["ok"] += 1
        elif status == "skip":
            report["skip"] += 1
        else:
            report["rejected"].append((n, result))
    return report


if __name__ == "__main__":
    import sys
    print(json.dumps(draft_file(sys.argv[1]), ensure_ascii=False, indent=2))
