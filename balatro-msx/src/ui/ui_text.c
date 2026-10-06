// UI strings that are shared between mapper code segments. Code segments share bank 2, so a pointer to a literal
// stored in one segment is NOT readable from another. These live in the bank-3 data segment (SEG20), which is
// always mapped, and are referenced through ui.h.
#include "gtypes.h"
#include "bgame.h"

const char T_PLAY[]    = "Play";
const char T_START[]   = "Start";
const char T_CONTINUE[] = "Continue";
const char T_NEWRUN[]  = "New Run";
const char T_DISCARD[] = "Discard";
const char T_RANK[]    = "Rank";
const char T_SUIT[]    = "Suit";
const char T_RUNINFO[] = "Run Info";
const char T_SELECT[]  = "Select";
const char T_BACK[]    = "Back";
const char T_NEXT[]    = "Next Round";
const char T_USE[]     = "Use";
const char T_SKIP[]    = "Skip";
const char T_SKIPBLIND[] = "Skip Blind";
const char M_NEEDCARDS[] = "Select the cards to use it on first";
const char M_CANTUSE[]   = "Can't use that right now";
const char M_NOMONEY[]   = "Not enough money";
const char M_NOROOM[]    = "No room for it";
const char M_SOLDOUT[]   = "Sold out";
const char M_NOJOKER[]   = "No room for another Joker";
const char M_NOCONS[]    = "No room for another consumable";
const char M_ETERNAL[]   = "Eternal: it cannot be sold";

const char* const g_TextMsg[TX_COUNT] = { "Again!", "Upgrade!", "Reset", "Level Up!", "Debuffed", "Eaten!", "Extinct!", "Safe!", "Saved!", "Balanced!" };

// card modifiers: name + effect (info panel), edition and seal names
const char* const g_EnhText[ENH_COUNT] = { "", "Bonus Card: +30 chips", "Mult Card: +4 mult", "Wild Card: counts as any suit", "Glass Card: x2 mult, may shatter",
	"Steel Card: x1.5 mult while held", "Stone Card: +50 chips, no rank or suit", "Gold Card: $3 if held at round end", "Lucky Card: 1 in 5 +20 mult, 1 in 15 $20" };
const char* const g_EdName[5]   = { "", "Foil +50 chips", "Holographic +10 mult", "Polychrome x1.5 mult", "Negative: +1 slot" };
const char* const g_SealName[SEAL_COUNT] = { "", "Gold Seal: $3 when played", "Red Seal: scores twice", "Blue Seal: Planet if held", "Purple Seal: Tarot if discarded" };
const char* const g_SealShort[SEAL_COUNT] = { "", "Gold Seal", "Red Seal", "Blue Seal", "Purple Seal" };

// decks and stakes (deck screen)
const char* const g_DeckName[DECK_COUNT] = { "Red Deck", "Blue Deck", "Yellow Deck", "Green Deck", "Ghost Deck", "Abandoned Deck", "Checkered Deck", "Zodiac Deck", "Painted Deck", "Plasma Deck", "Erratic Deck", "Black Deck", "Magic Deck", "Nebula Deck", "Anaglyph Deck" };
const char* const g_DeckDesc[DECK_COUNT] = {
	"+1 discard every round",
	"+1 hand every round",
	"Start the run with an extra $10",
	"End of round: $2 per hand left and $1 per discard left. No interest",
	"Spectral cards may appear in the shop. Start with a Hex card",
	"Start with no Face Cards in your deck",
	"Start with 26 Spades and 26 Hearts in your deck",
	"Start with the Tarot Merchant, Planet Merchant and Overstock vouchers",
	"+2 hand size, -1 Joker slot",
	"Chips and Mult are balanced when scoring. Blinds are twice as big",
	"The ranks and suits of the starting deck are randomized",
	"+1 Joker slot, -1 hand every round",
	"Start with the Crystal Ball voucher and 2 copies of The Fool",
	"Start with the Telescope voucher, -1 consumable slot",
	"After defeating each Boss Blind, gain a Double Tag" };
const char* const g_StakeName[STAKE_COUNT] = { "White Stake", "Red Stake", "Green Stake", "Black Stake", "Blue Stake", "Purple Stake", "Orange Stake", "Gold Stake" };
const char* const g_StakeDesc[STAKE_COUNT] = {
	"Base difficulty",
	"The Small Blind gives no reward money",
	"Blinds grow faster each Ante. Includes the stakes before it",
	"Shop Jokers can be Eternal: they cannot be sold or destroyed",
	"-1 discard every round",
	"Blinds grow faster still",
	"Shop Jokers can be Perishable: debuffed after 5 rounds",
	"Shop Jokers can be Rental: cost $1, but take $3 every round" };

// info panel texts (moved out of the UI core segment, which is full)
const char I_BLINDDESC[] = "Reach the target score before you run out of hands.";
const char I_FACEDOWN[] = "Face down card";
const char I_EMPTYJ[] = "Empty Joker slot";
const char I_EMPTYC[] = "Empty consumable slot";
const char I_PLAYD[] = "Score the selected cards (1-5).";
const char I_DISCD[] = "Throw away the selected cards and draw new ones.";
const char I_RANKD[] = "Highest rank on the left.";
const char I_SUITD[] = "Group cards by suit.";
const char I_INFOD[] = "Poker hand levels and statistics.";
const char I_SELLD[] = "Sell this card for money.";
const char I_USED[] = "Use this consumable now.";
const char I_REROLLD[] = "Get a new set of shop cards. The price grows each time.";
const char I_NEXTD[] = "Leave the shop and pick the next Blind.";
const char I_RANKT[] = "Sort by Rank";
const char I_SUITT[] = "Sort by Suit";
const char I_PLAYT[] = "Play Hand";
const char I_CHIPSW[] = "chips when scored";
const char I_SMALLB[] = "Small Blind";
const char I_BIGB[] = "Big Blind";
const char* const g_PackName[5] = { "Arcana Pack", "Celestial Pack", "Buffoon Pack", "Standard Pack", "Spectral Pack" };
const char* const g_PackDesc[5] = { "Choose from random Tarot cards.", "Choose from random Planet cards.", "Choose from random Jokers.", "Choose playing cards to add to your deck.", "Choose from random Spectral cards." };
