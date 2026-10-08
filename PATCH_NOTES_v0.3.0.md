# v0.3.0 — The Key to Twilight

Based directly on the supplied `Kingdom-Key-Ordon-v0.2.3g-universal-source.zip`. The mod ID remains `local.kingdom_key.ordon`; Dusklight 2.0.3 is required.

The display name is now **The Key to Twilight**. Keeping the existing mod ID preserves saved selections and update compatibility.

- Adds independent, persistent **Keyblade** (None / Kingdom Key) and **Character** (None / Xion) controls using the public ConfigService and UiService APIs.
- Selecting Keyblade None removes the replacement and restores native Ordon Sword rendering, UI, text, draw/sheath behavior and audio. Kingdom Key activates the complete v0.2.3g weapon implementation.
- Adds a visual character renderer driven by Link's native body/face matrices. Armor abilities, collisions, movement, camera, combat and interactions remain in the game. The four human outfits share the supported skeleton; wolf form remains native.
- Adds original English Xion vocal recordings from the user's KH3 installation. An explicit map substitutes human Link gameplay and cutscene reaction cues while preserving their native sound handles and routing. Dialogue streams, other characters and wolf vocals are unchanged. The voice choice follows Character independently of Keyblade.
- Missing character assets retain Link; missing required weapon assets retain Ordon Sword. Missing custom audio retains native audio. Status messages explain an asset fallback.
- Moves the existing weapon integration into `src/kingdom_key_runtime.hpp` and separates selection, lifecycle, character rendering and character audio modules.
- Preserves the baseline universal build matrix, SDK pin, merge helper, existing runtime resources and portable weapon audio header.
- Includes the editable KH3 Xion archive, Link-compatible retarget, portable conversion and texture tools, and runtime assets. The long coat preserves its front opening; separate flap identity, leg-driven hem weights and corrected finger hinge rotations address the first playtest's coat and grip defects. Validation and remaining limitations are recorded separately.

Selections apply at the next mod update, without reloading the game. Asset files are loaded at mod initialization: changing/replacing an asset or repairing missing files requires reloading the mod. Defaults are Kingdom Key and normal Link. Existing user configuration is preserved by the same mod ID.

This file describes implementation, not a claim that every animation/platform has been tested. See `VALIDATION_v0.3.0.md` for revision-specific evidence and remaining checks.
