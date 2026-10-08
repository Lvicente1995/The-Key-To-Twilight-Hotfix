# Xion outfit compatibility: offline evidence

Checked 2026-10-07 against Dusklight source `40457c6adb381928e4b5fef6ed459ed291edd5e2` and extracted user-owned native Link reference models. This report contains **offline validation only**. It does not claim an in-game Ordon, Hero, Zora or Magic Armor test, native animation-clip coverage, or correct cloth appearance in every pose.

| Native outfit | Body model | Face model | Palette comparison |
|---|---|---|---|
| Hero's Clothes | `Kmdl/al.bmd` | `Kmdl/al_face.bmd` | 35 body + 5 face joints match |
| Ordon Clothes | `Bmdl/bl.bmd` | `Bmdl/al_face.bmd` | 35 body + 5 face joints match |
| Zora Armor | `Zmdl/zl.bmd` | `Zmdl/zl_face.bmd` | 35 body + 5 face joints match |
| Magic Armor | `Mmdl/ml.bmd` | `Mmdl/al_face.bmd` | 35 body + 5 face joints match |

The names, parent indices, local rest matrices, global rest matrices and derived inverse bind matrices all match the Hero palette. The native archive selection is independently confirmed by `src/d/actor/d_a_alink_wolf.inc:312` (`changeLink`). Face world transforms attach to native body head joint 4; the fixture reproduces that connection. Hat and hand auxiliary models have different skeleton sizes, but they are suppressed as whole models and are not used as Xion's skinning palette.

The production `xion_model.hpp` decoder and skinning functions processed all 23,110 vertices through four outfit skeletons and 52 synthetic poses each: **4,806,880 skinned vertex samples**. Scenarios cover bind, two walk phases, raised sword/shield arms, crouch, climb, swim orientation, feet/coat joints, head turn, jaw/brows and a world transform, followed by a separate rotation of every joint. These are constructed stress poses, not sampled native BCK animations.

- Maximum bind reconstruction error: **0.00001691 cm**.
- Maximum position or normal difference between outfits: **zero** at float precision.
- Maximum unit-normal length error: **0.000000179**.
- Every tested skinning and normal matrix was finite and invertible.

The native BMD EVP inverse matrices differ slightly from inverse matrices derived from quantized JNT rotations (maximum matrix-element residual about 0.02309). Xion consistently uses the JNT-derived bind basis, as the actual mesh reconstruction test confirms. That residual is not an outfit mismatch.

The corrected coat/hand geometry was retested with SHA-256 `a4851fd1d5cdbed842504fa32a6e11d5553e33d0eee4fdc5e78257cb40dc646a` (1,559,842 bytes). It retains the same vertex/triangle counts and passes all results above. The separate production-loader regression also passed its truncation, transactional failure, invalid index/palette/path, 3,000 affine skinning and padded GX texture-tiling cases. Rerun the mesh test after changing weights or geometry; the skeleton comparison remains independently valid.

## Armor gameplay integration review

Source inspection found no character-selection writes to the native outfit flags, rupees, damage values, oxygen state, animation packs or outfit-resource pointers. Xion captures native body/face matrices and replaces their draw packets. `compatible()` requires the supported joint layout and yields to the original renderer during outfit loading, wolf form or invalid replacement setup (`src/xion_renderer.hpp`). Shared equipment, shadow and mirror handling remains separate (`src/customization_rendering.hpp`).

Native armor decisions remain in the game:

- Zora ability checks use outfit flags in `d_a_alink.cpp:14313`; swim speed and oxygen handling remain in `d_a_alink_swim.inc:32` and `:54`. The Zora damage multiplier remains in `d_a_alink_damage.inc:171`.
- Magic Armor ability checks remain at `d_a_alink.cpp:14317`, heavy-state logic at `:12748`, invulnerability/mode handling at `d_a_alink_damage.inc:292`, and the native rupee-drain loop at `d_a_alink.cpp:18740`.
- The mod attaches pre/post callbacks to `execute`; its pre-callback returns `HOOK_CONTINUE`. It does not replace the native execute, armor, damage, swim or oxygen routines. The existing weapon feature's scoped shield-arm animation layer is separate from character selection.

This supports the conclusion that armor gameplay logic is retained; it is **not a substitute for gameplay tests**. All outfits display the same Xion costume. Original armor surface animations, glow and Zora mask/fins are not reproduced on her textures. Native equipment remains separate, with Xion foot geometry hidden when the game's heavy boots are drawn.

## Reproduce

The skeleton fixture is generated from the existing private `link-skeletons.json` extraction; native game reference matrices are not included with the mod. From the project root, with Python 3 and a C++20 compiler:

```text
python tools/prepare_xion_outfit_test.py --reference <private-link-skeletons.json> --output <work>/outfits-private.bin --report <work>/skeleton-validation.json
c++ -std=c++20 -O2 -Isrc tools/xion_outfit_test.cpp -o <work>/xion_outfit_test
<work>/xion_outfit_test res/characters/xion/xion.mesh <work>/outfits-private.bin <work>/pose-validation.json
```

The Windows MSVC equivalent is `cl /std:c++20 /EHsc /O2 /Isrc tools/xion_outfit_test.cpp /Fe:<work>/xion_outfit_test.exe`. The supplied `xion-outfit-skeleton-validation.json` and `xion-outfit-pose-validation.json` record this run. Keep the generated matrix fixture in local work rather than the distributable archive.
