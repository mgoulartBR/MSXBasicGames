// PSG sound: a tiny music sequencer (channels A/B) and sound effects on channel C + noise.
#pragma once
#include "gtypes.h"
enum { SFX_NONE, SFX_SELECT, SFX_DISCARD, SFX_USE, SFX_SELL, SFX_COIN, SFX_CASHOUT, SFX_BUY, SFX_REROLL, SFX_PICK,
       SFX_CHIPS, SFX_MULT, SFX_XMULT, SFX_WIN, SFX_LOSE, SFX_ERROR, SFX_COUNT };
void Snd_Init(u8 hz);
void Snd_Update(void);                 // call once per frame
void Snd_Play(u8 sfx);
void Snd_PlayPitch(u8 sfx, u8 step);   // chips/mult blips rise with the event number (like the original)
void Snd_Music(bool on);
bool Snd_MusicOn(void);
