# The Key to Twilight — v0.3.0

## Description

**Bring the power of Kingdom Hearts into The Legend of Zelda: Twilight Princess with this crossover mod for Dusklight!**

This mod introduces customizable Keyblades and playable Kingdom Hearts characters, allowing players to experience Twilight Princess in a whole new way.

**Features:**

- **Keyblade Selection:** Replace Link's weapons with iconic Keyblades from the Kingdom Hearts series, starting with the Kingdom Key.
- **Character Selection:** Play as characters from Kingdom Hearts, starting with Xion from Kingdom Hearts III.
- **Outfit Compatibility:** Character replacements support Link's Ordon Clothes, Hero's Clothes, Zora Armor, and Magic Armor.
- **Custom Visuals:** Enjoy updated weapon icons, descriptions, and special summoning effects inspired by Kingdom Hearts.
- **Optional Replacements:** Choose "None" in the mod settings to restore Link's original appearance or the default Ordon Sword.

**Planned Features:**

- Additional playable characters and Keyblades.
- Kingdom Hearts-inspired combat abilities.
- Magic attacks such as Fire, Blizzard, Thunder, and Cure.
- Additional visual effects and customization options.

**The worlds of light and twilight collide!**

*Note: This is a fan-made crossover mod and is not affiliated with or endorsed by Nintendo, Square Enix, or Disney.*

Expandable customization for **Dusklight 2.0.3**, based directly on the user's known-good **v0.2.3g** source. Independent persistent Keyblade and Character controls preserve the existing Kingdom Key implementation and add Xion's visual model and original English KH3 vocal reactions. The eight-platform universal workflow and mod ID `local.kingdom_key.ordon` are retained.

## Select your appearance

Open Dusklight's **Mods → The Key to Twilight** panel.

| Setting | None | Replacement |
| --- | --- | --- |
| Keyblade | Native Ordon Sword, icons, text, sounds and behavior | Kingdom Key with the complete v0.2.3g feature set |
| Character | Native Link and voice | Xion with Link's native animations and Xion vocal reactions |

The settings are independent: Link + Ordon Sword, Link + Kingdom Key, Xion + Ordon Sword and Xion + Kingdom Key are supported combinations. Choices are saved across restarts and apply at the next mod update. Defaults are Kingdom Key and normal Link. Repairing or changing asset files requires a mod reload; changing these choices does not.

Xion replaces the visual human body for Ordon Clothes, Hero's Clothes, Zora Armor and Magic Armor. Native armor abilities remain active, including Zora swimming and Magic Armor behavior. Wolf form remains native. The character renderer does not change movement, camera, collisions, combat, interactions or the native animation controller. A character asset failure leaves Link visible and reports the problem in the mod panel.

Fourteen original KH3 English recordings cover attacks, jumps/effort, damage, defeat, breathing, gasps and softer reactions. Character None restores Link's voice. An explicit list targets human Link vocal cues, including nonverbal cutscene reactions; it preserves dialogue streams and other characters. KH3 performances are adapted by intent rather than matched to every individual TP scene. Source evidence and the user's four-clip audition are recorded in `tools/xion-voice-selection.json`.

## Install

Build the project with the included GitHub Actions workflow and download the `mod-combined` artifact. Inside is the universal `kingdom_key.dusk`; install that single file through Dusklight's mod manager/data-folder `mods` directory. Do not extract the `.dusk`. Remove older Kingdom Key versions so only one package with this mod ID is installed.

The universal bundle is intended to contain native libraries for every target in Dusklight's official mod-template matrix while sharing one copy of `mod.json` and `res/`.

## Appearance and movement

- The button icon uses the user-approved Kingdom Key artwork; the Collection icon mirrors that same artwork horizontally, matching the requested menu orientation. Shading and outlines follow the original Twilight Princess sword icon.
- The English equipment title and sword-acquisition message say **Kingdom Key**. Existing formatting, controller glyphs and other languages are preserved.

