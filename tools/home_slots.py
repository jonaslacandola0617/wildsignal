"""HOME-001: gives a region hidden scene objects for the homestead's placeable pieces.

Usage: python -I tools/home_slots.py REGION_JSON HOMESTEAD_JSON

For a region with a homestead site, each piece that has a model gets one persistent row
"home_<piece>" and limit-1 copies, drawn after every other row so the world's own draws do
not change. They wait 40 m underground until placed. The models must already be in the region's style pack. Running it twice changes nothing.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from spawn_slots import add_slots  # noqa: E402


def main():
    region_path, home_path = Path(sys.argv[1]), Path(sys.argv[2])
    region = json.loads(region_path.read_text(encoding="utf-8"))
    home = json.loads(home_path.read_text(encoding="utf-8"))
    if not any(s["region"] == region["id"] for s in home["sites"]):
        print("%s: no homestead site" % region_path)
        return
    pack = json.loads((region_path.parent.parent.parent / region["shared"]["packs"]["style"]).read_text(encoding="utf-8"))
    ids = {m["id"] for m in pack["models"]}
    rows = region["persistent"]
    orders = [r.get("order", 0) for r in rows]
    for entry in region.get("chunks", []):
        chunk = json.loads((region_path.parent / entry["file"]).read_text(encoding="utf-8"))
        orders += [r.get("order", 0) for r in chunk.get("instances", [])]
    top = max(orders + [0])
    want, added = {}, 0
    have = {r["id"] for r in rows}
    for piece in home["pieces"]:
        model = piece.get("model")
        if not model:
            continue
        if model not in ids:
            raise SystemExit("%s is not in the region's style pack" % model)
        rid = "home_" + piece["id"]
        if rid not in have:
            top += 1
            rows.append({"id": rid, "model": model, "position": [0, -40, 0], "yaw": 0, "scale": piece["scale"], "order": top})
            added += 1
        want[model] = piece["limit"] - 1
    slots = add_slots(region, want, region_path.parent)
    region_path.write_text(json.dumps(region, indent=1), encoding="utf-8")
    print("%s: %d pieces, %d copies" % (region_path, added, slots))


if __name__ == "__main__":
    main()
