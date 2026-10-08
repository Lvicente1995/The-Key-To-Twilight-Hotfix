# v0.2.3 UI/name validation — 2 October 2026

Target: Dusklight 2.0.3, Windows x64, D3D12, isolated test installation and copied save.

- Final Windows build compiled, linked and packaged using the included `tools/build_windows.ps1` and pinned 2.0.3 SDK. The package contains 33 entries and is 4,433,470 bytes; SHA-256 `b83b0bd4beddbc82aa163c48477044d4c0be80c8ada8b2021a735f28b3c8f81e`.
- Final package activated and hot-reloaded successfully. The runtime log contains no error/fatal entry during this check.
- Visually verified the final Collection icon points upper right and the standard B-button icon points upper left, matching the user's approved artwork and requested horizontal menu flip.
- Visually verified the final selected item title reads **Kingdom Key**. The user subsequently confirmed the corrected result: “looks good”.
- The first runtime check exposed two issues despite successful compilation: the equipment title bypassed MessageService, and the native HUD applied an extra 76-degree sword rotation. Both were fixed and the final build rechecked in game.
- Text regression: 483 checks passed with MSVC `/W4 /WX`, including the real encoded pickup text, control tags, truncation/malformed data, fixed-size plain-menu replacement, guard bytes, language/ID scoping and service lifecycle.
- UI regression passed: resource/texture ownership, failure rollback, exact Ordon source matching, touch image revision refresh, disable/re-enable, exact Collection UV restoration, native HUD rotation restoration, unrelated panes, nested draws and bounded nesting. Final native integration also compiled against the real SDK.
- Compared against the supplied v0.2.2a source: Actions workflow, line-ending rules, SDK fetch helper, merge helper, portable audio, chain/presence/shield/effect implementation and every existing resource/model file remain byte-for-byte unchanged. Only UI/name code, new icon artwork, version and related documentation/helpers were added or changed.
- Source ZIP verification checks archive CRC, expected project/model topology, manifest version, included Actions workflow and absence of compiled build products.

Limits: this revision's eight-platform Actions build has not been run here. The supplied baseline had a user-reported successful full matrix build and Android runtime; the final v0.2.3 source still needs that matrix to produce an updated universal binary. Android/touch runtime and the acquisition scene were not observed for v0.2.3; their source/API paths are implemented and regression-checked. The English name is changed; other language translations and the original descriptive paragraph are preserved. Existing gameplay regressions were not repeated for this UI-only change.

---

# Historical v0.2.2 portability validation

This section records checks performed specifically for the v0.2.2 portability/audio patch. Historical validation below is retained from earlier Windows-focused iterations and should not be read as a runtime test of v0.2.2 on every platform.

- `mod.json` parses successfully.
- `.github/workflows/build.yml` parses successfully and contains the eight targets in Dusklight's current official mod-template matrix: Windows AMD64/ARM64, Linux x86_64/aarch64, macOS arm64/x86_64, iOS arm64, Android aarch64.
- Correction from review of the supplied v0.2.2a source: the actual SDK pin is commit `40457c6adb381928e4b5fef6ed459ed291edd5e2`, Dusklight **2.0.3**. The earlier 2.0.2 label was stale.
- The portable audio path resolves `JASCriticalSection` using platform-neutral HookService display names first, with MSVC and Itanium ABI fallbacks. If audio-lock resolution or sample registration fails after required hooks attach, the mod falls back to native sword audio. A required hook attachment failure still prevents initialization.
- CMake configure syntax was checked with a local stub SDK; configured `mod.json` contains LF line endings.
- Existing standalone regressions were rerun after the patch: indexed mesh PASS (52,072 triangles), anchor interpolation PASS (42,000 cases), presence PASS (65,536 exhaustive sequences), procedural FX historical regression PASS, KHIII FX regression PASS (8,208,016 assertions), KHIII effect budget PASS, and chain-physics stress completed with zero non-finite positions.
- The `res/` and sibling `model/` content are unchanged from v0.2.1; v0.2.2 changes source/build metadata rather than weapon/effect assets.
- Subsequent user evidence for the supplied v0.2.2a baseline: the full eight-platform build passed, and Android runtime worked. Other platforms' runtime behavior was not separately confirmed.

---

# Validation status

Target: mod v0.2.0 for Dusklight 2.0.3, Windows x64, D3D12. Updated 30 September 2026.

