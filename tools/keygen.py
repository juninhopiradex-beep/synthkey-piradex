#!/usr/bin/env python3
"""
SYNTHKEY by Piradex — gerador de chaves de packs na linha de comandos (alternativa ao separador ADMIN).

Exemplos:
  python3 tools/keygen.py --pack trap --perm
  python3 tools/keygen.py --pack trap rnb --until 2026-12-31 --qty 5 --seats 2
  python3 tools/keygen.py --pack kuduro --days 30            # 30 dias depois da ativação
O segredo é lido de .synthkey_secret_hex (criado por gen_secrets.py) ou da variável SYNTHKEY_SECRET_HEX.
"""
import argparse, csv, datetime, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import skkeys

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pack", nargs="+", required=True, choices=sorted(skkeys.PREFIX))
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--perm", action="store_true"); g.add_argument("--until"); g.add_argument("--days", type=int)
    ap.add_argument("--seats", type=int, default=1); ap.add_argument("--qty", type=int, default=1)
    ap.add_argument("--serial", type=int, default=int(datetime.datetime.now().strftime("%m%d%H%M")) & 0xFFFFFF)
    ap.add_argument("--note", default="")
    a = ap.parse_args()
    hexs = os.environ.get("SYNTHKEY_SECRET_HEX") or open(os.path.join(os.path.dirname(__file__), "..", ".synthkey_secret_hex")).read().strip()
    secret = bytes.fromhex(hexs)
    w = csv.writer(sys.stdout); w.writerow(["pack", "chave", "validade", "computadores", "serie", "nota"])
    serial = a.serial
    for _ in range(a.qty):
        for p in a.pack:
            if a.perm: k = skkeys.make_key(secret, p, False, True, 0, a.seats, serial); v = "permanente"
            elif a.until:
                d = datetime.date.fromisoformat(a.until); k = skkeys.make_key(secret, p, False, False, skkeys.days_since_2026(d), a.seats, serial); v = "até " + d.strftime("%d/%m/%Y")
            else: k = skkeys.make_key(secret, p, True, False, a.days, a.seats, serial); v = f"{a.days} dias após ativação"
            w.writerow([p, k, v, a.seats, serial, a.note]); serial += 1

if __name__ == "__main__":
    main()
