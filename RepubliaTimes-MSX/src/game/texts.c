// Story/briefing texts (port of MorningState.as / NightState.as message logic).
#include "logic.h"

static char* s_P; // write cursor
static bool s_Gov; // government name used for [GOV]: Republia (TRUE) / Democria (FALSE)

static void put(const char* s)
{
	while (*s)
		*s_P++ = *s++;
}

// append with [GOV] expansion
static void m(const char* s)
{
	const char* gov = s_Gov ? "Republia" : "Democria";
	while (*s)
	{
		if (s[0] == '[' && s[1] == 'G' && s[2] == 'O' && s[3] == 'V' && s[4] == ']')
		{
			put(gov);
			s += 5;
		}
		else
			*s_P++ = *s++;
	}
}

static void num(i16 v)
{
	char tmp[8];
	u8 n = 0;
	if (v < 0)
	{
		*s_P++ = '-';
		v = -v;
	}
	do
	{
		tmp[n++] = '0' + (v % 10);
		v /= 10;
	} while (v);
	while (n)
		*s_P++ = tmp[--n];
}

static const char* const s_Kill =
	"Your services are no longer required. Your family has been eliminated and you will be reassigned.";

static void performance(i8 loyalty)
{
	m("Your performance is: -");
	if (loyalty >= STAT_MAX) m("APPRECIATED");
	else if (loyalty > STAT_MAX * 2 / 3) m("ACCEPTABLE");
	else if (loyalty > STAT_MAX / 3) m("MARGINAL");
	else if (loyalty >= -STAT_MAX / 3) m("UNSATISFACTORY");
	else if (loyalty <= -STAT_MAX) m("DISASTROUS");
	else if (loyalty < -STAT_MAX * 2 / 3) m("DISASTROUS");
	else m("DISAPPOINTING");
	m("-");
}

static void family(i8 loyalty)
{
	m("Your family ");
	if (loyalty >= STAT_MAX) m("is receiving excellent treatment.");
	else if (loyalty > STAT_MAX * 2 / 3) m("is being well-cared for.");
	else if (loyalty > STAT_MAX / 3) m("lives normally under our care.");
	else if (loyalty >= -STAT_MAX / 3) m("has lost several privileges.");
	else if (loyalty <= -STAT_MAX) m("endures daily beatings.");
	else if (loyalty < -STAT_MAX * 2 / 3) m("suffers due to your failures.");
	else m("is being punished for your poor performance.");
}

static void tutorial(u8 day)
{
	static const char sep[] = "__________________________________________\n";
	switch (day)
	{
	case 2:
		m(sep);
		m("Article Size\n\nLarger articles have more influence on your reader's loyalty. "
		  "Use this to emphasize the stories you want and to downplay unflattering ones.\n");
		break;
	case 3:
		m(sep);
		m("Reader Interest\n\nThe public is interested in sports, entertainment, and military matters. "
		  "They are also fascinated by the weather. Choose stories on these topics to increase readership.\n");
		break;
	case 4:
		m(sep);
		m("Article Positioning\n\nArticle placement has no effect on loyalty or reader interest. "
		  "Only the size and content of stories matter. Use your vast artistic and design experience "
		  "to arrange articles in a way that pleases you.");
		break;
	case 5:
		m(sep);
		m("Weather\n\nThe government cannot control the weather yet. As a result, articles about the weather do not affect loyalty.");
		break;
	case 6:
		m(sep);
		m("Politics\n\nThe public finds political stories uninteresting, but positive articles on political subjects can increase loyalty.");
		break;
	case 7:
		m(sep);
		m("Article Size and Reader Interest\n\nArticle size does not affect reader interest. "
		  "If a paper contains articles on interesting topics of any size, readers will be interested.");
		break;
	}
}

