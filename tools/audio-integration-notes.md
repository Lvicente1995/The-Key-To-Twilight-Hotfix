# Native one-shot audio

`src/keyblade_audio.hpp` uses the Dusklight 2.0.3 audio resource service and the
native `JAISe` sound-effect mixer. It adds nine independent effects: summon,
dismissal, and seven impact variants. It does
not replace original waves or consume music-stream slots.

Expected files are mono **16-bit PCM WAV**:

- `res/kingdom_key_summon.wav`
- `res/kingdom_key_dismiss.wav`
- `res/kingdom_key_hit.wav`
- `res/kingdom_key_hit_02.wav`
- `res/kingdom_key_hit_03.wav`
- `res/kingdom_key_hit_04.wav`
- `res/kingdom_key_hit_05.wav`
- `res/kingdom_key_hit_06.wav`
- `res/kingdom_key_hit_07.wav`

Samples are decoded once by the host and retained while the mod is loaded.
The helper marks missing cues unavailable; the integrated mod requires all nine
and reports an initialization error if any cannot load. Runtime audio testing
must use the final samples; the helper's compile/link check alone does not
establish audible volume, timing, pause behavior or mixing quality.

## Integration

Import `AudioResService`, and create one `kingdom::audio::System` instance.
During `mod_initialize`, call:

```cpp
audio.initialize(mod_ctx, svc_audio_res, svc_hook);
```

Declare the sequence preparation hook by name; MSVC's member-pointer metadata
cannot constant-initialize this particular multiply inherited class:

```cpp
DEFINE_HOOK_SYMBOL("JAISe::prepare_getSeqData_", bool(JAISe*), PrepareKeybladeSound);
HookAction onPrepareKeybladeSound(ModContext*, void* args, void* result, void*) {
    if (audio.prepareSequence(mods::arg<JAISe*>(args, 0))) {
        *static_cast<bool*>(result) = true;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
```

Register that PRE callback through `mods::hook::add_pre`. Play a cue once per
simulation event with `audio.play(Cue::Summon, &position)`, `Cue::Dismiss` or
`Cue::Hit` through `Cue::Hit7`. Those seven contiguous impact cues map in order
to the seven hit files above. Pass `nullptr` for a nonpositional sound. Optional
volume and pitch arguments follow the position. Eight owned sound handles bound
simultaneous playback independently of the nine available cues; when all eight
are occupied, the next event stops and reuses one handle.

Use `ready(cue)` before suppressing the matching original sound. The original
draw/stow IDs are `0x20000` and `0x20001`, emitted through both
`daAlink_c::seStartSwordCut` and `seStartOnlyReverb`. Suppression must remain
limited to the eligible Ordon replacement; unrelated weapons and item sounds
must pass through.

Call `stopAll()` on a scene/actor discontinuity. Call `shutdown()` **before**
uninstalling the preparation hook or unloading the DLL.

## Implementation and validation

The resource service allocates fresh wave and sound-effect IDs. An original
eight-byte BMS sequence selects a private instrument bank/program, starts key
60 at velocity 127, waits for sample completion, and ends. A vacant bank slot
between 240 and 255 maps the nine programs to those new wave IDs. The bank
supplies a sentinel handle so the host's existing wave replacement lookup can
resolve newly added IDs before checking the sample metadata. It never installs
new instruments over occupied slots.

The native SFX path supplies game SFX volume, pause state, priority, distance and
panning. Native channels retain host-owned PCM references and a host-owned
oscillator; no DSP callback or envelope points into the mod. Shutdown locks the
same audio mutex used by the native driver, stops sounds, clears sequence
readers, detaches mod-owned handles, and removes only the mod's bank pointer.

The mutex wrapper constructor/destructor are not exported in the 2.0.3 Windows
import library. They are resolved with the public `HookService::resolve` API;
both exact decorated names were verified in the installed executable's
embedded symbol manifest. Failure to resolve them leaves audio unavailable.

Two 2.0.3 service quirks require care: register sound-table additions during
mod initialization, when lifecycle synchronization publishes them; leave their
removal to the host's mod-detach cleanup because the individual SE-table
removal path indexes its ID allocator incorrectly. Wave removal is safe and
explicit.

Validation completed: standalone native-header probe compiled and linked with
MSVC 19.44, the production game ABI definitions, and the installed Dusklight
2.0.3 Windows x64 import library. Only pre-existing SDK boolean-operation
warnings appeared. The nine-cue extension's enum, program indices, arrays and
resource names passed static review. All nine bundled files were checked as
mono 16-bit PCM at 48 kHz. The user confirmed summon, dismissal and enemy-impact
playback in the final gameplay check. Detailed pause, panning and mixing behavior
across other scenes remains unverified. The bundled sounds are selected clips from the user's
KH3 PC files.
