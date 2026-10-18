import struct, sys

path = r"D:\expander\Expander-Dev\Expander\x64\Release\Expander.res"
data = open(path, "rb").read()
pos = 0
idx = 0
while pos + 32 <= len(data):
    data_size, hdr_size = struct.unpack_from("<II", data, pos)
    # type/name: 2-byte marker + 2-byte ordinal per WORD pair
    t_marker, t_ord = struct.unpack_from("<HH", data, pos + 8)
    n_marker, n_ord = struct.unpack_from("<HH", data, pos + 12)
    dv = struct.unpack_from("<I", data, pos + 16)[0]
    mf, lang = struct.unpack_from("<HH", data, pos + 20)
    ver = struct.unpack_from("<I", data, pos + 24)[0]
    ch  = struct.unpack_from("<I", data, pos + 28)[0]
    if hdr_size == 0 or data_size > len(data):
        print(f"entry@{pos}: malformed/end (hdr={hdr_size} data={data_size}), stop")
        break
    payload = data[pos + hdr_size : pos + hdr_size + data_size]
    type_desc = f"type={t_ord}" if t_marker == 0xFFFF else f"type=str({payload[:20]!r})"
    name_desc = f"name={n_ord}" if n_marker == 0xFFFF else "name=str"
    print(f"entry@{pos}: dataSize={data_size} hdrSize={hdr_size} {type_desc} {name_desc} memFlags=0x{mf:04X} lang={lang} dv={dv} ver={ver} ch={ch}")
    if t_marker == 0xFFFF and t_ord == 10:
        print("   RCDATA name=%d bytes: %s" % (n_ord, payload.hex(" ")))
    pos = (pos + hdr_size + data_size + 3) & ~3
    idx += 1
    if idx > 60: break
print("total entries shown:", idx)
