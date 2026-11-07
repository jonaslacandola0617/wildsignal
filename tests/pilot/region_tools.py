"""REGION-001: the region tools keep every actor out of the chunks.

Usage: python -I tests/pilot/region_tools.py TOOLS_DIR

Builds a tiny village (one player, one companion, two Signari scene objects,
two trees) in a temporary folder, converts it with village_to_region.py and
tiles it with tile_test_region.py. Signari scene objects are moved by the game
as party members and wilds, and a chunk's object slots are reused when it
unloads, so they must be "persistent" rows, listed once, and never in a chunk.
"""
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ACTORS = {"player_actor", "companion", "wild", "pilot_ws004", "pilot_ws008"}


def row(id_, model, x, z):
    return {"id": id_, "model": model, "position": [x, 0.2, z], "rotation_y": 0, "scale": 1}


def check(ok, message):
    if not ok:
        raise SystemExit("FAIL: " + message)


def chunk_ids(region):
    ids = []
    for entry in json.loads((region / "region.json").read_text())["chunks"]:
        ids += [r["id"] for r in json.loads((region / entry["file"]).read_text())["instances"]]
    return ids


def main():
    tools = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as tmp:
        assets = Path(tmp)
        instances = [row("player_actor", "WS0P_PLAYER", 0, 0), row("companion", "PETALYN", 1, 0),
                     row("wild", "PETALYN", 2, 0), row("pilot_ws004", "WS004", -6.4, -2.6),
                     row("pilot_ws008", "WS008", 3.2, 4.6), row("tree_a", "EA_TREE_0", -20, 20),
                     row("tree_b", "EA_TREE_1", 20, -20)]
        (assets / "scene.json").write_text(json.dumps({"name": "TINY", "instances": instances}))
        (assets / "game.json").write_text(json.dumps({
            "world_half_extent": 29.65, "solids": [[-20, 20, 1, 1]], "house_solids": [],
            "ground_spacing": 19.9, "spawn": {"x": 0, "z": 0}, "wild": {"x": 2, "z": 0},
            "rest_spots": [], "review_cameras": [], "exclude_materials": []}))
        (assets / "grass_00.json").write_text(json.dumps([{"position": [5, 0, 5]}]))
        (assets / "vegetation.json").write_text(json.dumps({"instance_files": ["grass_00.json"]}))
        (assets / "terrain_height.bin").write_bytes(struct.pack("<%df" % (257 * 257), *([0.0] * 257 * 257)))
        (assets / "pack_style.json").write_text(json.dumps({"revision": 1, "models": []}))

        for cmd in ([sys.executable, "-I", str(tools / "village_to_region.py"), str(assets)],
                    [sys.executable, "-I", str(tools / "tile_test_region.py"), str(assets), str(assets)]):
            r = subprocess.run(cmd, capture_output=True, text=True)
            check(r.returncode == 0, "%s failed\n%s%s" % (Path(cmd[2]).name, r.stdout, r.stderr))

        for name, tiles in (("hearthgrove_test", 1), ("streaming_test", 9)):
            region = assets / "regions" / name
            persistent = [r["id"] for r in json.loads((region / "region.json").read_text())["persistent"]]
            ids = chunk_ids(region)
            check(sorted(persistent) == sorted(ACTORS), "%s persistent rows %s" % (name, persistent))
            leaked = [i for i in ids if i.split("@")[0] in ACTORS]
            check(not leaked, "%s has actors in chunks: %s" % (name, leaked))
            check(len(ids) == 2 * tiles, "%s has %d chunk objects, expected %d" % (name, len(ids), 2 * tiles))
    print("region tools keep actors persistent: ok")


if __name__ == "__main__":
    main()
