"""SPAWN-002: puts Signari models into a region, so the game can show them as wilds,
team members and fight foes.

Usage: python -I tools/add_signari.py REGION_JSON MANIFEST_JSON [--copies N] [--spawns SPAWNS_JSON]

MANIFEST_JSON lists the converted models ({"id": "WS013", "scale": 0.4167, ...}); their
.glb/.png files must already sit in the assets folder beside the packs. For each model:
- the region's packs that already hold Signari (any "WS0nn" entry) get {"id", "path"};
- the region gets one persistent row "wild_ws0nn" (hidden until used) plus COPIES-1
  extra rows (default 3 in all), drawn after every other row so the world's own
  draws do not change. Existing rows and entries are kept; running it twice changes nothing.
With --spawns, only species in that region's spawn table are added: every Signari
row is loaded at start and kept, so unused ones would only cost memory.
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from spawn_slots import add_slots  # noqa: E402

SIGNARI = re.compile(r"^WS0\d\d$")


def main():
    region_path, manifest_path = Path(sys.argv[1]), Path(sys.argv[2])
    rest = sys.argv[3:]
    copies, spawns = 3, None
    while rest:
        flag, value = rest[0], rest[1]
        rest = rest[2:]
        if flag == "--copies":
            copies = int(value)
        elif flag == "--spawns":
            spawns = Path(value)
        else:
            raise SystemExit("unknown option " + flag)
    region = json.loads(region_path.read_text(encoding="utf-8"))
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if spawns:
        table = json.loads(spawns.read_text(encoding="utf-8"))["regions"].get(region["id"], {"zones": []})
        used = {e["species"].replace("-", "") for z in table["zones"] for e in z["entries"]}
        manifest = [m for m in manifest if m["id"] in used]
    assets = region_path.parent.parent.parent
    packs_changed = 0
    for name in sorted(set(region["shared"]["packs"].values())):
        path = assets / name
        pack = json.loads(path.read_text(encoding="utf-8"))
        ids = {m["id"] for m in pack["models"]}
        if not any(SIGNARI.match(i) for i in ids):
            continue
        new = [{"id": m["id"], "path": m["id"] + ".glb"} for m in manifest if m["id"] not in ids]
        for entry in new:
            if not (assets / entry["path"]).exists():
                raise SystemExit("missing model file " + str(assets / entry["path"]))
        if new:
            pack["models"] += new
            path.write_text(json.dumps(pack, indent=1), encoding="utf-8")
            packs_changed += 1
    rows = region["persistent"]
    have = {r["model"] for r in rows}
    added = 0
    for m in manifest:
        if m["id"] in have:
            continue
        rows.append({"id": "wild_" + m["id"].lower(), "model": m["id"], "position": [0, 0, 0], "yaw": 0,
                     "scale": m["scale"], "order": 0})
        added += 1
    # Orders after every chunk row, as spawn_slots does for the copies.
    fresh = [r for r in rows if r["order"] == 0 and r["id"].startswith("wild_ws0")]
    if fresh:
        for r in fresh:
            rows.remove(r)
        base = {"persistent": rows, "chunks": region.get("chunks", [])}
        top = 0
        for entry in base["chunks"]:
            chunk = json.loads((region_path.parent / entry["file"]).read_text(encoding="utf-8"))
            top = max([top] + [r.get("order", 0) for r in chunk.get("instances", [])])
        top = max([top] + [r.get("order", 0) for r in rows])
        for k, r in enumerate(fresh):
            r["order"] = top + 1 + k
            rows.append(r)
    slots = add_slots(region, {m["id"]: copies - 1 for m in manifest}, region_path.parent)
    region_path.write_text(json.dumps(region, indent=1), encoding="utf-8")
    print("%s: %d packs changed, %d models added, %d extra slots" % (region_path, packs_changed, added, slots))


if __name__ == "__main__":
    main()
