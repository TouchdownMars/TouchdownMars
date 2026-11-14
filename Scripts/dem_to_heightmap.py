# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Cut a 1 km window from the Mars Trek Jezero HiRISE DEM and write a 16-bit PNG heightmap for UE.

Source: NASA Mars Trek, "MRO HiRISE, Jezero DEM, Controlled"
  label JEZ_hirise_soc_006_DTM_MOLAtopography_DeltaGeoid_1m_Eqc_latTs0_lon0_blend40 (1 m/px, equirectangular, Mars 2000 sphere)
  download https://trek.nasa.gov/mars/TrekWS/rest/cat/data/stream?label=<label>
Needs: numpy, tifffile, imagecodecs, zarr (pip). Run outside Unreal:
  python Scripts/dem_to_heightmap.py <dem.tif> <out_dir> [lat] [lon]
Default centre: the Perseverance landing site (18.4447 N, 77.4508 E).
Output: TD_Jezero_1009.png (16-bit, 1009x1009, 1 m/px) and TD_Jezero_1009.json (z scale, heights).
A pad around the base is levelled (real outposts sit on prepared ground); this is a designer change and is
written into the JSON. Everything else is the measured terrain.
"""

import json
import math
import os
import sys
import zlib
import struct

import numpy as np
import tifffile
import zarr

SIZE = 1009                 # UE landscape resolution (8x8 components, 63 quads, 2 sections)
MARS_RADIUS_M = 3396190.0   # Mars 2000 sphere (from the product's projection)
WEST, NORTH = 77.2229331, 18.6693150   # product bbox (Mars Trek)
PAD_CENTRE_M = (15.0, -5.0)  # pad centre relative to the window centre, metres (x = east, y = south)
PAD_RADIUS_M, PAD_FALLOFF_M = 30.0, 25.0


def write_png16(path, array):
    height, width = array.shape
    raw = b"".join(b"\x00" + array[row].astype(">u2").tobytes() for row in range(height))
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 16, 0, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    dem_path, out_dir = sys.argv[1], sys.argv[2]
    lat = float(sys.argv[3]) if len(sys.argv) > 3 else 18.4447
    lon = float(sys.argv[4]) if len(sys.argv) > 4 else 77.4508
    metres_per_degree = math.pi / 180.0 * MARS_RADIUS_M
    col = int(round((lon - WEST) * metres_per_degree))
    row = int(round((NORTH - lat) * metres_per_degree))
    half = SIZE // 2

    with tifffile.TiffFile(dem_path) as tif:
        page = tif.pages[0]
        print("tiff", page.shape, page.dtype, "compression", page.compression, "tiled", page.is_tiled)
        store = page.aszarr()
        data = zarr.open(store, mode="r")
        window = np.asarray(data[row - half:row + half + 1, col - half:col + half + 1], dtype=np.float64)
        store.close()

    nodata = (window < -1e4) | ~np.isfinite(window)
    if nodata.mean() > 0.01:
        raise SystemExit(f"window has {nodata.mean():.1%} no-data; choose another centre")
    window[nodata] = np.nanmedian(window[~nodata])
    raw_min, raw_max = float(window.min()), float(window.max())

    # Level the base pad to its median height with a smooth falloff.
    ys, xs = np.mgrid[0:SIZE, 0:SIZE]
    px = half + PAD_CENTRE_M[0]
    py = half + PAD_CENTRE_M[1]
    dist = np.hypot(xs - px, ys - py)
    pad_height = float(np.median(window[dist <= PAD_RADIUS_M]))
    blend = np.clip((dist - PAD_RADIUS_M) / PAD_FALLOFF_M, 0.0, 1.0)
    blend = blend * blend * (3 - 2 * blend)
    terrain = window * blend + pad_height * (1 - blend)

    low, high = float(terrain.min()), float(terrain.max())
    span = max(high - low, 1.0)
    margin = span * 0.05
    low, high = low - margin, high + margin
    heightmap = np.round((terrain - low) / (high - low) * 65535).astype(np.uint16)

    os.makedirs(out_dir, exist_ok=True)
    png = os.path.join(out_dir, f"TD_Jezero_{SIZE}.png")
    write_png16(png, heightmap)
    # UE: world Z (cm) = (value - 32768) * ZScale / 128 + LocationZ. Full 16-bit range = 512 * ZScale cm.
    z_scale = (high - low) * 100.0 / 512.0
    pad_value = (pad_height - low) / (high - low) * 65535
    location_z = -(pad_value - 32768) * z_scale / 128.0   # puts the pad at world Z = 0
    info = {
        "source_label": "JEZ_hirise_soc_006_DTM_MOLAtopography_DeltaGeoid_1m_Eqc_latTs0_lon0_blend40",
        "centre_lat": lat, "centre_lon": lon, "pixel_row": row, "pixel_col": col, "size": SIZE, "metres_per_pixel": 1.0,
        "raw_min_m": raw_min, "raw_max_m": raw_max, "pad_height_m": pad_height, "relief_m": raw_max - raw_min,
        "pad": {"centre_m": PAD_CENTRE_M, "radius_m": PAD_RADIUS_M, "falloff_m": PAD_FALLOFF_M, "note": "levelled base pad, designer change"},
        "ue_scale": [100.0, 100.0, z_scale], "ue_location": [-(SIZE - 1) * 50.0, -(SIZE - 1) * 50.0, location_z],
    }
    with open(os.path.join(out_dir, f"TD_Jezero_{SIZE}.json"), "w") as f:
        json.dump(info, f, indent=2)
    print(json.dumps(info, indent=2))


main()
