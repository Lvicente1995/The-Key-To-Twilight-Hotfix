# Xion crash diagnostic — voice OFF

This is not a release build.

Purpose:
- Character = Xion still enables the Xion visual renderer.
- Xion voice substitution is forcibly disabled.
- Kingdom Key behavior is unchanged.

Interpretation:
- If this build still crashes with Character = Xion, focus on the Xion renderer/model suppression/mirror/shadow path.
- If this build becomes stable with Character = Xion, focus on the character voice runtime/hook.
- Character = None remains the native-Link control case.

The Dusklight log should contain:
`The Key to Twilight v0.3.0 DIAGNOSTIC: Xion renderer active; Xion voice substitution forced OFF.`
