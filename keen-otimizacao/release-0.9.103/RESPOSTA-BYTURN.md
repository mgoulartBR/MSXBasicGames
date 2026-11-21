# Resposta sugerida ao byturn (comentário no itch.io / msx.org) — versão 0.9.103

> Thanks again for the feedback on 0.9.98 — 0.9.103 is out, and here is where each of your points ended up:
>
> **1. Title screen not shown — fixed (0.9.99).** The HUD-bar interrupts read a flag that was never initialised, so random RAM contents made them rewrite the page/scroll registers every frame. Cleared before the hooks are installed. It's in 0.9.103 too.
>
> **3. MSX2 hangs when scrolling left — handled by requiring MSX2+ (0.9.99).** As you said, without hardware horizontal scroll it isn't worth it. The ROM now shows a message and stops on a plain MSX2.
>
> **4. Leftover two-pass code — the streamed enemies are now one pass (0.9.100).** The bayonet troopers, Aspar/Mouth and the broccoli enemies are `LMMM` + `TIMP`, same method as the boss: half the texture bytes and half the VDP commands per draw. I checked the converted data against the old mask/colour pairs (the generated tables came out byte-identical) and looked at the frames in-game. The fps gain is small, as expected — in those stages the CPU is the limit, not the VDP — so the benefit is mostly the freed VPF/VDP time and bytes. Still AND/OR: the carts and the special bitmaps of City, Apple and Castle, plus a few frames of the generic path whose mask/colour rectangles overlap in the atlas.
>
> **2. Keen as a bitmap instead of hardware sprites — not measured yet.** Still on my list; I'd rather decide with numbers than guess. What I did measure is where the time goes now: in most stages it's CPU (scroll, actor/item scans, draw-record collection), which is where the rest of the release went — City +30% and Apple +21% at 5.37 MHz (openMSX), with the other stages between −3% and +10%.
>
> **5. V9990 — no change.** Still a separate renderer for a different VDP; with the CPU as the limit in most stages I wouldn't expect it to remove the frame drops. No promise for now.
>
> Everything is measured in openMSX only — I haven't tried real hardware, so reports from a real MSX2+ or turbo R would be very welcome. Charts and the full list of changes are in the devlog.
>
> Thanks for pushing on this one!
