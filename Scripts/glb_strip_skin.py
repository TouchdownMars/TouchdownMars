# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Write a copy of a GLB without skins (joints, weights, animations), so Unreal imports it as a static mesh.
Some NASA 3D Resources files carry an armature that Interchange turns into a skeletal mesh.
    python Scripts/glb_strip_skin.py <in.glb> <out.glb>
"""

import json
import struct
import sys


def main():
    src, dst = sys.argv[1], sys.argv[2]
    data = open(src, "rb").read()
    magic, version, _ = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise SystemExit("not a GLB")
    offset, chunks = 12, []
    while offset < len(data):
        length, kind = struct.unpack_from("<II", data, offset)
        chunks.append([kind, data[offset + 8:offset + 8 + length]])
        offset += 8 + length
    doc = json.loads(chunks[0][1].decode("utf-8"))
    doc.pop("skins", None)
    doc.pop("animations", None)
    for node in doc.get("nodes", []):
        node.pop("skin", None)
    for mesh in doc.get("meshes", []):
        for prim in mesh.get("primitives", []):
            for key in [k for k in prim.get("attributes", {}) if k.startswith(("JOINTS_", "WEIGHTS_"))]:
                del prim["attributes"][key]
    text = json.dumps(doc, separators=(",", ":")).encode("utf-8")
    text += b" " * ((4 - len(text) % 4) % 4)
    chunks[0][1] = text
    body = b"".join(struct.pack("<II", len(c), k) + c for k, c in chunks)
    open(dst, "wb").write(struct.pack("<III", magic, version, 12 + len(body)) + body)
    print(f"{dst}: skins removed")


main()
