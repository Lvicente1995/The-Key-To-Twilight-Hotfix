The character voice module substitutes the sequence used by an existing human Link sound. It keeps the original sound ID, native handle, position, reverb, volume controls, priority and level-sound lifetime. It does not replace dialogue streams, other characters, wolf vocals, music or Kingdom Key sounds.

The explicit list in `src/character_voice_mapping.hpp` contains the 152 `Z2SE_AL_V_*` values in Dusklight v2.0.3 `include/Z2AudioLib/Z2SeMgr.h`. Human and wolf IDs are interleaved, so replacing the whole player-voice category would be incorrect. Cutscene-specific Link reactions are in this same list. Their mapping to Xion performances is an adaptation by vocal intent; it does not claim that KH3 provides a corresponding recording of each TP scene.

Source routing was checked in `Z2LinkMgr.cpp` (`startLinkVoice`, `startLinkVoiceLevel`), `Z2Creature.cpp`, `Z2SoundObject.cpp`, `Z2SoundMgr.cpp`, and `JAISe.cpp`. The shared `JAISe::prepare_getSeqData_` hook catches the final native cue after combat/free-swing/underwater remaps, including animation and cutscene sound routes. It does not bypass those decisions.

Integration:

- Load once with `character_voice::System::initialize(context, audioResources, hooks, kXionClips)` at the normal mod lifecycle boundary. Do not reload samples on every selection change.
- Enable only when the Xion character selection is applied. An audio failure must leave the character renderer usable and native voice playback available.
- The shared sound-prepare hook consults this system and the independent Kingdom Key helper. Their IDs are disjoint. Return true and skip the original prepare function only when a helper succeeds.
- Disable with `setEnabled(false)` when restoring Character None. This stops only sounds whose retained sequence pointer belongs to this module. Missing samples or unmapped groups continue through native playback.
- Call `shutdown()` before native hook removal/unload. It uses the host recursive audio mutex, clears retained sequence readers, releases only its own vacant-slot bank and removes registered samples. Failure to resolve that mutex never traverses the host sound list.

`tools/prepare_xion_voice.py` accepts an AudioMog-decoded project and a reviewed cue selection. `TrackUsers.txt` supplies the authoritative cue-to-recording mapping. The generated provenance records exact source and output hashes, conversion, timing and the evidence used for each selection. Mono PCM samples remain unchanged; stereo is averaged to mono without changing rate, duration, pitch or gain.

No listening claim is made by source classification alone. Runtime voice balance, repeated level sounds and cutscene suitability require a listening check in the game.

The user auditioned four original cutscene samples on 7 October 2026: `kg8720262xi0` breathing, `kg8720263xi0` light grunt, `kg8720279xi0` surprise, and `kg8720280xi0` stronger grunt. These supply Breath, Soft, Gasp and Exertion respectively. Fourteen recordings now cover all ten groups. Native KH3 cue volume is applied separately in the instrument, keeping decoded PCM unchanged and preserving TP's own volume/reverb controls.
