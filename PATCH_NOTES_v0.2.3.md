# v0.2.3 — Kingdom Key icons and name

Based on the supplied v0.2.2a universal source, targeting Dusklight 2.0.3.

- Replaces the Ordon Sword Collection/pause icon with the approved Kingdom Key artwork, mirrored horizontally for that menu.
- Uses the approved artwork directly for the standard B-button HUD and touch button UI. The native HUD's extra sword rotation is removed only while drawing the Keyblade icon.
- Changes the English equipment name and named acquisition message to **Kingdom Key**, preserving existing formatting and controller glyphs.
- Preserves the baseline weapon model, chain physics, effects, sounds, shield motion, SDK pin and eight-platform workflow.

The menu title bypasses the message processor, so its native string helper receives a bounded post-hook. Processed messages use MessageService. UI transformations are temporary and restore their original pane state after drawing. No original game archive or save is modified.

The universal source ZIP is ready for the included GitHub Actions build. The separate Windows x64 `.dusk` is a local preview, not an eight-platform bundle. See `VALIDATION.md` for completed tests.
