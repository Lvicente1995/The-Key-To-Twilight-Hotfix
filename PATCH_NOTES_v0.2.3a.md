# v0.2.3a — universal build fix

- Removed the dependency on Dusklight's internal `dusk/ui/icon_provider.hpp`; out-of-tree mods only receive public game ABI and SDK include paths.
- Removed the optional touch-control icon hook. Collection/pause and standard in-game HUD icon replacement remain.
- Changed the requested English item name to **Kingdom Keyblade**.
- Preserves the v0.2.2a universal audio/gameplay code and the eight-platform workflow.
