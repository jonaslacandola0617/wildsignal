"""REGION-001 build 019: close a region's map edge with a ring of scenery chunks.

Usage: python -I tools/edge_ring.py ASSETS_DIR REGION_ID [DEPTH]

The region tools (village_to_region.py, tile_test_region.py,
height_test_region.py) call add_ring() on their output, so this command is
only needed for a region made some other way.

Adds DEPTH (default 1) rows of 32 m chunks all around the region's grid. The
old chunks keep their contents and move up DEPTH places in the grid (the grid
origin moves out by the same amount, so nothing moves in the world). The ring
chunks hold:
  - ground: more of the region's own ground tiles (EA_GROUND), laid exactly as
    the region lays them (same turn and spacing, so their seams meet without
    cracks), or, in a region whose ground is drawn from heights ("terrain"),
    more height samples that continue the edge and rise into a gentle wooded
    bank. A tile belongs to the chunk its anchor corner stands in, so a tile
    just outside the old edge can belong to an old edge chunk;
  - a forest band: trees (WSP_TREE_*, WSP_TREE_FAR*), a sparse border of bushes
    (WSP_BUSH_0) and a few rocks (EA_ROCK_*), copied from the region's own rows
    for their sizes.
Only models the region already uses are placed, so no new art is needed.
Every row it adds has an id starting "ring_", so tools that copy a region's
own rows (tile_test_region.py, height_test_region.py) can leave them out.

The walk bounds do not change: the ring is scenery outside them, so nobody can
walk into it and it needs no colliders. The forest band is as deep as the flat
camera can see past the walk edge (about 21 m with the tallest trees), and the
ground goes on to the end of the ring.

Private data: the output is derived from private art data, so keep it out of git.
"""
import json
import math
import random
import shutil
import struct
import sys
from pathlib import Path

GROUND = "EA_GROUND"
TILE = 20.0  # EA_GROUND mesh: local x -20..0, z 0..20 around its origin
TREES_NEAR = ("WSP_TREE_0", "WSP_TREE_1", "WSP_TREE_2", "WSP_TREE_3")
TREES_FAR = ("WSP_TREE_FAR", "WSP_TREE_FAR1")
BUSHES = ("WSP_BUSH_0",)
ROCKS = tuple("EA_ROCK_%d" % n for n in range(6))
# Distances past the walk edge, in metres.
TREE_BAND = (1.2, 24.0)
FAR_FROM = 11.0  # far trees (cheaper) from here out, when the region has them
BUSH_BAND = (0.4, 6.0)  # a thin border only: from above, bushes read as dark discs
ROCK_BAND = (0.8, 9.0)
TREE_GAP, BUSH_GAP, ROCK_GAP = 2.3, 1.1, 1.1  # half the spacing between trunks/bushes/rocks
# Terrain regions: the ground beyond the old lattice rises into a soft bank.
BANK_HEIGHT, BANK_FROM, BANK_TO = 2.2, 2.0, 16.0


