"""SPAWN-001: give a region extra scene objects for Signari, so several of one
species can be on screen (spawned wilds, the story wild and a team member).

Usage: python -I tools/spawn_slots.py REGION_JSON [MODEL=COPIES ...]
Default: WS005=5 WS012=5 WS004=2 WS008=2.

Copies the region's existing persistent row for each model as "<id>#1", "<id>#2", ...
(same model, scale and home; the game hides unused ones out of sight). Rows that
already exist are kept, so running it twice changes nothing. Each copy gets an
"order" after every other row, so draws of the world itself do not change.
"""
import json
import sys
from pathlib import Path

DEFAULT = {"WS005": 5, "WS012": 5, "WS004": 2, "WS008": 2}


def add_slots(region, want, folder=None):
    rows = region["persistent"]
    orders = [r.get("order", 0) for r in rows]
    for entry in region.get("chunks", []) if folder else []:
        chunk = json.loads((folder / entry["file"]).read_text(encoding="utf-8"))
        orders += [r.get("order", 0) for r in chunk.get("instances", [])]
    top = max(orders + [0])
    ids = {r["id"] for r in rows}
    added = 0
    for model, copies in want.items():
        base = next((r for r in rows if r["model"] == model and "#" not in r["id"]), None)
        if base is None:
            continue
        for k in range(1, copies + 1):
            rid = "%s#%d" % (base["id"], k)
            if rid in ids:
                continue
            row = dict(base)
            row["id"] = rid
            top += 1
            row["order"] = top
            rows.append(row)
            ids.add(rid)
            added += 1
    return added


def main():
    path = Path(sys.argv[1])
    want = dict(DEFAULT)
    for arg in sys.argv[2:]:
        model, n = arg.split("=")
        want[model] = int(n)
    region = json.loads(path.read_text(encoding="utf-8"))
    added = add_slots(region, want, path.parent)
    if added:
        path.write_text(json.dumps(region, indent=1), encoding="utf-8")
    print("%s: %d slots added" % (path, added))


if __name__ == "__main__":
    main()
