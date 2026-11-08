"""SPAWN-001: tools/spawn_slots.py adds Signari copies as persistent rows, after every
other row in draw order, and running it twice changes nothing.

Usage: python -I tests/pilot/spawn_slots.py TOOLS_DIR
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def check(ok, message):
    if not ok:
        raise SystemExit("FAIL: " + message)


def main():
    tools = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        (folder / "chunks").mkdir()
        (folder / "chunks/c_0_0.json").write_text(json.dumps({"instances": [{"id": "tree", "model": "T", "order": 500}]}))
        region = {"persistent": [{"id": "pilot_ws005", "model": "WS005", "position": [1, 0, 2], "order": 10},
                                 {"id": "player_actor", "model": "WS0P_PLAYER", "position": [0, 0, 0], "order": 11}],
                  "chunks": [{"x": 0, "z": 0, "file": "chunks/c_0_0.json"}]}
        path = folder / "region.json"
        path.write_text(json.dumps(region))
        run = lambda: subprocess.run([sys.executable, "-I", str(tools / "spawn_slots.py"), str(path), "WS005=3"],
                                     check=True, capture_output=True, text=True)
        run()
        rows = json.loads(path.read_text())["persistent"]
        copies = [r for r in rows if r["id"].startswith("pilot_ws005#")]
        check(len(copies) == 3, "three copies of WS005")
        check(all(r["model"] == "WS005" and r["position"] == [1, 0, 2] for r in copies), "copies keep model and home")
        check(min(r["order"] for r in copies) > 500, "copies are drawn after every chunk row")
        check(len({r["order"] for r in rows}) == len(rows), "orders stay unique")
        before = path.read_text()
        run()
        check(path.read_text() == before, "second run changes nothing")

        # add_signari.py: pack entry + hidden row + copies, only for species in the region's table.
        assets = folder / "assets"
        (assets / "regions/r/chunks").mkdir(parents=True)
        (assets / "regions/r/chunks/c_0_0.json").write_text(json.dumps({"instances": [{"id": "tree", "model": "T", "order": 500}]}))
        (assets / "pack_style.json").write_text(json.dumps({"models": [{"id": "WS005", "path": "WS005.glb"}]}))
        for m in ("WS013", "WS020"):
            (assets / (m + ".glb")).write_bytes(b"glTF")
        reg = assets / "regions/r/region.json"
        reg.write_text(json.dumps({"id": "r", "shared": {"packs": {"default": "pack_style.json", "source": "pack_style.json", "style": "pack_style.json"}},
                                   "persistent": [{"id": "pilot_ws005", "model": "WS005", "position": [0, 0, 0], "order": 10}],
                                   "chunks": [{"x": 0, "z": 0, "file": "chunks/c_0_0.json"}]}))
        manifest = folder / "manifest.json"
        manifest.write_text(json.dumps([{"id": "WS013", "scale": 0.4}, {"id": "WS020", "scale": 0.5}]))
        spawns = folder / "spawns.json"
        spawns.write_text(json.dumps({"regions": {"r": {"zones": [{"entries": [{"species": "WS-013"}]}]}}}))
        add = lambda: subprocess.run([sys.executable, "-I", str(tools / "add_signari.py"), str(reg), str(manifest), "--spawns", str(spawns)],
                                     check=True, capture_output=True, text=True)
        add()
        rows = json.loads(reg.read_text())["persistent"]
        pack = json.loads((assets / "pack_style.json").read_text())["models"]
        check([r["id"] for r in rows if r["model"] == "WS013"] == ["wild_ws013", "wild_ws013#1", "wild_ws013#2"], "WS013 gets a row and 2 copies")
        check(all(r["scale"] == 0.4 and r["order"] > 500 for r in rows if r["model"] == "WS013"), "manifest scale, drawn after the world")
        check(not any(r["model"] == "WS020" for r in rows) and not any(m["id"] == "WS020" for m in pack), "species not in the table are left out")
        check({"id": "WS013", "path": "WS013.glb"} in pack, "pack lists the model")
        before = reg.read_text()
        add()
        check(reg.read_text() == before, "add_signari twice changes nothing")

        # home_slots.py: hidden rows for the placeable pieces, only in a region with a site.
        (assets / "pack_style.json").write_text(json.dumps({"models": [{"id": "EA_BARREL", "path": "b.gltf"}]}))
        home = folder / "homestead.json"
        home.write_text(json.dumps({"sites": [{"region": "r"}], "pieces": [{"id": "plot"}, {"id": "barrel", "model": "EA_BARREL", "limit": 3, "scale": 1}]}))
        slots = lambda: subprocess.run([sys.executable, "-I", str(tools / "home_slots.py"), str(reg), str(home)], check=True, capture_output=True, text=True)
        slots()
        ids = [r["id"] for r in json.loads(reg.read_text())["persistent"] if r["model"] == "EA_BARREL"]
        check(ids == ["home_barrel", "home_barrel#1", "home_barrel#2"], "a barrel row per allowed barrel")
        before = reg.read_text()
        slots()
        check(reg.read_text() == before, "home_slots twice changes nothing")
    print("spawn slots ok")


if __name__ == "__main__":
    main()
