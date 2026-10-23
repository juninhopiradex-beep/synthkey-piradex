"""SYNTHKEY by Piradex — chaves de packs (espelho em Python de Source/Core/Keys.h)."""
import hashlib, hmac, datetime

ALPHA = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
PREFIX = {"trap": "TRAP", "rnb": "RNB", "afrobeat": "AFRO", "amapiano": "AMAP", "drill": "DRILL", "kizomba": "KIZ", "kuduro": "KUDU"}
EPOCH = datetime.date(2026, 1, 1)


def days_since_2026(d: datetime.date) -> int:
    return (d - EPOCH).days


def pack_bits(relative: bool, permanent: bool, expiry_day: int, seats: int, serial: int) -> bytes:
    days = 0 if (permanent and not relative) else (expiry_day & 0x1FFF)
    v = ((1 if relative else 0) << 39) | (days << 26) | (((seats - 1) & 3) << 24) | (serial & 0xFFFFFF)
    return v.to_bytes(5, "big")


def mac_for(secret: bytes, pack: str, data: bytes) -> bytes:
    return hmac.new(secret, b"synthkey-key|" + pack.encode() + b"|" + data, hashlib.sha256).digest()[:5]


def make_key(secret: bytes, pack: str, relative=False, permanent=True, expiry_day=0, seats=1, serial=0) -> str:
    data = pack_bits(relative, permanent, expiry_day, seats, serial)
    allbits = int.from_bytes(data + mac_for(secret, pack, data), "big")
    chars = "".join(ALPHA[(allbits >> (75 - 5 * i)) & 31] for i in range(16))
    return PREFIX.get(pack, "PACK") + "".join("-" + chars[g * 4:g * 4 + 4] for g in range(4))
