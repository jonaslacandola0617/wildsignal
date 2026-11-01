# Field-kit UI review pass

The field book now uses private registered book-opening and page-turn sprites supplied by the owner. Its pages use a defined margin/gutter layout, readable page headings, clickable named bookmarks, keyboard page controls and a reduced-motion toggle. Portraits are optional private sprites; the supplied review assets derive from existing game models. Wild Signari show a silhouette until the existing observation record reveals their portrait. Questions and answers, trust, lessons, joining and save semantics still use the existing bond journal.

Exploration feedback is consolidated into one paper panel. Frightened encounters explain giving space before offering an observation action. Opening the book dismisses the transient notice and removes other exploration HUD elements from the reading view. Pause uses the same paper/button vocabulary with keyboard and pointer controls.

## Controls

- B or click the book icon: open; B/Escape or Close: close.
- A/D or Left/Right, Back/Next or bookmarks: navigate. Home: first spread.
- M inside the book: toggle reduced motion for the current session.
- Escape outside the book: pause; Enter/Escape/Continue: resume; Q/Save and quit: save and exit.

World movement and camera input are blocked while reading. Closing clears held movement keys. Opening/closing and turns use timed presentation only; they do not award trust, damage or items. Text is shown once a turn settles so it is not painted across a moving page. Existing save version 3 remains unchanged. Reduced-motion choice is session-local in this review build, not a complete settings screen.

## Private assets

The public repository contains code only. The private runtime requires its updated ui_paper.png/.json and folio.json. Rectangles can include two optional sprite-origin fields after the existing border value; older five-field atlases still work. Supplied opening and turn sequences are exported from source pixels, with registered origins. Runtime opening is 0.4 seconds and page turning 0.36 seconds; exported Aseprite derivatives preserve original frame durations. The original uploads remain unchanged.

The current review atlas is 3072×2304 RGBA (27 MiB). This is separate from world texture counters and is logged as ui_texture_bytes. Preserve the i3/4 GB and Galaxy S10+ requirements; current mobile and hardware UI validation remain outstanding. No postprocessing blur or new creature model pipeline is part of this pass.

## Scope and validation

This pass covers exploration, the field book and pause. Combat command layout, audio, title screen, localization, full text-size settings, Android touch/lifecycle and controller navigation remain separate work. This is an owner visual-review build, not a declaration of production art approval.

Run the existing CTests. With the private folio assets and a GL display, tests/fieldbook_runtime.py checks pointer/keyboard navigation at 640 and 1280 windows, movement/camera isolation, close/resume, pause/resume and safe save/quit. It can also be enabled in CTest through the existing private-asset/GL flags. Private source frame exports have separate RGB-preservation and Aseprite round-trip validation logs.
