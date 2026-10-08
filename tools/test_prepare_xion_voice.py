"""Conversion checks with synthetic fixtures; does not require KH3 assets."""
import json
from pathlib import Path
import struct
import tempfile
import unittest
import wave
from contextlib import contextmanager
from prepare_xion_voice import prepare


@contextmanager
def workspace_temporary():
    root = Path.cwd().resolve()
    with tempfile.TemporaryDirectory(prefix="voice-test-", dir=root) as temporary:
        # Verify the resolved cleanup target remains within this workspace.
        assert Path(temporary).resolve().is_relative_to(root)
        yield temporary


class VoiceConversionTest(unittest.TestCase):
    def fixture(self, root, channels, samples):
        decoded, project = root / "decoded", root / "project"
        decoded.mkdir()
        (project / "src").mkdir(parents=True)
        (project / "tools").mkdir()
        # Deliberately unrelated track index: the decoder's cue mapping must
        # win over a tempting guess from the cue's trailing digits.
        (decoded / "TrackUsers.txt").write_text("Battle_Xion_039.hca, users: bt2150060xi0\n")
        with wave.open(str(decoded / "Battle_Xion_039.wav"), "wb") as out:
            out.setnchannels(channels)
            out.setsampwidth(2)
            out.setframerate(48000)
            out.writeframes(struct.pack("<" + "h" * len(samples), *samples))
        selection = root / "selection.json"
        selection.write_text(json.dumps({"clips": [{"cue": "bt2150060xi0",
                              "groups": ["AttackLight", "Jump"], "evidence": "synthetic fixture"}]}))
        return decoded, selection, project

    def test_mono_cue_resolution_and_unmodified_pcm(self):
        with workspace_temporary() as temporary:
            samples = [0, -32768, 32767, -19, 17000, 0]
            decoded, selection, project = self.fixture(Path(temporary), 1, samples)
            result = prepare(decoded, selection, project)
            self.assertEqual((result["files"], result["definitions"]), (1, 2))
            with wave.open(str(project / "res/xion/voice/bt2150060xi0.wav"), "rb") as wav:
                self.assertEqual((wav.getnchannels(), wav.getnframes(), wav.getframerate()), (1, 6, 48000))
                self.assertEqual(wav.readframes(6), struct.pack("<6h", *samples))
            provenance = json.loads((project / "tools/xion-voice-provenance.json").read_text())
            self.assertEqual(provenance["clips"][0]["decoded_track"], "Battle_Xion_039.wav")
            self.assertEqual(provenance["clips"][0]["gain"], 1.0)
            header = (project / "src/character_voice_clips.hpp").read_text()
            self.assertIn('"res/xion/voice/bt2150060xi0.wav",Group::AttackLight', header)
            self.assertIn('"res/xion/voice/bt2150060xi0.wav",Group::Jump', header)

    def test_stereo_downmix_preserves_timing(self):
        with workspace_temporary() as temporary:
            decoded, selection, project = self.fixture(Path(temporary), 2, [30000, 10000, -30000, -10000, -32768, 32767])
            prepare(decoded, selection, project)
            with wave.open(str(project / "res/xion/voice/bt2150060xi0.wav"), "rb") as wav:
                self.assertEqual(wav.getnframes(), 3)
                self.assertEqual(struct.unpack("<3h", wav.readframes(3)), (20000, -20000, 0))

    def test_missing_evidence_is_rejected(self):
        with workspace_temporary() as temporary:
            decoded, selection, project = self.fixture(Path(temporary), 1, [100, 200])
            value = json.loads(selection.read_text())
            del value["clips"][0]["evidence"]
            selection.write_text(json.dumps(value))
            with self.assertRaisesRegex(AssertionError, "evidence"):
                prepare(decoded, selection, project)

    def test_source_volume_is_separate_from_unchanged_pcm(self):
        with workspace_temporary() as temporary:
            decoded, selection, project = self.fixture(Path(temporary), 1, [1000, -2000])
            value = json.loads(selection.read_text())
            value['clips'][0]['source_volume'] = 0.65
            selection.write_text(json.dumps(value))
            prepare(decoded, selection, project)
            with wave.open(str(project / 'res/xion/voice/bt2150060xi0.wav'), 'rb') as wav:
                self.assertEqual(struct.unpack('<2h', wav.readframes(2)), (1000, -2000))
            self.assertIn('0.65000000f', (project / 'src/character_voice_clips.hpp').read_text())
            for invalid in [-1, 0, float('nan'), float('inf'), 5]:
                value['clips'][0]['source_volume'] = invalid
                selection.write_text(json.dumps(value))
                with self.assertRaisesRegex(AssertionError, 'volume'):
                    prepare(decoded, selection, project)

    def test_multiple_banks_keep_battle_and_cutscene_definitions(self):
        with workspace_temporary() as temporary:
            root = Path(temporary)
            decoded, selection, project = self.fixture(root, 1, [100, 200])
            event = root / "Event_Project"
            event.mkdir()
            (event / "TrackUsers.txt").write_text("Event_013.hca, users: kg8720262xi0\n")
            (event / "Event_013.wav").write_bytes((decoded / "Battle_Xion_039.wav").read_bytes())
            value = json.loads(selection.read_text())
            value["clips"].append({"cue": "kg8720262xi0", "groups": ["Gasp"], "evidence": "synthetic event fixture"})
            selection.write_text(json.dumps(value))
            result = prepare([decoded, event], selection, project)
            self.assertEqual((result["files"], result["definitions"]), (2, 3))
            header = (project / "src/character_voice_clips.hpp").read_text()
            self.assertIn("bt2150060xi0.wav", header)
            self.assertIn("kg8720262xi0.wav", header)


if __name__ == "__main__":
    unittest.main()