## v0.2.0 runtime status

**The user confirmed the corrected KHIII summon/dismiss appearance: “The effects look right now”.** This answered a check of the blue glow's alignment along the blade and improvement to the blocky, overly bright appearance. Slower shield movement, audio and hit triggering were approved earlier. Enemy-hit appearance was not separately retested after the latest shader corrections.

The runtime log confirms the latest deactivate/reload/activate sequence and KHIII activation message without errors. Earlier Collection-preview checks ran at 60 FPS. That preview observation does not establish combat frame rate or performance in every area.

An earlier screenshot exposed incorrect blue-glow alignment and a hard rectangular appearance after preliminary positive feedback. It prompted the orientation and material corrections below; the latest user response confirms the corrected result. Earlier shield feedback likewise led to the independent, slower shield animation.

Implemented/revised behavior: the Keyblade, fourteen links and charm appear in hand and disappear together; no stowed weapon or weapon shadow remains. The original scabbard is hidden. Source KHIII settings of 0.22-second visibility delay and 0.06-second dither time inform a seven-plus-two-tick state sequence in the 30 Hz host. Actual weapon visibility switches at seven ticks and the transition phase ends at nine; a native KHIII opacity-dither shader is not implemented. The source Key Fresnel effect supplies transition glow. The normal sword-arm reach-behind gesture is removed while the shield arm follows a separately sampled native animation and hand/back transfer frame. Following feedback that it moved too quickly, the final shield sampler runs at 0.75 playback speed with a five-tick settling blend. Native item cleanup and equipment setup remain in use; scripted events retain their animations.

The effects now use selected original KHIII textures and meshes with extracted Cascade settings adapted to Dusklight. Native KHIII cue `se02001_010` supplies both summon and dismiss audio, matching the weapon's shared appearance/disappearance effect reference. The seven cues referenced by `enumset_hit_w_so010` supply the hit family. Impact callbacks require confirmed, nonblocked enemy contact and deduplicate repeated callbacks for the same target within a game tick. The generic native hitmark is replaced only for accepted Keyblade enemy contacts; collision, damage and enemy reactions remain native.

Broader unverified cases include extreme rapid retriggering, every item/event transition, detailed audio volume/panning/pause behavior, all camera positions, wolf/form changes, teleport resets, mirrors, and performance across areas and combat situations. The inventory preview remains separate and uses a static chain pose. The positive runtime feedback does not establish exhaustive coverage of these cases.

## Latest visual correction

- Corrected the Unreal pitch/roll conversion so the blue glow and spiral geometry follow the blade. The source +X blade/+Z teeth basis and 1.26 scale are retained.
- Material reconstruction now uses actual emissive texture data for RGB and the separate opacity mask for alpha, with per-material color/brightness parameters. The previous white-RGB/alpha-mask treatment discarded part of the source material behavior.
- Restored UV scrolling from the source particle dynamic parameters for the spiral textures. The cooked data retains parameter values and function descriptions, but not complete graph wiring; the precise reconstruction remains an inference where those connections are missing.
- Removed the older forced whole-weapon white flash. This was a separate port behavior, not evidence from the extracted KHIII effect.

The corrected build passed a fresh `tools/build_windows.ps1` compile/link/package run. The installable archive has 32 entries and is 4,407,495 bytes. Its SHA-256 is `49afb325c565b820092553dc259c7ef4c0cf64ee6cc57094d9746b600c55fa25`. Build/load evidence and the separate user visual confirmation are recorded independently.

## Earlier v0.1.0 in-game checks

The earlier mesh/physics version was tested with the mod enabled in an isolated copy of Dusklight and a copy of the user's save. The original game installation and saves were not edited. These checks predate the new disappearing-weapon and gesture behavior.

- Loads successfully and replaces the equipped Ordon Sword. Master Sword remains separate.
- Visually checked hand alignment, proportions, stowed weapon, drawing and swinging.
- Fourteen chain links and the Mickey charm hang from the handle, respond to movement and settle independently of the weapon body.
- Final silver/gold/blue/black materials retain visible shading under the Kakariko scene lights. Initial dark and overbright lighting variants were corrected before release.
- Custom weapon shadow was visible on the ground and shallow-water surface.
- The checked v0.1.0 build ran at 60 FPS in that scene; this is not a v0.2.0 benchmark or a benchmark for every area or hardware configuration.
- Native reload succeeds and the chain recovers its hanging pose.
- Final swing/stow check: user confirmed the motion looked good with no reported chain issues.
- Collection/equipment preview: visually verified the Keyblade in Link's hand with the final materials.

