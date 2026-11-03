"""REGION-001: split today's single-file test village into region + chunk files.

Usage: python -I tools/village_to_region.py ASSETS_DIR [REGION_ID]

Reads game.json, scene.json, vegetation.json (+ its grass files) and
terrain_height.bin from ASSETS_DIR and writes ASSETS_DIR/regions/REGION_ID/:
  region.json                 grid, shared assets, spawn, rest spots, cameras
  chunks/c_X_Z.json           instances, colliders, grass of one 32 m chunk
  chunks/c_X_Z.height.bin     the height samples that chunk owns
Nothing is moved or edited; the original files stay beside it. The output is
derived from private art data, so it belongs in the private asset folder.
"""
import json
import math
import struct
import sys
from pathlib import Path

CHUNK = 32.0


def main():
    assets = Path(sys.argv[1])
    region_id = sys.argv[2] if len(sys.argv) > 2 else "hearthgrove_test"
    game = json.loads((assets / "game.json").read_text())
    scene = json.loads((assets / "scene.json").read_text())
    vegetation = json.loads((assets / "vegetation.json").read_text())

    raw = (assets / "terrain_height.bin").read_bytes()
    samples = int(round(math.sqrt(len(raw) // 4)))
    if samples != 257 or samples * samples * 4 != len(raw):
        raise SystemExit("expected a 257 x 257 height grid")
    heights = struct.unpack("<%df" % (samples * samples), raw)
    half = float(game["world_half_extent"])
    step = 2 * half / (samples - 1)

    grass = []
    for name in vegetation["instance_files"]:
        for row in json.loads((assets / name).read_text()):
            row = dict(row)
            row["seed"] = len(grass)  # keeps today's grass detail bands exactly
            grass.append(row)

    xs = [r["position"][0] for r in scene["instances"]] + [r["position"][0] for r in grass] + [-half, half]
    zs = [r["position"][2] for r in scene["instances"]] + [r["position"][2] for r in grass] + [-half, half]
    ox, oz = math.floor(min(xs) / CHUNK) * CHUNK, math.floor(min(zs) / CHUNK) * CHUNK
    cols = int(math.floor((max(xs) - ox) / CHUNK)) + 1
    rows = int(math.floor((max(zs) - oz) / CHUNK)) + 1

    def cell(x, z):
        return int(math.floor((x - ox) / CHUNK)), int(math.floor((z - oz) / CHUNK))

    chunks = {(cx, cz): {"instances": [], "solids": [], "grass": []}
              for cz in range(rows) for cx in range(cols)}
    # "order" is a fixed draw order: objects that touch at the same depth
    # always draw the same way, whichever chunk loads first.
    for order, r in enumerate(scene["instances"]):
        chunks[cell(r["position"][0], r["position"][2])]["instances"].append({**r, "order": order})
    houses = set(game["house_solids"])
    for index, (x, z, hx, hz) in enumerate(game["solids"]):
        chunks[cell(x, z)]["solids"].append({"box": [x, z, hx, hz], "house": index in houses, "order": index})
    for r in grass:
        chunks[cell(r["position"][0], r["position"][2])]["grass"].append(r)

    # Each height sample belongs to the chunk its position falls in. Chunk
    # squares are axis aligned, so a chunk owns a rectangle of samples.
    owned = {}
    for j in range(samples):
        for i in range(samples):
            key = cell(-half + i * step, -half + j * step)
            lo_i, lo_j, hi_i, hi_j = owned.get(key, (i, j, i, j))
            owned[key] = (min(lo_i, i), min(lo_j, j), max(hi_i, i), max(hi_j, j))

    out = assets / "regions" / region_id
    (out / "chunks").mkdir(parents=True, exist_ok=True)
    listing = []
    for (cx, cz), c in sorted(chunks.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        name = "c_%d_%d" % (cx, cz)
        chunk = {"revision": 1, "x": cx, "z": cz, **c}
        if (cx, cz) in owned:
            i0, j0, i1, j1 = owned[(cx, cz)]
            ni, nj = i1 - i0 + 1, j1 - j0 + 1
            values = [heights[(j0 + j) * samples + i0 + i] for j in range(nj) for i in range(ni)]
            (out / "chunks" / (name + ".height.bin")).write_bytes(
                b"WSH1" + struct.pack("<4I", i0, j0, ni, nj) + struct.pack("<%df" % len(values), *values))
            chunk["height"] = name + ".height.bin"
        (out / "chunks" / (name + ".json")).write_text(json.dumps(chunk, separators=(",", ":")))
        listing.append({"x": cx, "z": cz, "file": "chunks/" + name + ".json"})

    region = {
        "revision": 1,
        "id": region_id,
        "name": scene.get("name", region_id),
        "chunk_size": CHUNK,
        "grid": {"origin": [ox, oz], "columns": cols, "rows": rows},
        "walk_half_extent": half,
        "height": {"half_extent": half, "samples": samples},
        "shared": {
            "packs": {"default": "pack.json", "source": "pack_source.json", "style": "pack_style.json"},
            "vegetation": "vegetation.json",
        },
        "ground_spacing": game["ground_spacing"],
        "terrain_instance_y_scale": game.get("terrain_instance_y_scale", 1),
        "spawn": game["spawn"],
        "wild": game["wild"],
        "rest_spots": game["rest_spots"],
        "review_cameras": game["review_cameras"],
        "exclude_materials": game["exclude_materials"],
        "chunks": listing,
    }
    (out / "region.json").write_text(json.dumps(region, indent=1))
    counts = {k: sum(len(c[k]) for c in chunks.values()) for k in ("instances", "solids", "grass")}
    print("region", region_id, "grid", cols, "x", rows, "origin", ox, oz, counts)


if __name__ == "__main__":
    main()
