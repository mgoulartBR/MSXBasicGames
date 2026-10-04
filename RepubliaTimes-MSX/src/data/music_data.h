#pragma once
#include "msxgl.h"
// Music streams: pairs (midi note, ticks of 1/30 s); note 0 = rest; 0xFF terminates (loop)
typedef struct
{
	const u8* voice[2];
} MusicTrack;
extern const MusicTrack g_MusicTracks[2];