## Current v0.2.0 automated and source checks

- Actual production presence header: 65,536 exhaustive 16-tick state sequences, 491,520 exact transition events and 12,000 stable ticks passed with KHIII visibility timing, rapid reversals, bounded emission and reset suppression.
- Independent shield-animation sampler: 1,860 owned joint samples passed across six playback rates in both directions, using the final 0.75 speed and five-tick blend. Checks cover the contact-frame edge, endpoint/fade duration, source-frame restoration, isolation from source mutation, cancellation and storage bounds. These do not validate the actual arm geometry or runtime hook alignment.
- The shield-animation integration and native one-shot audio compiled and linked against the Dusklight 2.0.3 Windows x64 SDK. Draw/dismiss and enemy-hit audibility, plus the slower shield movement, are user-confirmed. Detailed mixing behavior remains outside the completed check.
- Confirmed-contact filtering was reviewed against native hitmark inputs: the active Ordon replacement, Link's normal sword attack, a real enemy, accepted contact, and valid hit position are required. Per-target/per-tick deduplication limits duplicate cosmetic triggers. Only the generic hitmark call is replaced for accepted contacts; damage and collision handling occur outside that call. Hit triggering and sound were previously user-confirmed; the latest material changes were not separately retested on enemy hits, and every rejection branch has not been tested in gameplay.
- AudioMog decoded the selected Kingdom Key bank. Track-user mapping proves cue `010` maps to decoded track `012`; hit-family references prove cues `001` through `007`. WAV metadata was checked. The prepared samples are mono PCM16 at 48 kHz, with one common gain across the downmixed family and a maximum peak of 0.75. Summon and dismiss use the same sample without reversal.
- Fourteen selected effect textures were decoded from their native cooked payloads with format, dimensions, payload length and bulk offsets checked. The source masks, material texture references, colors/scalars, UV tiling and mirror/clamp/wrap addressing are recorded in `res/kh3fx/`. Visual inspection confirmed the quarter-glow/cross masks; their native mirrored addressing is essential to reconstruct their shape.
- Eight selected native effect meshes passed buffer-stride/count, section-index, finite-coordinate, serialized-bound and complete output round-trip checks. The current `KKFXM002` file contains 87,552 bytes, 2,810 vertices and 3,316 triangles, retaining source positions, UVs, indices and RGB/alpha vertex gradients. The report records per-mesh data and SHA-256.
- Current KHIII effect regression: 8,208,016 assertions passed. Checks cover source curve tangents and locked axes, uniform endpoints, duplicate-time keys, source colors/material reconstruction, child-emitter delays, identical seeded appearance effects for summon/dismiss, the source Key delay of 0.18311 seconds, deterministic sampling, owned snapshots, finite values, expiry, reset and sustained retriggering. The effect transform retains the source +X blade/+Z teeth basis and 1.26 attached-weapon scale, including the cancellation case when interpolating through a 180-degree change and the corrected Unreal pitch/roll convention.
- Actual indexed GX effect payload, measured using the shipped `KKFXM002` geometry: APP0 peaked at 78,288 bytes, HIT0 at 5,568 bytes, and retrigger stress at 187,008 bytes. All passed the 1 MiB effect-payload ceiling. These figures exclude the world's and weapon's separate render packets and do not establish runtime frame rate. Port simulation particle counts are not a measurement of KHIII's native engine particle count.
- Native equipment handoff retains item cleanup/setup, avoids a duplicate handoff, retains mounted sword cleanup and excludes scripted special stow. The shield animation is sampled into owned storage and layered only onto its arm joints. These source checks do not replace runtime verification.
- A fresh build through the delivered `tools/build_windows.ps1` compiled, linked and packaged the full current feature set successfully. Hot reload and Collection rendering also succeeded; the separate user gameplay result is recorded above.

