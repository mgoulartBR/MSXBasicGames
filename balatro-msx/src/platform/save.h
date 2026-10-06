// Run save in the cartridge SRAM of an ASCII8 + SRAM mapper (openMSX romtype ASCII8SRAM2, Koei-style carts).
// The SRAM is mapped into bank 3 (0xA000) by writing a segment number with the "SRAM enable" bit to the bank-3 register;
// that bit is probed at start-up. On a plain ROM cartridge the probe fails and every call is a harmless no-op.
#pragma once
#include "gtypes.h"

typedef struct { u8 screen, ante, deck, stake; i16 money; } SaveInfo;

void Save_Init(void);                       // probe for SRAM
bool Save_Available(void);                  // SRAM present
bool Save_Peek(SaveInfo* info);             // a valid save exists (fills a short summary)
bool Save_Load(u8* screen);                 // restore the run into `g`; screen = SC_* to resume on
void Save_Write(u8 screen);                 // snapshot `g`
void Save_Erase(void);                      // run finished: forget the save
u16  Save_Sum(void);                        // checksum of `g` (change detection for the autosave)
