# v0.3.0 stability test

This test patch addresses likely scene-transition / multiplayer coexistence crash paths:

- Xion rendering is restricted to `dComIfGp_getLinkPlayer()` only. Link-compatible remote/puppet actors are never replaced.
- Xion voice substitution is gated by a live local Xion gameplay render packet. Global/remote/scene-load JAISe traffic falls through to native/other-mod handling.
- Xion voice cleanup captures the next audio-list node before stopping a sound and avoids dereferencing a track after `JAISe::stop()`.
- The in-game `mod.json` description is plain text; the README retains release formatting.

This is a runtime diagnostic/stability patch, not yet proof of Crests model synchronization.
