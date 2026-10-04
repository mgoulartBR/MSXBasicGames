package
{
	import org.flixel.*;
	
	public class Assets
	{
		[Embed(source="assets/Cursor.png")] public static var cursorImage:Class;
		[Embed(source="assets/Background.png")] public static var backgroundImage:Class;
		[Embed(source="assets/ArticleSmall.png")] public static var articleS:Class;
		[Embed(source="assets/ArticleMed.png")] public static var articleM:Class;
		[Embed(source="assets/ArticleBig.png")] public static var articleB:Class;
		[Embed(source="assets/Blurb.png")] public static var blurb:Class;
		[Embed(source="assets/BlurbCheck.png")] public static var blurbCheck:Class;		
		[Embed(source="assets/BlurbArticleS.png")] public static var blurbArticleS:Class;		
		[Embed(source="assets/BlurbArticleM.png")] public static var blurbArticleM:Class;		
		[Embed(source="assets/BlurbARticleB.png")] public static var blurbArticleB:Class;		
		[Embed(source="assets/CenterPopup.png")] public static var centerPopup:Class;		
		[Embed(source="assets/Button.png")] public static var button:Class;		
		[Embed(source="assets/StatMeter.png")] public static var statMeter:Class;		
		[Embed(source="assets/Logo.png")] public static var logo:Class;		
		[Embed(source="assets/LogoSmall.png")] public static var logoSmall:Class;
		[Embed(source="assets/Logo2.png")] public static var logo2:Class;		
		[Embed(source="assets/LogoSmall2.png")] public static var logoSmall2:Class;
		[Embed(source="assets/Dragging.png")] public static var dragging:Class;
		[Embed(source="assets/Mute.png")] public static var mute:Class;
		[Embed(source="assets/Morning.png")] public static var morningImage:Class;
		[Embed(source="assets/PrintedPaper.png")] public static var printedPaperImage:Class;

		[Embed(source="assets/7x5.ttf", fontFamily="FEED", embedAsCFF="false")] public static var feedFont:String;
		[Embed(source="assets/MotorolaScreentype.ttf", fontFamily="ARTICLEB", embedAsCFF="false")] public static var articleFontB:String;
		[Embed(source="assets/SILKWONDER.ttf", fontFamily="ARTICLEM", embedAsCFF="false")] public static var articleFontM:String;
		[Embed(source="assets/SG03.ttf", fontFamily="ARTICLES", embedAsCFF="false")] public static var articleFontS:String;
		
		[Embed(source="assets/MainMusic.mp3")] public static var mainMusic:Class;
		[Embed(source="assets/NightMusic.mp3")] public static var nightMusic:Class;
		[Embed(source="assets/Silence.mp3")] public static var nullSound:Class;
		[Embed(source="assets/Drop.mp3")] public static var dropSound:Class;
		[Embed(source="assets/Drag.mp3")] public static var dragSound:Class;
		[Embed(source="assets/ButtonDown.mp3")] public static var buttonDownSound:Class;
		[Embed(source="assets/ButtonUp.mp3")] public static var buttonUpSound:Class;
		[Embed(source="assets/DayOver.mp3")] public static var dayOverSound:Class;
		[Embed(source="assets/Feed.mp3")] public static var feedSound:Class;
		[Embed(source="assets/Alarm.mp3")] public static var alarmSound:Class;
	}
}
