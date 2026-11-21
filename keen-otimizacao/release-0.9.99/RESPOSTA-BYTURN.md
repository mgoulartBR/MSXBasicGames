# Resposta sugerida ao feedback do byturn (itch.io / msx.org)

> Thank you very much for the detailed feedback — it was really useful. Point by point:
>
> **1. Title screen not shown — confirmed, and fixed in 0.9.99.** Real bug, present since 0.9.76. The HUD-bar interrupts read a flag (`bar_on`) that was never initialised at start-up; with garbage in RAM they rewrote the page/scroll registers every frame and hid the title (I could reproduce it in openMSX: black screen with R#2 = 0x7F). The flag is now cleared before the interrupt hooks are installed. 0.9.62 was not affected.
>
> **2. Keen using all 8 sprites per line.** It is a fair point, but my profile says the cost is elsewhere: hardware sprites take no VDP command time, so drawing Keen with `LMMM`+`TIMP` would *add* a restore and a draw command per frame (the CPU waits for the VDP only 4–9% of the time in most stages). The real benefit would be freeing the sprite planes for other actors. I haven't measured it, so I'll try the bitmap path for Keen on MSX2+ and decide with numbers rather than guessing.
>
> **3. MSX2 reboots when scrolling left — confirmed.** In my MSX2 emulation it hangs: the stack gets corrupted inside the V9938 software-scroll code (a routine that remaps its own bank window while running from it). I couldn't find a cheap fix, and you are right that without hardware horizontal scroll it's not worth it, so 0.9.99 requires an MSX2+ (V9958) and shows a message on an MSX2.
>
> **4. Leftover masks / old two-pass code — yes, there is some.** The bayonet enemy (the potato troopers), the broccoli enemies, the mine carts and the special bitmaps of City, Apple and Castle are still drawn with AND/OR, and several frames in the generic path too (the atlas shares pixels between overlapping rectangles, so converting them in place would corrupt neighbouring frames). The boss and the hero are single-pass. The streamed ones (troopers, broccoli) are the next candidates to convert, using the same method as the boss. On my profile those stages are mostly CPU-bound, so I expect modest gains, but it also frees VRAM, which is what you pointed out.
>
> **5. V9990.** Thanks for the suggestion. I'll keep it in mind, but it would be a separate renderer (different VDP, registers and commands), it needs a Gfx9000/Video9000, and in most stages the profile shows the CPU, not the VDP command speed, is the limit — so I wouldn't expect it to remove the frame drops there. No promise for now.
>
> Thanks again for the time you spent on this!
