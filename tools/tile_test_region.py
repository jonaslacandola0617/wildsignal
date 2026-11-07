"""REGION-001 step 2: a bigger streaming test region made from the test village.

Usage: python -I tools/tile_test_region.py ASSETS_DIR HOUSES_DIR [REGION_ID] [--no-ring]

Lays 3 x 3 copies of regions/hearthgrove_test side by side (one copy every
59.7 m, three ground tiles), about 180 m across, cut into 32 m chunks. The
middle copy keeps the characters. Each outer copy swaps its five houses for
other approved houses from HOUSES_DIR, so walking around really loads and frees
models. Heights are resampled onto a 0.25 m lattice. This is an engineering
test layout, not a level: copies repeat, and swapped houses keep the old
house's walls.

Writes ASSETS_DIR/regions/REGION_ID/ and ASSETS_DIR/pack_REGION_ID.json, and
copies the extra house files into ASSETS_DIR. Private data: keep out of git.
"""
import glob
import json
import math
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # edge_ring.py beside this tool
import edge_ring  # noqa: E402

CHUNK = 32.0
PERIOD = 59.7  # three 19.9 m ground tiles
STEP = 0.25
EXTRA_HOUSES = ["WS_HOUSE_%03d" % n for n in (2, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 17, 18)]


def main():
    args = [a for a in sys.argv[1:] if a != "--no-ring"]
    assets, houses = Path(args[0]), Path(args[1])
    region_id = args[2] if len(args) > 2 else "streaming_test"
    src = assets / "regions" / "hearthgrove_test"
    village = json.loads((src / "region.json").read_text())
    rows, solids, grass = [], [], []
    hx = village["height"]["half_extent"]
    hn = village["height"]["samples"]
    heights = [0.0] * (hn * hn)
    for entry in village["chunks"]:
        if entry.get("ring"):  # the village's own edge ring is not tiled
            continue
        c = json.loads((src / entry["file"]).read_text())
        rows += [r for r in c["instances"] if not r["id"].startswith("ring_")]
        solids += c["solids"]
        grass += c["grass"]
        if "height" in c:
            b = (src / "chunks" / c["height"]).read_bytes()
            i0, j0, ni, nj = struct.unpack("<4I", b[4:20])
            vals = struct.unpack("<%df" % (ni * nj), b[20:])
            for j in range(nj):
                for i in range(ni):
                    heights[(j0 + j) * hn + i0 + i] = vals[j * ni + i]

    def village_height(x, z):  # the game's bilinear lookup over the village grid
        u = min(max((x + hx) / (2 * hx), 0), 1) * (hn - 1)
        v = min(max((z + hx) / (2 * hx), 0), 1) * (hn - 1)
        i, j = min(int(u), hn - 2), min(int(v), hn - 2)
        a, b = u - i, v - j
        h = heights
        return (1 - b) * ((1 - a) * h[j * hn + i] + a * h[j * hn + i + 1]) + \
            b * ((1 - a) * h[(j + 1) * hn + i] + a * h[(j + 1) * hn + i + 1])

    half = 1.5 * PERIOD
    ox = oz = -math.ceil(half / CHUNK) * CHUNK
    cols = rows_n = int(round(-2 * ox / CHUNK))

    def cell(x, z):
        return int(math.floor((x - ox) / CHUNK)), int(math.floor((z - oz) / CHUNK))

    chunks = {(cx, cz): {"instances": [], "solids": [], "grass": []}
              for cz in range(rows_n) for cx in range(cols)}
    copies = [(i, j) for j in (-1, 0, 1) for i in (-1, 0, 1)]
    used_houses = set()
    for k, (ci, cj) in enumerate(copies):
        dx, dz = ci * PERIOD, cj * PERIOD
        outer = (ci, cj) != (0, 0)
        swap = {}
        if outer:
            originals = sorted({r["model"] for r in rows if r["model"].startswith("WS_HOUSE")})
            for n, m in enumerate(originals):
                swap[m] = EXTRA_HOUSES[(k * len(originals) + n) % len(EXTRA_HOUSES)]
        for r in rows:
            r2 = dict(r)
            r2["position"] = [r["position"][0] + dx, r["position"][1], r["position"][2] + dz]
            r2["model"] = swap.get(r["model"], r["model"])
            if outer:
                r2["id"] = "%s@%d_%d" % (r["id"], ci, cj)
            r2["order"] = k * 100000 + r["order"]
            used_houses.add(r2["model"])
            chunks[cell(r2["position"][0], r2["position"][2])]["instances"].append(r2)
        for s in solids:
            x, z, w, d = s["box"]
            chunks[cell(x + dx, z + dz)]["solids"].append(
                {"box": [x + dx, z + dz, w, d], "house": s["house"], "order": k * 100000 + s["order"]})
        for g in grass:
            g2 = dict(g)
            g2["position"] = [g["position"][0] + dx, g["position"][1], g["position"][2] + dz]
            g2["seed"] = k * 100000 + g["seed"]
            chunks[cell(g2["position"][0], g2["position"][2])]["grass"].append(g2)

    # Height lattice over the whole region; each sample is owned by one chunk.
    n = int(math.ceil(2 * half / STEP)) + 1
    lo = -half

    def wrapped(x):
        return min(max(x - PERIOD * round(x / PERIOD), -hx), hx)

    owned = {}
    for j in range(n):
        for i in range(n):
            key = cell(lo + i * STEP, lo + j * STEP)
            a = owned.get(key, (i, j, i, j))
            owned[key] = (min(a[0], i), min(a[1], j), max(a[2], i), max(a[3], j))

    out = assets / "regions" / region_id
    if out.exists():
        shutil.rmtree(out)
    (out / "chunks").mkdir(parents=True)
    listing = []
    for (cx, cz), c in sorted(chunks.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        name = "c_%d_%d" % (cx, cz)
        chunk = {"revision": 1, "x": cx, "z": cz, **c}
        if (cx, cz) in owned:
            i0, j0, i1, j1 = owned[(cx, cz)]
            ni, nj = i1 - i0 + 1, j1 - j0 + 1
            vals = [village_height(wrapped(lo + (i0 + i) * STEP), wrapped(lo + (j0 + j) * STEP))
                    for j in range(nj) for i in range(ni)]
            (out / "chunks" / (name + ".height.bin")).write_bytes(
                b"WSH1" + struct.pack("<4I", i0, j0, ni, nj) + struct.pack("<%df" % len(vals), *vals))
            chunk["height"] = name + ".height.bin"
        (out / "chunks" / (name + ".json")).write_text(json.dumps(chunk, separators=(",", ":")))
        listing.append({"x": cx, "z": cz, "file": "chunks/" + name + ".json"})

    # The region's model list: the village pack plus the extra approved houses.
    pack = json.loads((assets / "pack_style.json").read_text())
    have = {m["id"] for m in pack["models"]}
    for h in EXTRA_HOUSES:
        if h in used_houses and h not in have:
            for f in (h + ".glb", h + "_atlas.png"):
                if not (assets / f).exists():  # HOUSES_DIR may be the assets folder itself
                    shutil.copy(houses / f, assets / f)
            pack["models"].append({"id": h, "path": h + ".glb"})
    pack_name = "pack_%s.json" % region_id
    (assets / pack_name).write_text(json.dumps(pack, indent=1))

    walk = half - 0.2
    region = {k: village[k] for k in ("ground_spacing", "terrain_instance_y_scale", "spawn", "wild",
                                      "rest_spots", "review_cameras", "exclude_materials", "persistent")}
    region.update({
        "revision": 1, "id": region_id, "name": "Streaming test (3 x 3 village copies)",
        "chunk_size": CHUNK, "grid": {"origin": [ox, oz], "columns": cols, "rows": rows_n},
        "height": {"origin": [lo, lo], "step": STEP, "count": [n, n]},
        "walk": {"min": [-walk, -walk], "max": [walk, walk]},
        "shared": {"packs": {"default": pack_name, "source": pack_name, "style": pack_name},
                   "vegetation": "vegetation.json"},
        "shadow_follow": True,
        "meadow": {"world_min": [-PERIOD / 2, -PERIOD / 2], "world_size": [PERIOD, PERIOD], "repeat": True},
        "chunks": listing,
    })
    (out / "region.json").write_text(json.dumps(region, indent=1))
    models = {r["model"] for c in chunks.values() for r in c["instances"]} | {r["model"] for r in village["persistent"]}
    print("region", region_id, "grid", cols, "x", rows_n, "objects",
          sum(len(c["instances"]) for c in chunks.values()), "grass",
          sum(len(c["grass"]) for c in chunks.values()), "models", len(models), "pack", len(pack["models"]))
    if "--no-ring" not in sys.argv:
        edge_ring.add_ring(assets, region_id)


if __name__ == "__main__":
    main()
