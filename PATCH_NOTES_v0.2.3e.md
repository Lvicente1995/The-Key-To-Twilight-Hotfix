# v0.2.3e — Summon ring orientation fix

- Keeps the thin `hrng000` summon ring in the Kingdom Key's local orientation instead of forcing it into a generic camera-facing billboard.
- The ring mesh lies in its local YZ plane, so its +X normal now remains aligned with the Keyblade blade axis.
- Main glow, long ribbon (`c_cntglw0`), audio, UI, chain physics, hit effects, and gameplay behavior are unchanged.
