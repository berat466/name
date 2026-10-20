"""Mudahale adimlari. Hicbiri cihaza dokunmaz: yalnizca onerilen komutu uretir (dry-run)."""


def suggest_block(alert):
    ip = alert.get("key")
    if not ip or ip == "config":
        return None
    return (f"! {alert['rule_id']}: {alert['name']}\n"
            f"access-list BLOCK_LIST extended deny ip host {ip} any\n"
            f"! Uygulamadan once kaynagi dogrula (yanlis pozitif riski).")


def run_actions(alert):
    """Uyarinin eylem listesini yurutur, uretilen ciktilari dondurur."""
    outputs = []
    for action in alert.get("actions", ["alert"]):
        if action == "suggest_block":
            cmd = suggest_block(alert)
            if cmd:
                outputs.append(cmd)
    return outputs
