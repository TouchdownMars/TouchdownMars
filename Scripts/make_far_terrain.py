# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Far terrain around the outpost from the same Mars Trek Jezero DEM as TD_Terrain (dem_to_heightmap.py).

The near landscape covers 1 km at 1 m/px. This one covers about 15 km at 15 m/px, so the horizon shows the
measured crater floor instead of an empty edge. Each output pixel is the mean of the DEM pixels under it.
The planet's curvature is applied (drop = d^2 / 2R, Mars 2000 sphere radius from the product), so the horizon
sits where it really is for a standing astronaut. Inside the near landscape the far terrain is pushed 4 m down
(easing to 0 at its edge) so it never shows through. Heights use the same datum as TD_Terrain: the base pad is world Z = 0.
Needs numpy, scipy and tifffile (pip). Run outside Unreal:
  python Scripts/make_far_terrain.py <dem.tif> Data/Terrain
Output: TD_Jezero_Far_1009.png (16-bit) and TD_Jezero_Far_1009.json (UE location and scale).
"""

import json
import math
import os
import sys
import struct
import zlib

import warnings

import numpy as np
import tifffile
from scipy import ndimage

SIZE = 1009
STEP_M = 15                 # metres per output pixel
NEAR_HALF_M = 504.0         # the near landscape spans +-504 m
NEAR_FADE_M = 30.0
NEAR_SINK_M = 4.0
MARS_RADIUS_M = 3396190.0


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
    near = json.load(open(os.path.join(out_dir, "TD_Jezero_1009.json"), encoding="utf-8"))
    row0, col0, pad_height = near["pixel_row"], near["pixel_col"], near["pad_height_m"]
    half = SIZE // 2
    dem = tifffile.memmap(dem_path)
    span = half * STEP_M
    r_lo, c_lo = row0 - span - STEP_M // 2, col0 - span - STEP_M // 2
    if r_lo < 0 or c_lo < 0 or r_lo + SIZE * STEP_M > dem.shape[0] or c_lo + SIZE * STEP_M > dem.shape[1]:
        raise SystemExit("window leaves the DEM; lower STEP_M")
    block = np.asarray(dem[r_lo:r_lo + SIZE * STEP_M, c_lo:c_lo + SIZE * STEP_M], dtype=np.float32)
    block[(block < -1e4) | ~np.isfinite(block)] = np.nan
    cells = block.reshape(SIZE, STEP_M, SIZE, STEP_M)
    with np.errstate(all="ignore"), warnings.catch_warnings():
        warnings.simplefilter("ignore")
        heights = np.nanmean(cells, axis=(1, 3)).astype(np.float64)
    missing = np.isnan(heights)
    print("no-data share", float(missing.mean()))
    if missing.any():
        # Fill the DEM's no-data corners from the nearest measured pixel, then soften only the filled part.
        _, (iy, ix) = ndimage.distance_transform_edt(missing, return_indices=True)
        filled = heights[iy, ix]
        soft = ndimage.gaussian_filter(filled, 12)
        heights = np.where(missing, soft, heights)

    ys, xs = np.mgrid[0:SIZE, 0:SIZE]
    dx, dy = (xs - half) * STEP_M, (ys - half) * STEP_M
    terrain = heights - pad_height - (dx * dx + dy * dy) / (2.0 * MARS_RADIUS_M)
    # Under the near landscape: full sink inside, easing to none at its edge so the two meet without a step.
    edge = np.maximum(np.abs(dx), np.abs(dy))
    terrain -= NEAR_SINK_M * np.clip((NEAR_HALF_M - edge) / NEAR_FADE_M, 0.0, 1.0)

    low, high = float(terrain.min()), float(terrain.max())
    margin = (high - low) * 0.05
    low, high = low - margin, high + margin
    heightmap = np.round((terrain - low) / (high - low) * 65535).astype(np.uint16)
    write_png16(os.path.join(out_dir, f"TD_Jezero_Far_{SIZE}.png"), heightmap)
    # UE: world Z (cm) = (value - 32768) * ZScale / 128 + LocationZ; value 0 is `low` metres.
    z_scale = (high - low) * 100.0 / 512.0
    location_z = low * 100.0 + 256.0 * z_scale
    info = {
        "source_label": near["source_label"], "centre_row": row0, "centre_col": col0, "size": SIZE,
        "metres_per_pixel": STEP_M, "height_min_m": float(terrain.min()), "height_max_m": float(terrain.max()),
        "curvature": "drop d^2/2R applied, R = 3396190 m (Mars 2000 sphere)", "no_data_share": float(missing.mean()),
        "near_sink_m": NEAR_SINK_M,
        "ue_scale": [STEP_M * 100.0, STEP_M * 100.0, z_scale],
        "ue_location": [-half * STEP_M * 100.0, -half * STEP_M * 100.0, location_z],
    }
    json.dump(info, open(os.path.join(out_dir, f"TD_Jezero_Far_{SIZE}.json"), "w"), indent=2)
    print(json.dumps(info, indent=2))


main()