def smoothstep(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def rotate(yaw_deg, x, z):
    """Local x/z to world offset, as the engine's transform() does."""
    a = math.radians(yaw_deg)
    c, s = math.cos(a), math.sin(a)
    return x * c + z * s, -x * s + z * c


class TileMesh:
    """Surface height of the EA_GROUND tile mesh, for standing objects on it."""

    def __init__(self, gltf_path):
        g = json.loads(gltf_path.read_text())
        buf = (gltf_path.parent / g["buffers"][0]["uri"]).read_bytes()
        prim = g["meshes"][0]["primitives"][0]

        def read(index, comps):
            acc = g["accessors"][index]
            view = g["bufferViews"][acc["bufferView"]]
            fmt = {5126: "f", 5123: "H", 5125: "I"}[acc["componentType"]]
            size = struct.calcsize(fmt)
            stride = view.get("byteStride", size * comps)
            base = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
            return [struct.unpack_from("<%d%s" % (comps, fmt), buf, base + k * stride)
                    for k in range(acc["count"])]

        self.pos = read(prim["attributes"]["POSITION"], 3)
        idx = [i[0] for i in read(prim["indices"], 1)]
        self.tris = [tuple(idx[k:k + 3]) for k in range(0, len(idx), 3)]
        self.cell = 2.0
        self.grid = {}
        for t in self.tris:
            xs = [self.pos[v][0] for v in t]
            zs = [self.pos[v][2] for v in t]
            for gi in range(int(math.floor(min(xs) / self.cell)), int(math.floor(max(xs) / self.cell)) + 1):
                for gj in range(int(math.floor(min(zs) / self.cell)), int(math.floor(max(zs) / self.cell)) + 1):
                    self.grid.setdefault((gi, gj), []).append(t)

    def height(self, x, z):
        best = None
        for t in self.grid.get((int(math.floor(x / self.cell)), int(math.floor(z / self.cell))), ()):
            (ax, ay, az), (bx, by, bz), (cx, cy, cz) = (self.pos[v] for v in t)
            d = (bz - cz) * (ax - cx) + (cx - bx) * (az - cz)
            if abs(d) < 1e-12:
                continue
            l1 = ((bz - cz) * (x - cx) + (cx - bx) * (z - cz)) / d
            l2 = ((cz - az) * (x - cx) + (ax - cx) * (z - cz)) / d
            l3 = 1 - l1 - l2
            if min(l1, l2, l3) >= -1e-6:
                y = l1 * ay + l2 * by + l3 * cy
                best = y if best is None else max(best, y)
        return best


def owners_per_axis(blocks, n_old, offset, n_new, origin_new, step, grid_origin, chunk, count, depth, axis):
    """Which grid column (axis 0) or row (axis 1) owns each sample of the new lattice."""
    old = [None] * n_old
    for (cx, cz), (i0, j0, ni, nj) in blocks.items():
        c, lo, n = (cx, i0, ni) if axis == 0 else (cz, j0, nj)
        for k in range(lo, lo + n):
            if old[k] is not None and old[k] != c:
                raise SystemExit("edge ring: height blocks are not split along grid lines")
            old[k] = c
    if None in old:
        raise SystemExit("edge ring: height lattice not fully owned")
    out = []
    for k in range(n_new):
        if offset <= k < offset + n_old:
            out.append(old[k - offset] + depth)
        else:
            w = origin_new + k * step
            out.append(min(count - 1, max(0, int(math.floor((w - grid_origin) / chunk)))))
    if any(b < a for a, b in zip(out, out[1:])):
        raise SystemExit("edge ring: height ownership not in grid order")
    return out


def add_ring(assets, region_id, depth=1, seed=19):
    assets = Path(assets)
    out = assets / "regions" / region_id
    region = json.loads((out / "region.json").read_text())
    if "edge_ring" in region:
        raise SystemExit("edge ring: %s already has one" % region_id)
    C = float(region["chunk_size"])
    ox, oz = region["grid"]["origin"]
    cols, rows = region["grid"]["columns"], region["grid"]["rows"]
    legacy = "half_extent" in region["height"]
    if legacy:
        w = float(region["walk_half_extent"])
        walk = (-w, -w, w, w)
    else:
        walk = (*region["walk"]["min"], *region["walk"]["max"])
    terrain = "terrain" in region
    D = depth
    nox, noz, ncols, nrows = ox - D * C, oz - D * C, cols + 2 * D, rows + 2 * D

    chunks = {}
    for e in region["chunks"]:
        c = json.loads((out / e["file"]).read_text())
        blk = None
        if "height" in c:
            b = (out / "chunks" / c["height"]).read_bytes()
            i0, j0, ni, nj = struct.unpack("<4I", b[4:20])
            blk = ((i0, j0, ni, nj), list(struct.unpack("<%df" % (ni * nj), b[20:])))
        chunks[(e["x"], e["z"])] = (c, blk)
    rows_by_model, tiles, objects, solids = {}, [], [], []
    last_order = 0
    for c, _ in chunks.values():
        for r in c["instances"]:
            rows_by_model.setdefault(r["model"], []).append(r)
            objects.append(r)
            if r["model"] == GROUND:
                tiles.append(r)
            last_order = max(last_order, r.get("order", 0))
        for s in c["solids"]:
            solids.append(s["box"])
    for r in region.get("persistent", []):
        last_order = max(last_order, r.get("order", 0))
    order = int(last_order) + 1

    # Height lattice: a terrain region needs heights under its ring chunks.
    lattice = None
    if terrain:
        hox, hoz = region["height"]["origin"]
        step = float(region["height"]["step"])
        nx, nz = region["height"]["count"]
        kx = int(math.ceil((hox - nox) / step - 1e-6))
        kz = int(math.ceil((hoz - noz) / step - 1e-6))
        lox, loz = hox - kx * step, hoz - kz * step
        nnx = int(math.ceil((nox + ncols * C - lox) / step - 1e-6)) + 1
        nnz = int(math.ceil((noz + nrows * C - loz) / step - 1e-6)) + 1
        old = [0.0] * (nx * nz)
        for (cx, cz), (_, blk) in chunks.items():
            (i0, j0, ni, nj), vals = blk
            for j in range(nj):
                old[(j0 + j) * nx + i0: (j0 + j) * nx + i0 + ni] = vals[j * ni:(j + 1) * ni]
        blocks = {k: v[1][0] for k, v in chunks.items()}
        col_of = owners_per_axis(blocks, nx, kx, nnx, lox, step, nox, C, ncols, D, 0)
        row_of = owners_per_axis(blocks, nz, kz, nnz, loz, step, noz, C, nrows, D, 1)
        hx1, hz1 = hox + (nx - 1) * step, hoz + (nz - 1) * step

        def new_height(i, j):
            oi, oj = i - kx, j - kz
            if 0 <= oi < nx and 0 <= oj < nz:
                return old[oj * nx + oi]
            x, z = lox + i * step, loz + j * step
            ci, cj = min(nx - 1, max(0, oi)), min(nz - 1, max(0, oj))
            d = math.hypot(max(hox - x, 0, x - hx1), max(hoz - z, 0, z - hz1))
            return old[cj * nx + ci] + BANK_HEIGHT * smoothstep(BANK_FROM, BANK_TO, d)

        heights = [[new_height(i, j) for i in range(nnx)] for j in range(nnz)]
        lattice = (lox, loz, step, nnx, nnz, heights, col_of, row_of, kx, kz)

        def ground_y(x, z):
            u = min(max((x - lox) / step, 0), nnx - 1)
            v = min(max((z - loz) / step, 0), nnz - 1)
            i, j = min(int(u), nnx - 2), min(int(v), nnz - 2)
            a, b = u - i, v - j
            h = heights
            return (1 - b) * ((1 - a) * h[j][i] + a * h[j][i + 1]) + b * ((1 - a) * h[j + 1][i] + a * h[j + 1][i + 1])
    else:
        ground_y = None

    def in_ring(x, z):
        cx, cz = int(math.floor((x - nox) / C)), int(math.floor((z - noz) / C))
        if not (0 <= cx < ncols and 0 <= cz < nrows):
            return None
        if D <= cx < D + cols and D <= cz < D + rows:
            return None
        return cx, cz

    ring = {(cx, cz): {"instances": [], "solids": [], "grass": []}
            for cz in range(nrows) for cx in range(ncols) if in_ring(nox + (cx + .5) * C, noz + (cz + .5) * C)}
    added_to_old = {}  # old chunk (new grid key) -> ground tile rows for the ring

    # Ground tiles: carry the region's own tile lattice out over the ring, with
    # the same turn as its tiles (the kit tiles only meet cleanly that way).
    new_tiles = []
    if tiles and not terrain:
        mesh = TileMesh(assets / "EA_GROUND.gltf")
        relief = float(region.get("terrain_instance_y_scale", 1))
        sp = float(region["ground_spacing"])
        yaws = {t["yaw"] for t in tiles}
        if len(yaws) != 1:
            raise SystemExit("edge ring: the region's ground tiles are turned differently")
        yaw = yaws.pop()
        dx, dz = rotate(yaw, -TILE / 2, TILE / 2)  # anchor corner -> tile centre
        centres = [(t["position"][0] + dx, t["position"][2] + dz) for t in tiles]
        y0 = tiles[0]["position"][1]
        c0 = centres[0]
        have = {(round((x - c0[0]) / sp), round((z - c0[1]) / sp)) for x, z in centres}
        k_lo = int(math.floor((nox - TILE / 2 - c0[0]) / sp)), int(math.floor((noz - TILE / 2 - c0[1]) / sp))
        k_hi = int(math.ceil((nox + ncols * C + TILE / 2 - c0[0]) / sp)), int(math.ceil((noz + nrows * C + TILE / 2 - c0[1]) / sp))
        for kz in range(k_lo[1], k_hi[1] + 1):
            for kx in range(k_lo[0], k_hi[0] + 1):
                cx, cz = c0[0] + kx * sp, c0[1] + kz * sp
                if (kx, kz) in have:
                    continue
                # Skip tiles that touch nothing inside the new grid.
                if cx + TILE / 2 <= nox or cx - TILE / 2 >= nox + ncols * C or                         cz + TILE / 2 <= noz or cz - TILE / 2 >= noz + nrows * C:
                    continue
                px, pz = cx - dx, cz - dz
                key = (int(math.floor((px - nox) / C)), int(math.floor((pz - noz) / C)))
                if not (0 <= key[0] < ncols and 0 <= key[1] < nrows):
                    continue  # anchor beyond the grid: the tile only covers ground past the ring
                row = {"id": "ring_ground_%d_%d" % (kx, kz), "model": GROUND, "position": [px, y0, pz],
                       "yaw": yaw, "scale": 1, "order": order}
                order += 1
                if key in ring:
                    ring[key]["instances"].append(row)
                else:
                    added_to_old.setdefault(key, []).append(row)
                new_tiles.append(row)
        all_tiles = tiles + new_tiles

        def ground_y(x, z):
            best = None
            for t in all_tiles:
                a = math.radians(t["yaw"])
                c, s = math.cos(a), math.sin(a)
                wx, wz = x - t["position"][0], z - t["position"][2]
                lx, lz = wx * c - wz * s, wx * s + wz * c
                if -TILE - .01 <= lx <= .01 and -.01 <= lz <= TILE + .01:
                    h = mesh.height(lx, lz)
                    if h is not None:
                        y = t["position"][1] + relief * h
                        best = y if best is None else max(best, y)
            return best if best is not None else 0.0
    elif ground_y is None:
        def ground_y(x, z):
            return 0.0

    # Forest band: dart throwing inside the ring, nearest the walk edge first.
    rng = random.Random(seed)

    def past_walk(x, z):
        return math.hypot(max(walk[0] - x, 0, x - walk[2]), max(walk[1] - z, 0, z - walk[3]))

    blockers = [(r["position"][0], r["position"][2], 1.6) for r in objects
                if r["model"].startswith(("WSP_TREE", "EA_TREE", "EA_ROCK", "WSP_CLIFF"))]
    boxes = [(x, z, hx + 1.0, hz + 1.0) for x, z, hx, hz in solids]
    placed = []
    area = sum(1 for _ in ring) * C * C
    counts = {"trees": 0, "bushes": 0, "rocks": 0}

    def scatter(kind, band, gap, pick, tries_per_m2):
        nonlocal order
        for _ in range(int(area * tries_per_m2)):
            x = rng.uniform(nox, nox + ncols * C)
            z = rng.uniform(noz, noz + nrows * C)
            key = in_ring(x, z)
            if key is None:
                continue
            d = past_walk(x, z)
            if not band[0] <= d <= band[1]:
                continue
            if any(abs(x - bx) < hx and abs(z - bz) < hz for bx, bz, hx, hz in boxes):
                continue
            if any(math.hypot(x - px, z - pz) < gap + pg for px, pz, pg in placed):
                continue
            if kind != "bushes" and any(math.hypot(x - px, z - pz) < gap + pg for px, pz, pg in blockers):
                continue
            model = pick(d)
            if model is None:
                continue
            t = rng.choice(rows_by_model[model])
            row = {"id": "ring_%s_%d" % (model.lower(), order), "model": model,
                   "position": [x, ground_y(x, z), z], "yaw": rng.randrange(360),
                   "scale": t.get("scale", 1), "order": order}
            order += 1
            ring[key]["instances"].append(row)
            placed.append((x, z, gap))
            counts[kind] += 1

    near = [m for m in TREES_NEAR if m in rows_by_model]
    far = [m for m in TREES_FAR if m in rows_by_model] or near
    bushes = [m for m in BUSHES if m in rows_by_model]
    rocks = [m for m in ROCKS if m in rows_by_model]
    if near or far:
        scatter("trees", TREE_BAND, TREE_GAP,
                lambda d: rng.choice(near if (d < FAR_FROM or rng.random() < .35) and near else far), .6)
    if rocks:
        scatter("rocks", ROCK_BAND, ROCK_GAP, lambda d: rng.choice(rocks) if rng.random() < .1 else None, .05)
    if bushes:
        scatter("bushes", BUSH_BAND, BUSH_GAP, lambda d: rng.choice(bushes) if rng.random() < .5 else None, .3)

    # Write: old chunks move up D places, ring chunks join them.
    tmp = out / "chunks_new"
    if tmp.exists():
        shutil.rmtree(tmp)
    tmp.mkdir()
    listing = []

    def write_height(name, block, vals):
        i0, j0, ni, nj = block
        (tmp / (name + ".height.bin")).write_bytes(
            b"WSH1" + struct.pack("<4I", i0, j0, ni, nj) + struct.pack("<%df" % len(vals), *vals))
        return name + ".height.bin"

    def lattice_block(cx, cz):
        lox_, loz_, step, nnx, nnz, heights, col_of, row_of, kx, kz = lattice
        ii = [i for i in range(nnx) if col_of[i] == cx]
        jj = [j for j in range(nnz) if row_of[j] == cz]
        block = (ii[0], jj[0], len(ii), len(jj))
        vals = [heights[j][i] for j in jj for i in ii]
        return block, vals

    for (x, z), (c, blk) in chunks.items():
        nx_, nz_ = x + D, z + D
        name = "c_%d_%d" % (nx_, nz_)
        c = dict(c, x=nx_, z=nz_)
        if (nx_, nz_) in added_to_old:
            c["instances"] = c["instances"] + added_to_old[(nx_, nz_)]
        if blk is not None:
            if lattice:
                block, vals = lattice_block(nx_, nz_)
                (i0, j0, ni, nj), old_vals = blk
                if block != (i0 + lattice[8], j0 + lattice[9], ni, nj) or vals != old_vals:
                    raise SystemExit("edge ring: moved height block differs (%s)" % name)
            else:
                block, vals = blk
            c["height"] = write_height(name, block, vals)
        (tmp / (name + ".json")).write_text(json.dumps(c, separators=(",", ":")))
        listing.append({"x": nx_, "z": nz_, "file": "chunks/" + name + ".json"})
    for (cx, cz), c in ring.items():
        name = "c_%d_%d" % (cx, cz)
        c = {"revision": 1, "x": cx, "z": cz, **c}
        if lattice:
            block, vals = lattice_block(cx, cz)
            c["height"] = write_height(name, block, vals)
        (tmp / (name + ".json")).write_text(json.dumps(c, separators=(",", ":")))
        listing.append({"x": cx, "z": cz, "file": "chunks/" + name + ".json", "ring": True})
    shutil.rmtree(out / "chunks")
    tmp.rename(out / "chunks")
    listing.sort(key=lambda e: (e["z"], e["x"]))

    region["grid"] = {"origin": [nox, noz], "columns": ncols, "rows": nrows}
    if lattice:
        region["height"] = {"origin": [lattice[0], lattice[1]], "step": lattice[2], "count": [lattice[3], lattice[4]]}
    region["chunks"] = listing
    region["edge_ring"] = {"depth": D, "chunks": len(ring), "ground_tiles": len(new_tiles),
                           "ground_tiles_in_old_chunks": sum(len(v) for v in added_to_old.values()), **counts}
    (out / "region.json").write_text(json.dumps(region, indent=1))
    print("edge ring", region_id, "grid", ncols, "x", nrows, region["edge_ring"])
    return region["edge_ring"]


def main():
    depth = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    if depth < 1:
        raise SystemExit("depth must be at least 1")
    add_ring(sys.argv[1], sys.argv[2], depth)


if __name__ == "__main__":
    main()
