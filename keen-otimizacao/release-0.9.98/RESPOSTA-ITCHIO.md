# Resposta sugerida ao comentário do byturn (itch.io), para publicar junto com a 0.9.98

> Hi byturn, and thanks again — your report was the starting point for all of this, and 0.9.98 is the follow-up I promised.
>
> **AND/OR → TIMP.** You were right. I measured it first with a proof-of-concept build that only swapped the two passes for a single `LMMM`+`TIMP`: VDP busy time dropped from 405 ms to 179 ms (−56%) in an actor-heavy scene (it does nothing for background restore/scroll cost, which is where the other bottleneck was). The hero, the Hill/Melon/Bridge atlases, many frames of the other stages and, in 0.9.98, the Boobus boss are now drawn in one pass, and the freed mask area stays free. The boss alone went from ~4.4 to ~6.2 fps (openMSX, 5.37 MHz) just from halving its pixels and its texture upload.
>
> **Hardware sprites.** I kept them where they already existed (Keen, carrot, tomato). For projectiles and other actors the 8-sprites-per-scanline limit is the problem: Keen alone uses 8 on the same line in 54 of 56 frames, so there is no budget left on most stages.
>
> **Where the rest of the time went.** Profiling showed that in most stages the CPU, not the VDP, was the bottleneck (the CPU waits for the VDP only 4–9% of the time there), so most of the gain came from rewriting the hot loops in Z80 (scroll ring, actor/item lists, keys, bombs, carts…), moving the page flip into the vblank interrupt and fixing the HUD as a bar at the bottom. Numbers at 5.37 MHz (openMSX, controlled-position script, 0.9.75 → 0.9.98): Hill 9 → 15 fps, Grove 3.7 → 17, Apple 6.7 → 12, Boobus 4.6 → 7.2, Castle 6.3 → 11; walking, Hill reaches ~40 fps.
>
> **Caveats.** Everything was measured in openMSX only — I have not tested on real hardware, and I'd really appreciate tests on a real MSX2+/turbo R. Keen's physics now steps at 35 Hz. TIMP coverage is still partial, but in the remaining stages the profile says the gain would be small.
>
> Thanks again for the constructive feedback!
