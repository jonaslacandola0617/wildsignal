"""Caps how much a region's bushes are scaled up.

Usage: python -I tools/fix_bushes.py REGION_FOLDER MAX_SCALE

In the dream look leaf cards keep their size however big a bush is, so past about twice its
size a WSP_BUSH_0 shows its bare dark core with leaves scattered round it. Any bush above
MAX_SCALE is set to a size spread evenly between MAX_SCALE * 0.8 and MAX_SCALE (from its old
scale, so neighbours still differ). Running it twice changes nothing.
"""
import json
import sys
from pathlib import Path

MODEL = "WSP_BUSH_0"


def main():
    folder, limit = Path(sys.argv[1]), float(sys.argv[2])
    region = json.loads((folder / "region.json").read_text(encoding="utf-8"))
    changed = total = 0
    for entry in region["chunks"]:
        path = folder / entry["file"]
        chunk = json.loads(path.read_text(encoding="utf-8"))
        dirty = False
        for row in chunk.get("instances", []):
            if row["model"] != MODEL:
                continue
            total += 1
            if row.get("scale", 1) > limit:
                row["scale"] = round(limit * (.8 + .2 * ((row["scale"] * 7.3) % 1)), 3)
                changed += 1
                dirty = True
        if dirty:
            path.write_text(json.dumps(chunk, indent=1), encoding="utf-8")
    print("%s: %d of %d bushes capped at %.2f" % (folder, changed, total, limit))


if __name__ == "__main__":
    main()
