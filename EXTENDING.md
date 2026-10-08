# Adding choices

The menu, requested settings, active features and asset ownership are separate. Keep the mod ID `local.kingdom_key.ordon` so existing configurations continue to load.

## Persistent selections

Append a new integer ID and matching label to `src/keyblade_selection.hpp` or `src/character_selection.hpp`. Never renumber `None = 0` or the existing choice at `1`. Update both the validity check and ID conversion; unknown IDs must still resolve safely to None. The public dropdown uses the option's array index as its persisted ID. Add configuration regression cases for the new value and invalid values.

## Feature lifecycle

Load and validate assets at initialization in `src/customization_runtime.hpp`; keep per-feature errors and owned resources separate. Apply pending choices at `mod_update`, not inside UI callbacks or rendering. Deactivate the previous choice before activating another, and leave the native feature active if activation fails. Do not require a character choice for a weapon, or a weapon choice for a character.

The current Kingdom Key implementation is isolated in `src/kingdom_key_runtime.hpp`. Its existing physics, effects, audio, UI and text modules remain unchanged from v0.2.3g. Add a new weapon's implementation separately and dispatch through the shared hooks; do not silently change Kingdom Key's geometry, sounds or effect basis to fit a new weapon.

The character format, decoder and CPU skinning live in `src/xion_model.hpp`; the initial renderer lives in `src/xion_renderer.hpp`. A future character can reuse the validated 40-joint Link palette and mesh format. Keep converted resources in a character-specific directory, editable files in the sibling model archive, and voice ownership separate from weapon audio. A new palette or format needs explicit validation rather than assuming Xion's layout fits it.

## Shared rendering and sound hooks

`src/customization_rendering.hpp` coordinates native body, equipment, Collection preview, shadows and reflections. Prepare a valid replacement before hiding any native model. Restore temporary native pointers and visibility after each scoped render. Preserve the existing interpolation key allocation and mirror winding convention. Native outfit flags and gameplay systems stay owned by the game.

Character voice routing uses exact human vocal IDs in `src/character_voice_mapping.hpp`. Preserve native handles, positions, routing and sound lifetime, stop only module-owned sequences, and release audio resources before removing hooks. Do not replace the whole voice category: human and wolf IDs are interleaved.

## Build and checks

Keep public SDK services and portable C++20 code. Resources are shared across all targets; the unchanged Actions workflow compiles eight native libraries and merges them into one `.dusk`. A local build verifies only its target. Re-run the relevant configuration, ownership, asset-decoder and model checks after changes, then test live switching, all character/weapon combinations, equipped outfits and unload/reload. Additional characters need their own movement, equipment-grip and facial limitations recorded.
