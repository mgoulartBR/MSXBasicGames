// Platform layer: PSG sound effects + music player (driven once per frame).
#pragma once
#include "msxgl.h"

enum
{
	SFX_NONE = 0,
	SFX_CLICK,   // button / selection
	SFX_DRAG,    // pick an article up
	SFX_DROP,    // drop an article
	SFX_FEED,    // a new news item arrived
	SFX_ALARM,   // 75% of the day
	SFX_DAYOVER, // day over
	SFX_ERROR,   // invalid placement
};
enum { MUSIC_NONE = 0, MUSIC_MORNING, MUSIC_NIGHT };

void Audio_Init(void);
void Audio_Update(void); // called by the VBlank hook (ISR) once per frame; the only PSG access
void Sfx_Play(u8 id);
void Music_Play(u8 id);
void Audio_Mute(bool mute);
bool Audio_IsMuted(void);
