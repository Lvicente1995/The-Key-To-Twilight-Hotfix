# v0.2.2 patch notes

- Restores the custom Kingdom Key summon, dismiss and seven hit-family sound cues on non-Windows targets by resolving Dusklight's host `JASCriticalSection` through `HookService`.
- Uses platform-independent display names first, with MSVC and Itanium ABI fallbacks for constructor/destructor symbols.
- Audio initialization now fails soft to native sword audio instead of failing the entire mod.
- Expands GitHub Actions to the official Dusklight mod-template targets: Windows AMD64/ARM64, Linux x86_64/aarch64, macOS arm64/x86_64, iOS arm64, Android aarch64.
- Adds manual Actions triggering (`workflow_dispatch`).
- Normalizes `mod.json` to LF at configure time and adds `.gitattributes` to prevent cross-platform bundle byte mismatches.
- Updates project/package metadata to 0.2.2.
