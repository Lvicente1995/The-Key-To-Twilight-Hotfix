# Cross-platform build notes (v0.3.0)

The eight-platform workflow, SDK pin, line-ending rules, merge helper and Kingdom Key audio implementation are preserved from the supplied v0.2.3g baseline. The new selectors use public ConfigService/UiService; character rendering uses native portable graphics APIs and CPU skinning. Xion audio uses the same host audio-lock strategy in an independent bank, without Windows audio APIs. Asset files and generated voice catalogues are shared identically by every target.

This revision has been compiled locally for Windows x64. Its full Actions matrix and non-Windows runtime still require validation; prior Android evidence concerns the earlier source. See `VALIDATION_v0.3.0.md`.

This source follows Dusklight's current official mod-template target matrix:

- Windows AMD64
- Windows ARM64
- Linux x86_64 (including Steam Deck)
- Linux aarch64
- macOS Apple Silicon (arm64)
- macOS Intel (x86_64)
- iOS arm64
- Android aarch64 / arm64-v8a

## Portable custom audio

Version 0.2.2 removes the old Windows-only custom-audio gate. Dusklight 2.0.3 implements `JASCriticalSection` as the guard for its recursive host audio mutex on native targets. The mod resolves that host constructor/destructor through `HookService` using platform-independent display names first, then exact MSVC or Itanium C++ ABI names only as a fallback.

This keeps the Kingdom Key summon, dismiss and seven hit-family WAV cues synchronized with Dusklight's own JAS audio thread on supported targets. The audio resource registration itself continues to use `AudioResService`.

If the host audio lock cannot be resolved or an audio sample cannot be registered after the required sound hooks attach, custom audio falls back to native sword sounds. Failure to attach a required hook still fails mod initialization, as in the supplied baseline.

## GitHub Actions

`.github/workflows/build.yml` mirrors the official Dusklight mod-template platform matrix and merges successful per-platform bundles into one `mod-combined` artifact. It also includes `workflow_dispatch` so a build can be started manually from GitHub Actions.

The final combined `.dusk` should contain native libraries for each target under its `lib/` platform directory while sharing one copy of `res/` and `mod.json`.

`.gitattributes` and CMake's LF-normalized `mod.json` copy prevent the Windows CRLF mismatch that can otherwise make bundle merging fail.

## Notes

A GitHub Actions matrix build proves that the source compiles and packages for a target; actual runtime behavior still needs device testing, especially custom audio and KHIII visual effects. GPU/backend behavior can vary by device and driver.

## v0.2.3 UI and name changes

The portable audio header, Actions workflow, SDK pin, line-ending rules and merge helper are byte-for-byte unchanged from the user's v0.2.2a source. UI artwork uses TextureService and the built-in mod image provider for touch buttons. The Collection pane is mirrored only while its menu draws; the standard HUD sword rotation is temporarily removed for the Ordon icon only. The English name uses MessageService callbacks that retain control tags, plus a bounded hook on the exported menu string helper, which bypasses MessageService. There are no new platform-specific libraries or runtime file paths.

The supplied baseline passed all eight builds and worked on Android according to the user. Re-run the included Actions workflow for this revision to produce an updated universal `.dusk`; the separately supplied Windows x64 preview is only a local test build.