- Original full-detail Kingdom Key geometry: 52,072 triangles, silver shaft and crown teeth, gold guard, blue neck, black grip, and Mickey charm.
- Grip aligned to the Ordon Sword. Tip reaches 99.37 game units versus the original 99.67, preserving the original attack reach.
- Fourteen individual chain links plus a heavier terminal charm move under gravity and inertia. Motion reacts to the weapon rather than repeating a baked animation.
- Chain constraints retain their length during fast motion. Basic torso and floor contacts reduce clipping. Motion resets safely when changing form, drawing/stowing, or teleporting.
- The Keyblade materializes in Link's hand and vanishes when put away. Its body, chain and charm leave together; no weapon, scabbard or weapon shadow remains on Link's back.
- Summoning, dismissal and enemy-hit effects use selected Kingdom Hearts III textures, effect meshes and Cascade particle settings from the user's installed copy, adapted to Dusklight's renderer. These replace the earlier procedural approximation.
- Authentic Kingdom Key audio accompanies appearing, disappearing and confirmed enemy contacts. KHIII references the same appearance effect and sound cue `se02001_010` for both appearing and disappearing. Seven original hit-family cues provide impact variations.
- Link's sword arm no longer performs the normal reach-behind draw, sheath or victory-flourish gesture. An independent animation preserves the shield arm's native movement and transfers the shield at its hand/back contact frame. It plays at 75% native speed with a five-tick settling blend, following feedback that the initial version moved too quickly. Scripted event animations retain their native timing.
- Metal highlights use source metalness and roughness with the game's sunlight, room lights and fog. Brightness is balanced for the supplied solid materials.
- Native frame interpolation, inventory-model rendering, custom shadow geometry, and mirror rendering are implemented.

## Current scope

The delivered revision retains occasional lower-leg/rear-coat clipping during running. Further visual polishing was stopped at the user's request. Fixed finger poses, incomplete facial animation and untested gameplay/platform cases are documented below and in `VALIDATION_v0.3.0.md`.

Version 0.3.0 adds selectors, character rendering and character voices to v0.2.3g. Existing weapon gameplay, physics, sound and particle behavior are preserved. The user's earlier successful full matrix build and Android runtime do not automatically validate this revision. See `VALIDATION_v0.3.0.md` for current checks; `VALIDATION.md` retains historical results.

Sword damage, hitboxes, attack animations and attack trails retain Ordon behavior. Native equipment changes complete immediately while the cosmetic weapon transition and shield gesture run independently. The Keyblade hit effect/audio trigger is limited to confirmed, nonblocked enemy contacts and deduplicated per target per game tick. It replaces the native generic hitmark only for those accepted contacts; collision, damage and enemy reactions remain unchanged. Original draw/sheath sounds are suppressed when the replacement cue is available. The v0.2.3g English name, custom pause description and "dismiss" wording remain active only for Kingdom Key. Other translations and separate pickup/cutscene prop models retain their originals.

The inventory preview uses a static chain pose. Physics uses approximate torso/floor contact rather than full environment or chain self-collision. Compatibility with other sword replacers, remote co-op avatars and every gameplay situation has not been established.

The Blender preview is a studio render, not an in-game screenshot. The game's metal lighting approximates the source material. KHIII particle timing, masks, mesh geometry and vertex color/alpha gradients are retained where supported. The source effect basis (+X along the blade, +Z toward the teeth) is aligned to the replacement with a 1.26 scale. Engine-specific material effects such as Fresnel, erosion, lighting and compositing remain approximations; this is not a claim of pixel-identical KHIII rendering.

## Asset sources and storage

The weapon model comes from the supplied `kingdom-key.zip`. The new UI icon was generated from the user's approved Kingdom Key reference and the original game icon style; its source and exact prompt are documented in `ARTWORK.md`. Selected sounds and visual-effect assets come from the user's local Kingdom Hearts III installation. Its archives were read in place; only the needed files were extracted into the D: workspace. No game or full archive was copied, and no extracted game assets were placed on C:.

The original Twilight Princess ISO, original Dusklight installation and original saves were not modified during development. Testing uses an isolated Dusklight copy and a copied save. Installing this mod adds a package to Dusklight's mod directory; it does not patch either game's files.

## Editable source

