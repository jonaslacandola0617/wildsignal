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
    print("spawn slots ok")


if __name__ == "__main__":
    main()