void Text_Morning(char* dst, MorningResult* res)
{
	u8 day = g_Game.day;
	u8 goal = Goal_ForDay(day);
	u8 prevGoal = Goal_ForDay(day - 1);
	bool gameOver = FALSE;
	bool rebelsWon = FALSE;
	i8 ld = Game_LoyaltyDelta();

	s_P = dst;
	// The original expands [GOV] once, at the end of MorningState.create(), i.e. AFTER the final-day
	// branch has toggled stateInControl; reproduce that by predicting the toggle up front.
	s_Gov = g_Game.stateInControl;
	if (goal == GOAL_NONE && day > 1 && Goal_IsMet(prevGoal))
		s_Gov = !s_Gov;
	if (day == 1)
	{
		m("Welcome to The [GOV] Times. You are the new editor-in-chief.\n\n");
		if (g_Game.stateInControl)
		{
			m("The war with Antegria is over and the rebellion uprising has been crushed. Order is slowly returning to [GOV].\n\n");
			m("The public is not loyal to the government.\n\n");
		}
		else
			m("Freedom has returned to [GOV], but the public is skeptical.\n\n");
		m("It is your job to increase their loyalty by editing The [GOV] Times carefully. ");
		m("Pick only stories that highlight the good things about [GOV] and its government.\n\n");
		m("You have 3 days to raise the public's loyalty to ");
		num(Goal_TargetLoyalty(goal));
		m(".\n\n");
		if (g_Game.wonOnce)
			m("We have found a new wife and child for you. As a precaution against influence, we are keeping them in a safe location.");
		else
			m("As a precaution against influence, we are keeping your wife and child in a safe location.");
	}
	else if (goal != GOAL_NONE && goal != prevGoal)
	{
		if (prevGoal == GOAL_FIRST)
		{
			if (g_Game.loyalty >= Goal_TargetLoyalty(prevGoal))
			{
				m("You have completed your first task. The Great and Honorable Leader is pleased.\n\n");
				m("Continue to print positive articles and maintain a loyalty of at least ");
				num(Goal_TargetLoyalty(goal));
				m(".\n\nWe must now work to increase readership. More minds is more power.\n\n");
				m("Attain at least ");
				num(Goal_TargetReaders(goal));
				m(" readers by the end of day ");
				num(Goal_TargetDay(goal));
				m(".");
			}
			else
			{
				m("You have failed to inspire your readers and their loyalty remains weak.\n\n");
				m(s_Kill);
				gameOver = TRUE;
			}
		}
		else if (prevGoal == GOAL_SECOND)
		{
			if (Goal_IsMet(prevGoal))
			{
				m("Congratulations, you have completed your second task. The Great and Honorable Leader is pleased.\n\n");
				m("From this point we will withdraw our close oversight.\n\n");
				m("Continue to increase readership and maintain the promotion of positive news.");
			}
			else
			{
				m("You have failed to acquire enough readers with loyalty ");
				num(Goal_TargetLoyalty(goal));
				m(". Without a loyal audience, The [GOV] Times has no influence.\n\n");
				m(s_Kill);
				gameOver = TRUE;
			}
		}
	}
	else if (goal != GOAL_NONE && goal != GOAL_LAST)
	{
		// working for the state
		i8 tl = Goal_TargetLoyalty(goal);
		u8 td = Goal_TargetDay(goal);
		if (g_Game.loyalty >= tl)
		{
			m("Good work. The Great and Honorable Leader has been notified of your diligent efforts.\n\n");
			m("Keep your reader's loyalty at ");
			num(tl);
			m(" or higher.");
		}
		else if (ld > 0)
		{
			m("You are making good progress.\n\nKeep working towards a loyalty of ");
			num(tl);
			m(" or above by the end of day ");
			num(td);
			m(".");
		}
		else
		{
			m(ld < 0 ? "This is not good. Loyalty is dropping.\n" : "This is not good. Loyalty is not improving.\n");
			m("You must choose positive articles that cast [GOV] in a good light. Try harder.\n\n");
			m("Bring your reader's loyalty to at least ");
			num(tl);
			m(" by the end of day ");
			num(td);
			m(".");
		}
		if (Goal_TargetReaders(goal))
		{
			m("\n");
			if (g_Game.readers >= (i16)Goal_TargetReaders(goal))
			{
				m("Maintain at least ");
				num(Goal_TargetReaders(goal));
				m(" readers.");
			}
			else
			{
				m("You must have ");
				num(Goal_TargetReaders(goal));
				m(" or more readers by the end of day ");
				num(td);
				m(".");
			}
		}
		m("\n\n");
		family(g_Game.loyalty);
	}
	else if (goal != GOAL_NONE)
	{
		// working for the rebels
		if (ld >= 0)
			m("Good morning.\n\n");
		else
			m("A drop in reader loyalty has been noted. Try harder.\n\n");
		performance(g_Game.loyalty);
		m("\n");
		family(g_Game.loyalty);
	}
	else
	{
		// final day
		if (Goal_IsMet(prevGoal))
		{
			m("We have done it!\n\n");
			m("Thank you my friend! Without your efforts the rebellion would have failed once again. ");
			m("A new era for our beloved nation begins!\n\n");
			m("I'm truly sorry that we could not save your family.\n\n");
			m("We need someone with your skills to talk with the people. Come back tomorrow for your new position.\n\n");
			m("Long Live [GOV]!");
			g_Game.stateInControl = !g_Game.stateInControl;
			rebelsWon = TRUE;
		}
		else
		{
			m("We have reviewed your file.\n\n");
			performance(g_Game.loyalty);
			m("\n\nThe Great and Honorable Leader has decided that printed paper is old technology. ");
			m("The Ministry of Media will be moving to focus on online communications.\n\n");
			m(s_Kill);
		}
		gameOver = TRUE;
	}

	m("\n\n");
	if (!gameOver)
		tutorial(day);
	*s_P = 0;
	res->gameOver = gameOver;
	res->rebelsWon = rebelsWon;
}

static void result_line(const char* name, i16 value, i16 delta)
{
	m(name);
	m(": ");
	num(value);
	if (delta > 0)
	{
		m("   (+");
		num(delta);
		m(")");
	}
	else if (delta < 0)
	{
		m("   (");
		num(delta);
		m(")");
	}
	else
		m("   (no change)");
}

void Text_Night(char* dst)
{
	s_P = dst;
	s_Gov = g_Game.stateInControl;
	m("Today's issue has been printed and distributed.\n\nRESULTS\n\n");
	result_line("Loyalty", g_Game.loyalty, Game_LoyaltyDelta());
	m("\n");
	result_line("Readership", g_Game.readers, Game_ReadersDelta());
	m("\n\n");
	u8 c = g_Game.comments;
	if (c & CMT_BLANK) m("* The paper is blank. Money was saved on ink, but you have lost many readers.\n");
	if (c & CMT_TOO_FEW) m("* There are too few articles. You have lost readers.\n");
	if (c & CMT_FEW_INTEREST) m("* There are not enough interesting articles. You have lost readers.\n");
	if (c & CMT_MANY_INTEREST) m("* There are many interesting articles. You have gained readers.\n");
	if (c & CMT_LOYALTY_UP) m("* The included articles have increased your readership's loyalty to the government.\n");
	if (c & CMT_LOYALTY_DOWN) m("* The included articles have decreased your readership's loyalty to the government.\n");
	if (c & CMT_INFLUENCE_UP) m("* The paper's increasing readership has expanded its influence.\n");
	*s_P = 0;
}
