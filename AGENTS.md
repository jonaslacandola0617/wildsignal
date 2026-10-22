# WILDSIGNAL — Codex Project Rules (AGENTS.md)

These instructions apply to Codex tasks that run in a branch CONTAINING this file. Read the referenced specs before modifying production systems. Do not treat a prose plan as an asset.

## Mission and confirmed product boundaries
- Original first-person retro 2.5D creature-companion RPG, mood **beautiful but mysterious**.
- User's original hand-drawn 100-creature source is **canonical**: no AI redesign, generative replacement, public upload, mass conversion or changes without explicit permission.
- C++20 + SDL3 + CMake, modest Windows and Android devices; Windows game target **x86_64 OS/binaries on Win7 SP1–11**, actual compatibility unverified; no 32-bit Windows builds.
- Offline full story; optional two-player Windows↔Android co-op at commercial 1.0; locally untrusted saves never grant online owned creatures or stats; F2P cosmetics only. These future systems are **NOT** authorization to implement them on renderer/art tasks.

## Read order
1. `docs/planning/DECISION_REGISTER_v0.1.md` (new owner confirmations override stale statuses).
2. `docs/planning/SPEC_INDEX_v0.1.md`.
3. `docs/design/VISUAL_BIBLE_v0.1.md` and `docs/design/HERO_SCENE_TARGETS_v0.1.md` for environment visuals.
4. `docs/design/VISUAL_QUALITY_RESET_v0.1.md` and `docs/design/assets/ART_ENV001_APPROVAL_AND_HANDOFF_v0.1.md` (owner APPROVED Quay & Liltwood *concept and visual REFERENCES*; REJECTED procedural RENDER-001 environment appearance).
5. For an assigned coding task, read its exact GitHub issue/hand-off and only referenced technical spec files. Do not silently expand scope.

## Mandatory asset-first process
1. If a task depends on a visual target, locate the **actual approved asset/source files and manifest**; textual description or generated concept screenshot ALONE is not a final game texture or 3D-ready building.
2. Treat `ART-ENV-001` as APPROVED **visual style reference**, NOT game-ready tiles, prop alpha, 3D roof/geometry or permission to publish. Its asset archive is held privately outside GitHub; if not provided in Codex's accessible working folder, **STOP and request it**. Never invent substitute art.
3. Environment assets must have stable IDs, image dimensions, source/provenance, pixel format/alpha, pivots/world scale (when applicable), SHA-256, version, tileability/QA status and explicit usage permission.
4. Separate environment art production from engineering. Do **not** create production house windows, trees and roofs as hardcoded C++ color/pixel loops if approved sprite/texture/geometry assets should be loaded. Temporary generated art is acceptable only for explicitly marked engineering tests.
5. Owner approves actual visual result through fixed-camera screenshots **AND** moving/turning preview. Build/CTest/pixel-golden success **does not equal art approval**.
6. If a desired roof, root arch, or irregular shape is impossible in the grid-raycast architecture from side views, report geometry/occlusion limits and propose a bounded RENDER-002 comparison; don't pretend a billboard card is a solid 3D roof.
7. Keep original environment atlas and recovered creature masters private by default; never push these bytes to public GitHub, online services, or third-party asset hosts without separate owner approval. AI-generated environmental references are distinct from the owner's hand-drawn creatures.
8. Conserve the owner's device RAM: stream/cache only needed materials, nearest-neighbor world pixels, crisp independent UI, no expensive effects without benchmark proof.

## Work and verification discipline
- Use a task-specific branch and a DRAFT PR. Preserve previous technical prototype branches and never auto-merge to main.
- Deliver an asset-validation log, a list of EXACT asset IDs/files used, screenshots from the actual running executable, off-angle screenshots, memory/frame telemetry and known limitations.
- Mark tests not run as **NOT RUN**. Windows 11 user screenshots are not Android/Win7 certification; timed capped frame sampling includes pacing overhead.
- If essential inputs, art rights, source files, renderer capabilities, or owner approval are missing: **STOP / ASK**, do not fill the gaps creatively and claim completion.

## Current phase
- ENGINE-001 and RENDER-001 are tested technical PROOFS; **RENDER-001 art appearance was explicitly rejected**.
- Lantern Quay and Liltwood concepts and generated ART-ENV-001 reference appearance were subsequently **owner-approved on 2026-10-21**.
- **ART-ENV-002** (production-quality seamless textures, manually cleaned props, direction/geometry contract and versioned manifest) is the next art gate, followed by a tiny in-engine fidelity integration proof. Do not expand to quests/co-op/battles during this art task.


