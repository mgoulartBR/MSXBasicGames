package
{
	import org.flixel.*;
	
	public class NewsItem
	{
		public var dayRangeStart:int;
		public var dayRangeEnd:int;
		public var topic:String;
		private var blurbText:String;
		private var articleText:String;
		public var appearTime:Number = 0;
		public var used:Boolean = false;
		public var loyaltyEffect:int;
		public var interesting:Boolean;
		
		public static const kLoyaltyUp:int = 1;
		public static const kLoyaltyDown:int = -1;
		public static const kLoyaltyNone:int = 0;
		
		public static const kInteresting:Boolean = true;
		public static const kUninteresting:Boolean = false;
		
		public static var allNewsItems:Array = new Array(

// plot
	// if '|' is in blurbText, region is selected based on current goal status: kStatusNotWorking | kStatusWorkingTowards | kStatusMet
	new NewsItem(1, 3, kLoyaltyUp, kInteresting, "The rebellion has been crushed. Peace returns to all sectors", "Rebellion Crushed, Peace Restored!"),
	new NewsItem(3, -1, kLoyaltyNone, kInteresting, "*** ####....##...####..## ***", null),
	new NewsItem(4, -1, kLoyaltyNone, kInteresting, "*** Est#blishing secure chan#el. Aw#it further# comm###ication ***", null),
	new NewsItem(6, -1, kLoyaltyNone, kInteresting, "*** #Please hear me. I am Kurstov, leader of#the rebellion. We need your help. ***", null),
	new NewsItem(7, -1, kLoyaltyNone, kInteresting, "*** We can rescue your family. Sow disloyalty to strengthen the rebels. You have 4 days. ***", null),
	new NewsItem(8, -1, kLoyaltyNone, kInteresting, "*** Please help us. The government's tyranny must end! Place negative articles! ***|*** Your family will soon be safe! Drop the public's loyalty to -30 and get 1000 readers in 3 days! ***|*** It's working! Your efforts have strengthened us unimaginably! ***", null),
	new NewsItem(9, -1, kLoyaltyNone, kInteresting, "*** The government cannot win! Seal their fate! Place negative articles! ***|*** Your family's safety is assured! Convince 1000 readers to be disloyal in 2 days! ***|*** Yes! Our operations are in order. Soon we overthrow! ***", null),
	new NewsItem(10, -1, kLoyaltyNone, kInteresting, "*** We have no time! The people must be free! Spread negative news! ***|*** Our time is at hand! Hurry! Get 1000 readers with -30 loyalty by the end of today! ***|*** Oh glorious day! We strike at sundown. Prepare yourself! ***", null),

	new NewsItem(7, 100, kLoyaltyUp, kInteresting, "Terrorist rebel hideout near Central Chem destroyed", "Rebels Routed At Factory!"),
	new NewsItem(8, 100, kLoyaltyDown, kInteresting, "Rebels at Central Chem sabotage important machinary", "Factory Sabotaged!"),
	new NewsItem(9, 100, kLoyaltyUp, kInteresting, "Terrorist 2nd-in-command captured. Renounces fight against [GOV]", "Terrorist Leader Buckles!"),
	new NewsItem(10, 100, kLoyaltyDown, kInteresting, "Rebels regroup in western towns. Growing in strength and number.", "Rebels Gaining Support!"),

// war (always interesting)
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] forces have destroyed Antegria's illegal satellites", "[GOV] Downs Enemy Satellite!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] borders have been reinforced with 200,000 additional troops", "Borders Reinforced!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "State-of-the-art military spy satellites now used to reduce crime", "Keeping An Eye On Crime!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] Navy commissions an additional 500 destroyers to patrol coast", "Safeguarding The Coasts!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] Air Force tactical fighter sets new speed record", "Faster Fighter Flown!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Multiple terrorist cells in central district foiled in operation", "Central Terrorists Terminated!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] Army 5th Divison shuts down bomb factory in northern mountains", "Bomb Factory Found, Destroyed!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] soldiers strongest in the world according to latest tests", "Our Boys Are the Best!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Peace enforcement squad rounds up 200 terrorist rebels", "Peace Restored, Rebels Captured!"),
	
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "40,000 gallons of military gasoline stolen from western bases", "Military Gas Gone!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Critical oil fields in the north have been sabotaged", "Pipelines Crippled!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Terrorist bomb explodes on northern bay ferry. 600 people missing", "Explosion Rocks The Seas!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "[GOV] Air Force tactical fighter test flight ends in crash. Crew lost", "Futuristic Fight Crashes, Burns!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "[GOV] Navy identifies critical fault in all operational submarines", "Our Subs Are Faulty!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "The top general in charge of southern forces has died suddenly", "General Dies Overnight!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Antegria secret code remains unbreakable. Top [GOV] minds are flumoxed", "The Enemy's Unbreakable Code!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Tank production falls behind schedule. Poor factory conditions blamed", "Tanking Tanks!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Worldwide survey finds [GOV] soldiers worst trained, with worst aim", "Our Boys Can't Fire Straight!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Antegria Navy sinks [GOV] battleship off eastern coast", "[GOV] Battleship Bested!"),

// 	politics (never interesting)
	new NewsItem(0, 0, kLoyaltyUp, kUninteresting, "The Honorable and Great Leader awarded Lifetime Glory medal", "A Lifetime of Glory!"),
	new NewsItem(0, 0, kLoyaltyUp, kUninteresting, "Agricultural output from the farming sector doubles for 10th straight month", "More Corn Than Air!"),
	new NewsItem(0, 0, kLoyaltyUp, kUninteresting, "Income reallocation scheme contributes 400 million to schools. Proves system works", "Education Spending Up!"),
	new NewsItem(0, 0, kLoyaltyUp, kUninteresting, "Latest polls show broad satisfaction with government leaders", "Politics Polls Positive!"),
	new NewsItem(0, 0, kLoyaltyUp, kUninteresting, "Newest regional administrator fights for worker's rights", "Power To The People!"),

	new NewsItem(0, 0, kLoyaltyDown, kUninteresting, "Party officials have voted to adjust ration quotas for all orphans", "Less Food For Orphans"),
	new NewsItem(0, 0, kLoyaltyDown, kUninteresting, "The Honorable and Great Leader photographed in women's clothes", "Great Leader, In A Dress!"),
	new NewsItem(0, 0, kLoyaltyDown, kUninteresting, "30,000 teachers and academics reassigned to more useful labor tasks", "Educators Punished For Being Smart!"),
	new NewsItem(0, 0, kLoyaltyDown, kUninteresting, "Local citizen council votes will be eliminated in favor of suggestive comments", "Local Councils Lose Vote!"),
	new NewsItem(0, 0, kLoyaltyDown, kUninteresting, "Yearly donations to the state must increase to support growing government oversight", "Taxes Rise For 8th Year!"),

// weather (no loyalty effect, always interesting)
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Skies and temperatures will remain calm today", "Another Sunny Day!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Storms predicted to wash western coast out to sea", "Western Storms Threaten Coast!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Forecast expects heavy rains in the north and east", "Showers Rain Down!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Expect unseasonal snow in the south", "Blizzard Incoming?"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Sunny morning and cloudy evening for the day", "Warm To Cloudy!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Light showers throughout the day", "Warm To Cloudy This Week!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Hurricane-level winds spotted off eastern coast", "Eastern Hurricanes Return!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Clear skies and no sign of rain", "Another Dry Day!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Freezing sleet and snow expected in northern mountains", "Buckle Down For Ice!"),
	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Weather: Tropical breezes blow across southeastern coast", "Sea Breeze Incoming!"),

// sports (always interesting)
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "[GOV] National Team has won the global football tournament", "[GOV] Wins Football Crown!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Antegria ski team soundly defeated by [GOV] crew", "[GOV] Defeats Antegria Skiers!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Tennis star Restojiu powers through semifinal brackets", "Tennis Star Advances!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Young [GOV] atheletes dominate track and field. May win Olympic gold", "Our Young Heroes!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Skilled [GOV] baseball team finishes record season. Thanks Leader for support", "Baseball Success Sealed!"),

	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "Championship weight lifter Lekshou retires due to crippling injury", "Muscleman Retires!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "[GOV] National Football Team has lost the regional finals to Antegria", "[GOV] Football Stumbles!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Entire [GOV] National Hockey team killed in plane crash", "Tragedy Strikes Hockey!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Athletic training in [GOV] is years behind the competition", "Our Athletes: Behind The Curve?"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "National kayaking team has defected to Antegria", "Kayaking For The Enemy!"),