Reproducible tests and recorded output include `tools/presence_regression.cpp`, `tools/presence-regression-result.txt`, `tools/shield_gesture_test.cpp`, `tools/shield-gesture-results.txt`, `tools/kh3_fx_regression.cpp`, `tools/kh3_fx_budget.cpp`, `tools/kh3-fx-results.txt` and `tools/kh3-fx-mesh-validation.json`. The KHIII effect result file includes commands for rerunning both its simulation and payload checks.

To rerun the presence regression, open an x64 Visual Studio developer shell in the project folder and run:

```powershell
cl /nologo /EHsc /std:c++20 /O2 /W4 tools/presence_regression.cpp /Fe:presence_regression.exe
.\presence_regression.exe
```

Keep assertions enabled: do not define `NDEBUG`. The shield test uses the Dusklight SDK types and minimal engine lifecycle stubs.

## Earlier v0.2.0 iteration checks

The procedural-light iteration compiled, packaged and loaded successfully, including a fresh build through the delivered build script. Its particle/presence checks recorded 2,851,060 passing checks, including 941,735 bounded particle samples. Those results concern the older `summon_fx.hpp` implementation and its fixed 39-particle pool. They do not validate the subsequent KHIII Cascade renderer, its extracted assets or the new sound/shield integration. The retained `tools/summon_fx_test.cpp` and `tools/summon-fx-results.txt` document that earlier iteration.

## Asset and established engineering checks

- FBX import, material repair and visual inspection of the Blender studio render.
- Rig articulation: rotating the fourth chain bone leaves the weapon and first three links fixed while moving the later links and charm.
- Native mesh readback: all 52,072 source triangles, sixteen rigid parts, finite vertex data, unit normals, correct materials and valid stream boundaries.
- Exact indexing reconstructs every original triangle and all vertex attributes byte-for-byte using 26,131 unique vertices. Normal/material seams and triangle order are preserved.
- Indexed normal and shadow passes use 1,149,764 vertex bytes and 624,864 index bytes together, avoiding the vertex-buffer overflow encountered with the initial direct-vertex build.
- Solver stress test: 30,000 game ticks across still hanging, fast circular motion, abrupt motion, ground contact and conflicting torso contact. No invalid/infinite positions. Maximum link-length error: 0.00000763 game units. Stationary chain drop equals its rest length, 37.2132 game units.
- Attachment interpolation algebra: 42,000 numerical cases passed. This checks the correction mathematically, not exact closed-ring collision.
- The v0.1.0 native build compiled/linked against the 2.0.3 SDK import library, passed package integrity checks and registered successfully at runtime.
- The delivered build script previously completed a fresh compile and package. Its generated bundle passed AMD64 DLL, archive integrity and byte-for-byte content checks; invalid DLL input was rejected.

Reproducible geometry, interpolation and solver checks/results are included in `tools/`.

## Remaining limits

Mirrors and form-change/teleport reset handling are implemented and source-reviewed, but have not been visually tested in their respective gameplay situations. Full mod-stack compatibility, remote co-op avatars and every area/cutscene remain unverified.

Physics uses approximate torso/floor contact; it does not implement full environment collision or chain self-collision. Inventory chains use a static pose. Sword damage, collision, attack animations and trails remain the original Ordon behavior. Equipment changes complete immediately while visibility and the shield gesture run independently. Inventory icons, text, unrelated sounds and separate pickup/cutscene prop models retain their originals. Cosmetic enemy-hit effects replace only the accepted contact's generic hitmark, leaving native damage logic unchanged. The new light particles are not added to mirror or shadow passes.

Actual KHIII masks, meshes and Cascade parameters are adapted to a different engine. The cooked material graphs omit their complete wiring, so the two-color/brightness formula is inferred from retained parameters and function descriptions rather than recovered exactly. Scene-depth soft-particle fading is still absent; HDR flash gradients are clamped by the host's color range. Fresnel, erosion, lighting and compositing also remain approximations. The user approved the resulting corrected appearance, audio and slower shield movement. Original hit-family cues are used, with variation selection adapted to Dusklight's contact events. No pixel-identical or fully equivalent KHIII rendering claim is made.

The Blender studio preview is not an in-game screenshot; the native metal response approximates the source PBR materials.

## Original files and storage

The original Twilight Princess ISO, original Dusklight installation and original saves were left unchanged. Development testing uses an isolated installation and a copied save. Kingdom Hearts III archives were read in place, with only selected needed assets extracted into the D: workspace. No full game/archive copy was made and no extracted game assets were placed on C:.