See `EXTENDING.md` for adding future choices without changing existing persistent IDs or coupling character and weapon behavior.

`../model/kingdom-key-rigged.blend` contains the original detailed weapon mesh with separate bones for all fourteen links and the charm. `src/mod.cpp` registers hooks; `src/kingdom_key_runtime.hpp` retains the weapon integration. The existing chain, presence, shield, audio, FX, UI and text modules and runtime assets remain intact. `src/customization_config.hpp` owns public persistent controls, `src/customization_runtime.hpp` applies independent choices and fallbacks, and `src/customization_rendering.hpp` combines the render passes. `src/xion_model.hpp` validates and skins the character asset; `src/xion_renderer.hpp` integrates it with native lighting, preview, shadows and mirrors. `src/character_voice.hpp` owns separate native audio resources and `src/character_voice_mapping.hpp` lists the supported human vocal IDs.

`../model/characters/xion/Xion-KH3-Link-Retarget.blend` contains the editable Xion retarget and a hidden archive of the original full-detail KH3 mesh and 396-bone rig. The runtime uses Link's 35 body joints and five facial joints, with an authored LOD2 body/hair, full-detail face/eyes and simplified zipper. Its README explains Blender export, reproducible JSON retargeting and texture rebuilding; none of those rebuilds requires the original KH3 installation. The runtime package contains only the converted mesh, textures and sounds, not the Blender project.

Xion's jaw and brow weights follow the available native facial joints. Full KH3 facial animation, lip-sync and blinking are not implemented. Her fingers have a fixed closed grip; native Link hand-mesh swaps do not change her finger pose. The long coat preserves its separate front flaps and follows a waist/thigh/shin blend, with limited ankle influence for boot clearance; it does not use KH3 cloth simulation. Extreme poses may require additional weight cleanup. The eye pupil is a documented static approximation of KH3's procedural pupil shader. See the model README and revision validation report for the current visual checks and limits.

`tools/kingdom-key-mesh.json` and `tools/export_mesh.py` regenerate `res/kingdom_key.mesh` with Python 3. `tools/convert_kh3_fx_meshes.py` and its validation report document conversion of the selected KHIII effect geometry.

## Cross-platform support

This source tree follows Dusklight's official mod-template matrix: Windows AMD64/ARM64, Linux x86_64/aarch64, macOS arm64/x86_64, iOS arm64, and Android aarch64. See `CROSS_PLATFORM.md` for details. GitHub Actions builds the platform libraries and merges them into one multi-platform `.dusk`.

## Build

Use this `KingdomKey` directory as the repository root when uploading the editable source to GitHub; the sibling `model` directory is the editable asset archive and is not required by the runtime build.

The recommended distribution build is `.github/workflows/build.yml`. It retains the supplied baseline's official mod-template matrix and creates per-platform artifacts for:

- Windows AMD64 and ARM64
- Linux x86_64 and aarch64
- macOS Apple Silicon and Intel
- iOS arm64
- Android aarch64

When every matrix job succeeds, `Combine bundles` creates the `mod-combined` artifact containing one universal `kingdom_key.dusk`. The workflow also supports manual runs through `workflow_dispatch`.

The project pins the Dusklight 2.0.3 SDK commit `40457c6adb381928e4b5fef6ed459ed291edd5e2`. Dusklight 2.0.2 is not supported by this source. A local build can be made with:

```sh
cmake -B build
cmake --build build --parallel
```

A local build only produces a package for the current host/target; use the Actions matrix for distribution. `.gitattributes` and the LF-normalized configured `mod.json` prevent cross-platform metadata byte mismatches during bundle merging.

The `tools/` folder contains the existing physics, geometry, interpolation, presence, shield-animation, and KHIII effect regression checks. The v0.2.3 UI and message checks are in `tools/ui_regression.cpp` and `tools/keyblade_text_test.cpp`; successful CI compilation does not by itself prove runtime audio behavior on every device, so test the combined bundle on representative hardware.

Official references: [Dusklight mod template](https://github.com/TwilitRealm/mod-template), [v2.0.3 modding API](https://github.com/TwilitRealm/dusklight/blob/v2.0.3/docs/modding.md).
