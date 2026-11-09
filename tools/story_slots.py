"""STORY-R1: gives the story's region hidden scene objects for its people.

Usage: python -I tools/story_slots.py REGION_JSON STORY_JSON

Each person whose model is not a Signari (those come from add_signari.py) gets one persistent
row "story_<id>", waiting 40 m underground and drawn after every other row. Models must already
be in the region's style pack. Running it twice changes nothing.
"""
import json
import re
import sys
from pathlib import Path


def main():
    region_path, story_path = Path(sys.argv[1]), Path(sys.argv[2])
    region = json.loads(region_path.read_text(encoding="utf-8"))
    story = json.loads(story_path.read_text(encoding="utf-8"))
    if story["region"] != region["id"]:
        print("%s: not the story's region" % region_path)
        return
    pack = json.loads((region_path.parent.parent.parent / region["shared"]["packs"]["style"]).read_text(encoding="utf-8"))
    ids = {m["id"] for m in pack["models"]}
    rows = region["persistent"]
    orders = [r.get("order", 0) for r in rows]
    for entry in region.get("chunks", []):
        chunk = json.loads((region_path.parent / entry["file"]).read_text(encoding="utf-8"))
        orders += [r.get("order", 0) for r in chunk.get("instances", [])]
    top = max(orders + [0])
    have = {r["id"] for r in rows}
    added = 0
    for person in story["people"]:
        if re.match(r"^WS0\d\d$", person["model"]):
            continue
        if person["model"] not in ids:
            raise SystemExit("%s is not in the region's style pack" % person["model"])
        rid = "story_" + person["id"]
        if rid in have:
            continue
        top += 1
        rows.append({"id": rid, "model": person["model"], "position": [0, -40, 0], "yaw": 0, "scale": person["scale"], "order": top})
        added += 1
    region_path.write_text(json.dumps(region, indent=1), encoding="utf-8")
    print("%s: %d people" % (region_path, added))


if __name__ == "__main__":
    main()
