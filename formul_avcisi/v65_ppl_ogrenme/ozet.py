import re, sys, os
def parse(f):
    t = open(f).read(); d = {}
    for tag in ("TEST", "VAL", "EGITIM", "TEST_Q_yok", "TEST_baglam_karistirilmis", "TEST_C_slotlari"):
        m = re.search(r"V65\|%s[^|]*\|n=(\d+)\|CE=([\d.]+)\|PPL=([\d.]+)\|Top1=([\d.]+)\|Top5=([\d.]+)\|bin[^:]*: n=([\d,]+) CE=([\d.,]+) Top1=([\d.,]+)" % tag, t)
        if m: d[tag] = dict(n=int(m.group(1)), ce=float(m.group(2)), ppl=float(m.group(3)), t1=float(m.group(4)), t5=float(m.group(5)), bn=[int(x) for x in m.group(6).split(",")], bce=[float(x) for x in m.group(7).split(",")], bt1=[float(x) for x in m.group(8).split(",")])
    m = re.search(r"UNIGRAM_TEST\|CE=([\d.]+)\|PPL=([\d.]+)\|test hedef <unk> orani=([\d.]+)", t); d["uni"] = (float(m.group(1)), float(m.group(2)), float(m.group(3))) if m else None
    m = re.search(r"OOV\|test baglamda <unk> orani=([\d.]+)", t); d["oov"] = float(m.group(1)) if m else None
    m = re.search(r"MODEL_HAZIR\|([\d.]+) s.*tepe RSS=(\d+) MB", t); d["hazir"] = float(m.group(1)) if m else None; d["rss"] = int(m.group(2)) if m else None
    m = re.search(r"SURE\|degerlendirme toplam.*=([\d.]+) s", t); d["evals"] = float(m.group(1)) if m else None
    m = re.search(r"V65_ESDEGER\|test: maks\|dz\|\(ortuk-acik\)=([\d.e+-]+) CE\(ortuk\)=([\d.]+) CE\(acik\)=([\d.]+) Top1-uyum=([\d.]+)", t); d["esd"] = m.groups() if m else None
    m = re.search(r"rezidu=([\d.e+-]+)", t); d["rez"] = m.group(1) if m else None
    for k in ("g9 U=ACF'Y0", "f0 hazirlik", "f2 V51", "g45", "q1 Q_havuz", "g11 W_C", "e1a", "e1b", "v65 test akis"):
        m = re.search(r"TH\|%s[^|]*\|([\d.]+)" % re.escape(k), t); d["TH_" + k.split()[0]] = float(m.group(1)) if m else None
    tm = f[:-4] + ".time"
    if os.path.exists(tm):
        m = re.search(r"MAXRSS_KB (\d+)", open(tm).read()); d["maxrss"] = int(m.group(1)) / 1e6 if m else None
    return d
for m in ("A", "B"):
    for v in (2001, 5001, 10001):
        f = f"ham/{m}{v}.txt"
        if not os.path.exists(f): continue
        d = parse(f); te = d.get("TEST", {}); 
        nz = lambda r: (sum(r["bn"][i] * r["bt1"][i] for i in (1, 2, 3)) / sum(r["bn"][i] for i in (1, 2, 3)), sum(r["bn"][i] * r["bce"][i] for i in (1, 2, 3)) / sum(r["bn"][i] for i in (1, 2, 3))) if r else (None, None)
        print(f"== {m}{v}: hazir={d['hazir']} RSS={d['rss']}MB maxrss={d.get('maxrss')}GB eval={d['evals']}s rezidu={d['rez']} esdeger={d['esd']}")
        for k in ("EGITIM", "VAL", "TEST", "TEST_Q_yok", "TEST_baglam_karistirilmis", "TEST_C_slotlari"):
            r = d.get(k)
            if r: t1n, cen = nz(r); print(f"   {k:28s} CE={r['ce']:.4f} PPL={r['ppl']:.2f} Top1={r['t1']:.4f} Top5={r['t5']:.4f} | unk-disi Top1={t1n:.4f} CE={cen:.3f} | bin Top1(unk,sik,orta,nadir)={r['bt1']} CE={r['bce']} n={r['bn']}")
        print("   unigram", d["uni"], "baglam-OOV", d["oov"], {k: v for k, v in d.items() if k.startswith("TH_")})
