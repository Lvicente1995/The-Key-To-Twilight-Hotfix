# v0.3.0 validation — delivered working revision

Target: Dusklight 2.0.3, Windows x64, D3D12, isolated installation and copied save. Original game installations, archives and saves are unchanged.

The user requested an end to further visual polishing and playtesting because of cost after reporting that the corrected model looks a little better. This package preserves that working revision; it does not claim that all requested animation and platform checks passed.

## Completed checks

- Actual pinned SDK: Windows x64 compile, link and `.dusk` packaging pass with selectors, renderer integration and the independent character voice module.
- Public mod-menu dropdowns display None / Kingdom Key and None / Xion. Their values save independently in the isolated test configuration.
- Deliberately missing Xion model: selecting Xion produces a clear fallback status and native Link remains visible. The weapon selector remains functional.
- Live Ordon Sword restoration with Keyblade None: native held model and native HUD icon observed. Switching back to Kingdom Key immediately restores its held model and correctly oriented HUD icon.
- 43 required baseline files are hash-identical: every existing runtime resource, weapon audio, chain/presence/shield/FX/UI/text headers, universal workflow, SDK fetch helper, merge helper and line-ending rules. `tools/v030-baseline-preservation.json` records the inventory.
- Presence regression, KH3 FX regression (8,208,016 assertions, including corrected pitch/spiral direction), effect payload budget, UI ownership/restoration, text (483 checks), persistent configuration, exact human voice ID mapping and Xion mesh parsing/skinning tests pass.
- The supplied baseline's UI test referenced a removed pre-v0.2.3g touch-source method. The test was updated to exercise the actual baseline public texture registration and pane restoration. Production icon code remains unchanged.
- Five voice-conversion fixture tests pass, including separate source-volume handling without altering PCM. Fourteen original English recordings cover all ten vocal groups. Source track mapping, PCM hashes, duration and sample formats are recorded; the user auditioned the four cutscene reactions and identified breathing, light grunt, surprise and stronger grunt.
- Actual KH3 Xion mesh and all textures load in the isolated game. Xion replaces Hero's Clothes, with the Kingdom Key, shield, custom HUD icon, summon effect and character shadow visible. The current outdoor scenes display 60 FPS after loading; this is an observation, not a performance guarantee for other scenes or devices.
- Two distinct winding issues were corrected during visual review: the source conversion's redundant reversal, and the renderer's GX clockwise-front convention. Blender now uses outward mesh winding; the native renderer culls the appropriate opposite side, including the mirror reversal. Xion's face and eyes are now intact in game.
- Repeated mod reloads with Xion and voice resources loaded complete without a crash in the test session. None-to-Kingdom-Key weapon switching was also verified before Xion assets were added.
- Xion is visible during the native hawk-grass item interaction close-up, with the grass held near her mouth and the native Master Sword/shield on her back. This validates one item interaction pose, not every item or scripted cutscene.
- Editable Blender export passes the native mesh loader. Expanded triangles match the JSON runtime export: positions within 0.000008 game units, exact UVs and material references, maximum weight difference below 0.00000003. Corner splits change the editable export's vertex count to 23,516 without changing its 28,370 triangles.
- Portable JSON retarget/export reproduces the runtime mesh exactly. The material rebuild reproduces all eleven textures exactly, including the reviewed static pupil approximation. Source geometry, textures, bind references and editable Blender model are packaged separately from the runtime `.dusk`.
- The user tried Hero's Clothes, Zora Armor and Magic Armor with Xion: no body parts vanished. This exposed coat and hand-pose issues addressed in the later revision below; it is not a complete visual-quality pass. Ordon Clothes has offline skeleton/skinning coverage only.
- During the live movement/combat test, the user confirmed that Xion's voice replaces Link's and sounds right at the current volume. With both selectors subsequently set to None, the user confirmed Link's voice and ordinary sword draw/sheath sounds returned. Scene-specific cutscene reactions still need runtime verification.
- Live Character Xion -> None restores Hero's Clothes, face, hair/hat and native hands while Kingdom Key remains selected. Independently changing Keyblade to None restores the native held Ordon Sword, scabbard and HUD icon without a duplicate character or weapon model.

## Coat and hand corrections

- The initial front-coat defect was traced to cloth-side classification by vertex position. Original left/right flap identity is now preserved, with a smooth opening and separate zipper edges. The corrected front opening and visible legs were observed in the live Collection preview after reload.
- Long-coat weights now use waist/thigh/shin motion with limited ankle influence instead of Link's short-tunic chains. The final mesh has zero exact foot-weighted boot/lower-coat triangle intersections in 192 captured walk/run/roll/attack poses. This excludes shin/upper-boot vertices without ankle weights. A separate check found actual shin/upper-boot overlap with the back coat in running pose 4874: occasional lower-leg/coat clipping remains.
- Finger hinges now curl into both palms around the grip instead of bending sideways. Closed-hand dimensions and palm channels were compared with native closed hands and attachment geometry. The user reported that the combined revision looks a little better; an explicit complete grip-quality pass was not obtained.
- The temporary pose recorder was removed from the isolated game by replacing its bundle with the normal release build. It was never part of the editable release source. The final runtime mesh SHA-256 is `a4851fd1d5cdbed842504fa32a6e11d5553e33d0eee4fdc5e78257cb40dc646a`.
- The final mesh passes all four outfit skeleton/skinning checks: 4,806,880 synthetic skinned samples, maximum bind error 0.00001691 cm, and zero inter-outfit position/normal difference. These are offline checks; see `tools/XION_OUTFIT_VALIDATION.md`.
- With Collection left open, switching Keyblade in both directions updates the held model, sword icon, title, full description and "sheathe"/"dismiss" wording immediately while Xion remains visible. Character and weapon switching have shown all four combinations without duplicate models. The new mod display name is visible after reload.

## Remaining coverage and limitations

- Broader Xion animation and visual checks in game.
- Full gameplay coverage of all four outfits and character/weapon combinations, beyond the switching and outfit checks above.
- Further outfit restoration, scene-specific cutscene reactions and repeated/level sounds.
- Scene-specific voice suitability, facial motion and equipment grip quality beyond the checks above.
- Running, rolling, swimming, climbing, horseback, combat/shield/equipment alignment, shadows/reflections and practical cutscenes.
- Eight-platform Actions build and representative device testing for this revision. Prior user-reported universal build/Android success concerns the earlier baseline, not v0.3.0.

Historical `VALIDATION.md` is retained as historical evidence only. It does not establish completion of these new checks.