// entertainment (always interesting)
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Cherrywood's newest stars attended recent gala ball to honor verterans", "Stars Dance For Vets!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "New fall TV programming will focus on [GOV]'s rebuilding", "Fall TV Revealaed!"),
	new NewsItem(0, 0, kLoyaltyUp, kInteresting, "Beloved children's book \"Mumpit Mush\" finally coming to the big screen", "Mumpit Mush Is Coming!"),
	new NewsItem(2, -1, kLoyaltyUp, kInteresting, "Superstars Chad and Jenlyn preparing for Cherrywood wedding tomorrow", "C&J To Tie the Knot!"),
	new NewsItem(3, -1, kLoyaltyUp, kInteresting, "Superstars Chad and Jenlyn marry in extravagant festival", "C&J Finally Hitched!"),

	new NewsItem(0, 0, kLoyaltyNone, kInteresting, "\"My butt is not too fat, just right\" claims TV star Aprelica", "Butt Within Spec!"),

	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Reality star Mestonda found dead from apparent overdose", "Reality Star Overdoses!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Fashion designer CrevyCrevy has defected to Antegria", "Fashion Icon Defects!"),
	new NewsItem(0, 0, kLoyaltyDown, kInteresting, "Mega-group HugginBoyz admits to not singing on any albums, can barely dance", "HugginBoyz: Talentless After All!"),
	new NewsItem(6, -1, kLoyaltyDown, kInteresting, "Superstars Chad and Jenlyn file for divorce. Both claim infidelity", "C&J Fairytale Ends!")
		);
		
		public function NewsItem(
			dayRangeStart_:int, dayRangeEnd_:int, loyaltyEffect_:int,
			interesting_:Boolean, blurbText_:String, articleText_:String) 
		{
			dayRangeStart = dayRangeStart_;
			dayRangeEnd = dayRangeEnd_;
			loyaltyEffect = loyaltyEffect_;
			interesting = interesting_;
			blurbText = blurbText_;
			articleText = articleText_;
		}
		
		public function isWeather():Boolean
		{
			return blurbText.indexOf("Weather:") >= 0;
		}
		
		private function getProcessedText(str:String):String
		{
			if (str.indexOf("|") >= 0)
			{
				var tokens:Array = str.split("|");
				var goalStatus:int = Goal.getCurGoalStatus();
				str = tokens[goalStatus-1];			
				FlxG.log("goal status = " + goalStatus);
				FlxG.log("token = " + str);	
			}
			return GameStatus.expandGovNames(str);
		}
		
		public function getBlurbText():String
		{
			return getProcessedText(blurbText);
		}
		
		public function getArticleText():String
		{
			return articleText ? getProcessedText(articleText) : "";
		}
		
		public function hasArticleText():Boolean
		{
			return articleText != null;
		}
		
		public function isRebelLeader():Boolean
		{
			return blurbText.indexOf("***") >= 0;
		}
		
		public static function resetAllNewsItems():void
		{
			for each (var newsItem:NewsItem in allNewsItems)
			{
				newsItem.used = false;
			}
		}
	}
}