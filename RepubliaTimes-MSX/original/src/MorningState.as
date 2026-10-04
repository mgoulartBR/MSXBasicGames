package
{
	import org.flixel.*;
 
	public class MorningState extends FlxState
	{
		private var time:Number;
		private var backgroundSprite:FlxSprite;
		private var logoSprite:FlxSprite;
		private var messageText:FlxText;
		private var nextButton:FlxButton;
		private var dayText:FlxText;
		private var creditsText:FlxText;
		private var statMeters:StatMeters;
		
		override public function create():void
		{
			FlxG.bgColor = 0xffffffff;
			FlxG.playMusic(Assets.mainMusic, 0.5);
			
			time = 0;
			
			var gs:GameStatus = GameStatus.instance;
			var rs:Readership = gs.readership;
			var dayNumber:int = gs.dayNumber;
			var goal:Goal = Goal.getGoalForDay(dayNumber);
			var prevGoal:Goal = Goal.getGoalForDay(dayNumber-1);
			
			var gameOver:Boolean = false;
			var rebelsWon:Boolean = false;
			
			var killMessage:String = "Your services are no longer required. Your family has been eliminated and you will be reassigned.";
			
			var message:String = "";
			if (dayNumber == 1)
			{
				message += "Welcome to The [GOV] Times. You are the new editor-in-chief.\n\n";
				if (gs.stateInControl)
				{
					message += "The war with Antegria is over and the rebellion uprising has been crushed. Order is slowly returning to [GOV].\n\n";
					message += "The public is not loyal to the government.\n\n";
				}
				else
				{
					message += "Freedom has returned to [GOV], but the public is skeptical.\n\n";
				}
				message += "It is your job to increase their loyalty by editing The [GOV] Times carefully. ";
				message += "Pick only stories that highlight the good things about [GOV] and its government.\n\n";
				message += "You have 3 days to raise the public's loyalty to " + goal.targetLoyalty + ".\n\n";
				
				if (gs.haveWonAtLeastOnce)
				{
					message += "We have found a new wife and child for you. ";
					message += "As a precaution against influence, we are keeping them in a safe location.";
				}
				else
				{
					message += "As a precaution against influence, we are keeping your wife and child in a safe location.";
				}
			}
			else if (goal && goal != prevGoal)
			{
				// check prevGoal completion
				if (prevGoal.id == "first-state")
				{
					if (rs.curLoyalty >= prevGoal.targetLoyalty)
					{
						message += "You have completed your first task. The Great and Honorable Leader is pleased.\n\n";
						message += "Continue to print positive articles and maintain a loyalty of at least " + goal.targetLoyalty + ".\n\n";
						message += "We must now work to increase readership. More minds is more power.\n\n";
						message += "Attain at least " + goal.targetReaderCount + " readers by the end of day " + goal.targetDayNumber + ".";
					}
					else 
					{
						message += "You have failed to inspire your readers and their loyalty remains weak.\n\n";
						message += killMessage;
						gameOver = true;
					}
				}
				else if (prevGoal.id == "second-state")
				{
					if (prevGoal.isMet())
					{
						message += "Congratulations, you have completed your second task. The Great and Honorable Leader is pleased.\n\n";
						message += "From this point we will withdraw our close oversight.\n\n";
						message += "Continue to increase readership and maintain the promotion of positive news.";
					}
					else 
					{
						message += "You have failed to acquire enough readers with loyalty " + goal.targetLoyalty + ". Without a loyal audience, The [GOV] Times has no influence.\n\n";
						message += killMessage;
						gameOver = true;
					}
				}
			}
			else if (goal && goal.id.indexOf("state") >= 0)
			{
				// working for state
				if (rs.curLoyalty >= goal.targetLoyalty)
				{
					message += "Good work. The Great and Honorable Leader has been notified of your diligent efforts.\n\n";
					message += "Keep your reader's loyalty at " + goal.targetLoyalty + " or higher.";
				}
				else if (rs.getLoyaltyDelta() > 0)
				{
					message += "You are making good progress.\n\n";
					message += "Keep working towards a loyalty of " + goal.targetLoyalty + " or above by the end of day " + goal.targetDayNumber + ".";
				}
				else if (rs.getLoyaltyDelta() < 0)
				{
					message += "This is not good. Loyalty is dropping.\n";
					message += "You must choose positive articles that cast [GOV] in a good light. Try harder.\n\n";
					message += "Bring your reader's loyalty to at least " + goal.targetLoyalty + " by the end of day " + goal.targetDayNumber + "."; 
				}
				else if (rs.getLoyaltyDelta() == 0)
				{
					message += "This is not good. Loyalty is not improving.\n";
					message += "You must choose positive articles that cast [GOV] in a good light. Try harder.\n\n";
					message += "Bring your reader's loyalty to at least " + goal.targetLoyalty + " by the end of day " + goal.targetDayNumber + "."; 
				}				
				if (goal.targetReaderCount)
				{
					message += "\n";
					if (rs.curReaderCount >= goal.targetReaderCount)
					{
						//message += "Good work. You have reached the requested readership quota.\n\n";
						message += "Maintain at least " + goal.targetReaderCount + " readers.";
					}
					else
					{
						//message += "Your efforts to increase circulation are failing.\n\n";
						message += "You must have " + goal.targetReaderCount + " or more readers by the end of day " + goal.targetDayNumber + ".";
					}
				}
				
				message += "\n\n";
				message += getFamilyMessage(rs.curLoyalty);
			}
			else if (goal)
			{
				// working for rebels
				if (rs.getLoyaltyDelta() >= 0)
				{
					message += "Good morning.\n\n";
				}
				if (rs.getLoyaltyDelta() < 0)
				{
					message += "A drop in reader loyalty has been noted. Try harder.\n\n";
				}
				message += getPerformanceMessage(rs.curLoyalty) + "\n" + getFamilyMessage(rs.curLoyalty);
			}
			else
			{
				// final
				FlxG.log("prev = " + prevGoal.toString());
				if (prevGoal.isMet())
				{
					// rebels succeeded
					message += "We have done it!\n\n";
					message += "Thank you my friend! Without your efforts the rebellion would have failed once again. ";
					message += "A new era for our beloved nation begins!\n\n";
					message += "I'm truly sorry that we could not save your family.\n\n";
					message += "We need someone with your skills to talk with the people. Come back tomorrow for your new position.\n\n";
					message += "Long Live [GOV]!";
					gs.stateInControl = !gs.stateInControl;
					rebelsWon = true;
				}
				else
				{
					// rebels failed
					message += "We have reviewed your file.\n\n";
					message += getPerformanceMessage(rs.curLoyalty) + "\n\n";
					message += "The Great and Honorable Leader has decided that printed paper is old technology. ";
					message += "The Ministry of Media will be moving to focus on online communications.\n\n";
					message += killMessage;
				}
				gameOver = true;
			}
			
			message += "\n\n";		
			
			if (!gameOver)	
				message += getTutorialMessage(dayNumber);
				
			message = GameStatus.expandGovNames(message);
			
			backgroundSprite = new FlxSprite(0,0,Assets.morningImage);
			add(backgroundSprite);
			
			logoSprite = new FlxSprite(0,20,GameStatus.instance.stateInControl ? Assets.logo : Assets.logo2);
			logoSprite.x = FlxG.width/2-logoSprite.width/2;
			add(logoSprite);
			
			nextButton = new FlxButton(FlxG.width/2 - Const.buttonW/2, FlxG.height-50, "", gameOver ? onClickGameOver : onClickNext);
			nextButton.label.color = 0xff000000;
			nextButton.loadGraphic(Assets.button);
			nextButton.label.text = rebelsWon ? "Let's Go!" : (gameOver ? "Accept Fate" : "Start Work");
			add(nextButton);
			
			messageText = new FlxText(100, 90, FlxG.width-200);
			messageText.color = rebelsWon ? 0xffff0000 : 0xff000000;
			messageText.text = message;
			messageText.y = 180 - messageText.height/2;
			add(messageText);
			
			dayText = new FlxText(FlxG.width/2-25, 70, 50);
			dayText.color = 0xff000000;
			dayText.alignment = "center";
			dayText.text = "Day " + dayNumber;
			add(dayText);

			creditsText = new FlxText(0, FlxG.height-35, FlxG.width);
			creditsText.color = 0xff000000;
			creditsText.alignment = "right";
			creditsText.text = "by\nLucas Pope\n@dukope";
			add(creditsText);
			
			if (dayNumber > 1)
			{
				statMeters = new StatMeters(FlxG.width-90, 120, true);
				statMeters.setValues(rs, false);
				add(statMeters);
			}
			
			add(Util.createMuteButton(Util.kCornerBL));
			Util.attachButtonSounds();
		}		
		
		private function getPerformanceMessage(loyalty:int):String
		{
			var str:String = "";
			str += "Your performance is: -";
			if (loyalty >= Const.statMax) str += "APPRECIATED";
			else if (loyalty > Const.statMax*2/3) str += "ACCEPTABLE";
			else if (loyalty > Const.statMax*1/3) str += "MARGINAL";
			else if (loyalty >= -Const.statMax*1/3) str += "UNSATISFACTORY";
			else if (loyalty <= -Const.statMax) str += "DISASTROUS";
			else if (loyalty < -Const.statMax*2/3) str += "DISASTROUS";
			else if (loyalty < -Const.statMax*1/3) str += "DISAPPOINTING";
			str += "-";
			return str;
		}
		
		private function getFamilyMessage(loyalty:int):String
		{
			var str:String = "";
			str += "Your family ";
			if (loyalty >= Const.statMax) str += "is receiving excellent treatment.";
			else if (loyalty > Const.statMax*2/3) str += "is being well-cared for.";
			else if (loyalty > Const.statMax*1/3) str += "lives normally under our care.";
			else if (loyalty >= -Const.statMax*1/3) str += "has lost several privileges.";
			else if (loyalty <= -Const.statMax) str += "endures daily beatings.";
			else if (loyalty < -Const.statMax*2/3) str += "suffers due to your failures.";
			else if (loyalty < -Const.statMax*1/3) str += "is being punished for your poor performance.";
			return str;
		}
		
		private function getTutorialMessage(dayNumber:int):String
		{
			var str:String = "";
			
			if (dayNumber == 2)
			{
				// article size
				str += "__________________________________________\n";
				str += "Article Size\n\n";
				str += "Larger articles have more influence on your reader's loyalty. ";
				str += "Use this to emphasize the stories you want and to downplay unflattering ones.\n";
			}
			else if (dayNumber == 3)
			{
				// reader interest
				str += "__________________________________________\n";
				str += "Reader Interest\n\n";
				str += "The public is interested in sports, entertainment, and military matters. They are also fascinated by the weather. ";
				str += "Choose stories on these topics to increase readership.\n";
			}
			else if (dayNumber == 4)
			{
				// weather
				str += "__________________________________________\n";
				str += "Article Positioning\n\n";
				str += "Article placement has no effect on loyalty or reader interest. Only the size and content of stories matter. ";
				str += "Use your vast artistic and design experience to arrange articles in a way that pleases you.";
			}			
			else if (dayNumber == 5)
			{
				// weather
				str += "__________________________________________\n";
				str += "Weather\n\n";
				str += "The government cannot control the weather yet. As a result, articles about the weather do not affect loyalty.";
			}
			else if (dayNumber == 6)
			{
				// politics
				str += "__________________________________________\n";
				str += "Politics\n\n";
				str += "The public finds political stories uninteresting, but positive articles on political subjects can increase loyalty.";
			}
			else if (dayNumber == 7)
			{
				// politics
				str += "__________________________________________\n";
				str += "Article Size and Reader Interest\n\n";
				str += "Article size does not affect reader interest. ";
				str += "If a paper contains articles on interesting topics of any size, readers will be interested.";
			}
						
			return str;
		}
		
		override public function update():void
		{
			if (FlxG.debug)
			{
				if (FlxG.keys.pressed('K'))
				{
					GameStatus.instance.dayNumber++;
					FlxG.log(GameStatus.instance.readership.toString());
					FlxG.switchState(new MorningState());
				}
				if (FlxG.keys.pressed('O'))
				{
					GameStatus.instance.readership.curLoyalty -= 10;
					FlxG.log(GameStatus.instance.readership.toString());
					FlxG.switchState(new MorningState());
				}
				if (FlxG.keys.pressed('P'))
				{
					GameStatus.instance.readership.curLoyalty += 10;
					FlxG.log(GameStatus.instance.readership.toString());
					FlxG.switchState(new MorningState());
				}
				if (FlxG.keys.pressed('N'))
				{
					GameStatus.instance.readership.curReaderCount -= 100;
					FlxG.log(GameStatus.instance.readership.toString());
					FlxG.switchState(new MorningState());
				}
				if (FlxG.keys.pressed('M'))
				{
					GameStatus.instance.readership.curReaderCount += 100;
					FlxG.log(GameStatus.instance.readership.toString());
					FlxG.switchState(new MorningState());
				}
				//FlxG.log(GameStatus.instance.readership.toString());
			}
			
			var prevTime:Number = time;
			time += FlxG.elapsed;
			super.update();
		}
		
		protected function onClickNext():void
		{
			FlxG.switchState(new PlayState());
		}

		protected function onClickGameOver():void
		{
			GameStatus.instance.haveWonAtLeastOnce = true;
			GameStatus.instance.reset();
			FlxG.switchState(new MorningState());
		}			
	}
}
