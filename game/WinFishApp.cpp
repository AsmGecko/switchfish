#include "platform/PadCursor.h"
#include "fcaseopen/fcaseopen.h"
#include "sound/SoundManager.h"
#include "WideTank.h"
#include "SEHCatcher.h"
#include "SexyAppBase.h"
#include "widget/WidgetManager.h"
#include "misc/ResourceManager.h"
#include "graphics/ImageFont.h"
#include "widget/DialogButton.h"
#include "graphics/SWTri.h"
#include "widget/Checkbox.h"
#include "sound/SDLMusicInterface.h"
#include "misc/Buffer.h"
#include "graphics/Color.h"
#include "widget/ListWidget.h"

#include "WinFishApp.h"
#include "BubbleMgr.h"
#include "graphics/GLInterface.h"
#include "WinFishCommon.h"
#include "InternetManager.h"
#include "Res.h"

#include "Fish.h"
#include "Bilaterus.h"

#include "Board.h"
#include "TitleScreen.h"
#include "HelpScreen.h"
#include "StoreScreen.h"
#include "HatchScreen.h"
#include "GameSelector.h"
#include "BonusScreen.h"
#include "HighScoreScreen.h"
#include "InterludeScreen.h"
#include "StoryScreen.h"
#include "SimFishScreen.h"
#include "TankScreen.h"
#include "PetsScreen.h"
#include "SimSetupScreen.h"

#include "WorkerThread.h"
#include "HighScoreMgr.h"
#include "ProfileMgr.h"
#include "FishSongMgr.h"

#include "MoneyDialog.h"
#include "ContinueDialog.h"
#include "NewUserDialog.h"
#include "OptionsDialog.h"
#include "UserDialog.h"
#include "UpdateCheckDialog.h"
#include "VirtualDialog.h"
#include "PetDialog.h"
#include "ScreenSaverDialog.h"
#include "FishNamingDialog.h"
#include "FoodDialog.h"
#include "RegisterDialog.h"

#include <filesystem>
#include <thread>
#include <chrono>
#ifdef __SWITCH__
#include <switch.h>
#endif
#include <ctime>

namespace Sexy
{
	bool gUnkBool01 = false;
	bool gDoHundredUpdates = false;
	bool gUnkBool03 = false;
	bool gUnkBool04 = false;
	bool gUnkBool05 = false;
	bool gUnkBool06 = false;
	bool gUnkBool09 = false;
	bool gCanRemapMusic = true;
	bool gFishSongsLoaded = false;
	int gUsersDatVersion = 14;
	bool gScreenSaverStarted;
	int gUpdateFramesCounter = 0;
	bool gFirstVirtualTankEnter = false;
	int gFoodLimit = 1;
	int gUnkInt01 = 0;
	FishSongData* gKilgoreSongDataPtr = nullptr;
	FishSongData* gTestSongDataPtr = nullptr;
	int gUnkInt03 = 0;
	int gUnkInt07 = 5000;
	int gPetsDiedOnBossLevel = 0;
	int gUnkInt09 = -1;
	int gUnkInt10 = -1;
	int gUnkInt11 = 2160;
	int gLastStoryId = 0;
	int gDeadPetsIdArray[24] = {0};
	int gUnkIntArray02[100] = {0};
	SexyString gFishSongParseError;
	std::vector<FishSongData*> gSongsVector1;
	std::vector<FishSongData*> gSongsVector2;
	std::vector<FishSongData*> gLudwigSongs;
	std::vector<FishSongData*> gSantaSongs;
	std::map<SexyString, FishSongData*, Sexy::StringLessNoCase> gLongSongsMap;
	int gMusicFishSongsPlayed = 0;
	int gLudwigSongId = 0;
	int gSantaSongId = 0;
	int gSongs2Id = 0;
}

using namespace Sexy;

void LoadFishSongs(bool flag)
{
	if (!flag && gFishSongsLoaded)
		return;
	gFishSongsLoaded = true;

	Sexy::gSongsVector1.clear();
	Sexy::gSongsVector2.clear();
	Sexy::gLudwigSongs.clear();
	Sexy::gLongSongsMap.clear();

	gKilgoreSongDataPtr = nullptr;
	gTestSongDataPtr = nullptr;

	bool hasOpenedErrorFile = false;
	FILE* anErrorFile = nullptr;

	// switchfish: only the .txt songs, in the alphabetical order Windows listed them in, and no exception when
	// the folder is missing
	std::vector<std::string> aSongFiles;
	std::error_code anError;
	for (const auto& entry : std::filesystem::directory_iterator("fishsongs", anError))
	{
		std::string aName = entry.path().filename().string();
		if (aName.size() > 4 && stricmp(aName.c_str() + aName.size() - 4, ".txt") == 0)
		{
			aSongFiles.push_back(aName);
		}
	}
	std::sort(aSongFiles.begin(), aSongFiles.end(),
		[](const std::string& a, const std::string& b) { return stricmp(a.c_str(), b.c_str()) < 0; });

	for (const std::string& filename : aSongFiles)
	{
		FishSongData* aNewSong = new FishSongData();
		gSongsVector1.push_back(aNewSong);

		aNewSong->mProperties.clear();
        std::string relpath = "fishsongs/" + filename;

		if (!aNewSong->Parse(relpath)) // 128
		{
			if (!hasOpenedErrorFile)
			{
				hasOpenedErrorFile = true;
				anErrorFile = fopen("fishsongerror.txt", "w");
			}
			if (anErrorFile != nullptr)
				fprintf(anErrorFile, "%s - %s\n", filename.c_str(), gFishSongParseError.c_str());

			gSongsVector1.pop_back();
			delete aNewSong;
		}
		else 
		{
			size_t und = filename.rfind('_');
			if (und != std::string::npos)
			{
                std::string song_name = filename.substr(0, und);
                std::string len = filename.substr(und + 1);
				if (stricmp(len.c_str(), "long.txt") == 0)
                    aNewSong->mProperties["long"] = song_name;
				else if(stricmp(len.c_str(), "short.txt") == 0)
                    aNewSong->mProperties["short"] = song_name;
			}
		}

	}

	if(anErrorFile != nullptr)
		fclose(anErrorFile);

	for (int i = 0; i < gSongsVector1.size(); i++)
	{
		FishSongData* aSong = gSongsVector1[i];

		bool addToSongsVec2 = true;

		if (!aSong->GetProperty("test", nullptr))
		{
			SexyString aValue = "";
			if (aSong->GetProperty("long", &aValue))
			{ // 329
				gLongSongsMap[aValue] = aSong;
				addToSongsVec2 = false;
			}
			else if (aSong->GetProperty("beethovenrare", nullptr))
			{ // 321
				gLudwigSongs.push_back(aSong);
				addToSongsVec2 = false;
			}
			else if (aSong->GetProperty("beethoven", nullptr))
			{ // 312
				gLudwigSongs.push_back(aSong);
			}
			else if (aSong->GetProperty("santarare", nullptr))
			{ // 304
				gSantaSongs.push_back(aSong);
				addToSongsVec2 = false;
			}
			else if (aSong->GetProperty("santa", nullptr))
			{ // 299
				gSantaSongs.push_back(aSong);
			}
		}
		else // Is test
			gTestSongDataPtr = aSong;

		if (aSong->GetProperty("kilgore", nullptr))
			gKilgoreSongDataPtr = aSong;

		if (addToSongsVec2)
			gSongsVector2.push_back(aSong);
	}
}

void LoadFishSongsTaskWrapper(void* userData)
{
	LoadFishSongs(0);
	gUnkBool05 = false;
}

WinFishApp::WinFishApp()
{
	mBoard = NULL;
	mPrestoMenuData = NULL;
	mGameSelector = NULL;
	mTitleScreen = NULL;
	mTitleMusicPending = false;
	mPetsScreen = NULL;
	mStoreScreen = NULL;
	mHatchScreen = NULL;
	mBonusScreen = NULL;
	mInterludeScreen = NULL;
	mStoryScreen = NULL;
	mSimSetupScreen = NULL;
	mSimFishScreen = NULL;
	mTankScreen = NULL;
	mHighScoreScreen = NULL;
	mHelpScreen = NULL;

	mProdName = "Insaniquarium";
	
	// switchfish: our version (CMake); the window title stays the game's name
	mProductVersion = SWITCHFISH_VERSION;
	mTitle = SexyString("Insaniquarium Deluxe");
	
	mRegKey = "PopCap/Insaniquarium";
	mScreenSaverRegKey = "ScreenSaver/";
	mScreenSaverRegPath = mScreenSaverRegKey;
	mYieldMainThread = false;
	mCurrentProfile = NULL;

	mSeed = new MTRand(Rand());

	mMusicVolume = 1.5;
	m0x7a4 = -1;
	m0x7a8 = -1;
	m0x7ac = -1;
	m0x8a8 = false;
	m0x8a9 = false;
	m0x8aa = false;
	m0x8ab = false;
	mGameNotPlayed = true;
	mFrameTime = 28;
	mGameMode = 0;
	mAutoEnable3D = true;
	m0x881 = true;
	m0x882 = true;
	mRelaxMode = false;
	m0x884 = false;
	mPlaytimeCounter2 = 0; 
	mPlaytimeCounter = 0; 
	mMaxExecutions = 0; 
	mMaxPlays = 0; 
	mMaxTime = 0; 
	mCompletedLoadingThreadTasks = 0;

	mProfileMgr = new ProfileMgr();

	mHighScoreMgr = new HighScoreMgr();

	mPrestoPtr = NULL;

	mScreenSaverSound = true;
	mScreenSaverRotateBackdrops = true;
	mScreenSaverPeriodicDim = true;
	mScreenSaverShowMoney = false;
	mScreenSaverPowerSave = false;
	mScreenSaverEnabled = false;
	mScreenSaverUnk01 = false;

	mTankGameModeChoose = 1;

	m0x878 = false;

	Time aTodayTimeSec = GetTodayStartSeconds();
	mDaysSinceFirstRun = aTodayTimeSec;
	mDaysSinceLastRun = aTodayTimeSec;
	mLDAccum = 0;

	mWorkerThread = new WorkerThread();

	mCurrentSongOffset = -1;
	mCurrentSongId = -1;
}


Sexy::WinFishApp::~WinFishApp()
{
	SDL_Log("exit: game screens");
	if (mBoard)
		SaveCurrentUserData();

	StopMusic();

	for (int i = 0; i < DIALOG_END_ID; i++)
		KillDialog(i);
	ProcessSafeDeleteList(); // This line missed in original game

	if (mBoard != NULL)
	{
		mBoard->SaveCurrentGame();
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = NULL;
	}

	if (mTitleScreen != NULL)
		mWidgetManager->RemoveWidget(mTitleScreen);
	delete mTitleScreen;

	if (mStoreScreen != NULL)
		mWidgetManager->RemoveWidget(mStoreScreen);
	delete mStoreScreen;

	if (mHatchScreen != NULL)
		mWidgetManager->RemoveWidget(mHatchScreen);
	delete mHatchScreen;

	if (mBonusScreen != NULL)
		mWidgetManager->RemoveWidget(mBonusScreen);
	delete mBonusScreen;

	if (mHighScoreScreen != NULL)
		mWidgetManager->RemoveWidget(mHighScoreScreen);
	delete mHighScoreScreen;

	if (mHelpScreen != NULL)
		mWidgetManager->RemoveWidget(mHelpScreen);
	delete mHelpScreen;

	if (mInterludeScreen != NULL)
		mWidgetManager->RemoveWidget(mInterludeScreen);
	delete mInterludeScreen;

	if (mStoryScreen != NULL)
		mWidgetManager->RemoveWidget(mStoryScreen);
	delete mStoryScreen;

	if (mSimFishScreen != NULL)
		mWidgetManager->RemoveWidget(mSimFishScreen);
	delete mSimFishScreen;

	if (mTankScreen != NULL)
		mWidgetManager->RemoveWidget(mTankScreen);
	delete mTankScreen;

	if (mPetsScreen != NULL)
		mWidgetManager->RemoveWidget(mPetsScreen);
	delete mPetsScreen;

	if (mSimSetupScreen != NULL)
		mWidgetManager->RemoveWidget(mSimSetupScreen);
	delete mSimSetupScreen;

	if (mGameSelector != NULL)
		mWidgetManager->RemoveWidget(mGameSelector);
	delete mGameSelector;

	if(mProfileMgr)
		delete mProfileMgr;
	if(mHighScoreMgr)
		delete mHighScoreMgr;
}

// switchfish: boot timings with how much file system work it took
static void LogFileStats(const char* theWhat)
{
#if !defined(_WIN32)
	int anOpens, aMisses, aScans;
	fcaseopen_stats(&anOpens, &aMisses, &aScans);
	SDL_Log("%s (%d opens, %d not found, %d directories read; %s)", theWhat, anOpens, aMisses, aScans,
		ResourceManager::LoadTimes().c_str());
#else
	SDL_Log("%s", theWhat);
#endif
}

// switchfish: the controller rumbles on the moments that matter (docs/CONTROLS.md), told by the sound
// that goes with them: low motor, high motor (0..1), milliseconds.
static void RumbleForSound(intptr_t theSound)
{
	// before loading every SOUND_ is still 0
	if (!gSexyAppBase->mLoadingThreadCompleted)
	{
		return;
	}
	// the last field is the least time between two rumbles for that sound
	static const struct { intptr_t* mSound; float mLow, mHigh; int mMs, mGapMs; } kRumbles[] = {
		// an alien arrives (each kind has its own sound), and the warning before it
		{&SOUND_ROAR, 0.7f, 0.35f, 450, 0}, {&SOUND_ROAR2, 0.7f, 0.35f, 450, 0}, {&SOUND_ROAR3, 0.7f, 0.35f, 450, 0},
		{&SOUND_GUFFAW, 0.7f, 0.35f, 450, 0}, {&SOUND_INTERFER, 0.7f, 0.35f, 450, 0}, {&SOUND_EVILLAFF, 0.7f, 0.35f, 450, 0},
		{&SOUND_AWOOGA, 0.4f, 0.2f, 500, 0},
		// an alien goes down, a bomb goes off (a dead alien keeps exploding for 2 s; bombs close together rumble once)
		{&SOUND_EXPLOSION1, 0.8f, 0.6f, 300, 0}, {&SOUND_EXPLOSION4, 0.8f, 0.6f, 300, 0}, {&SOUND_EXPLODE, 0.6f, 0.5f, 250, 2200},
		// each shot that hits, and a tap on the glass
		{&SOUND_HIT, 0.1f, 0.25f, 35, 0}, {&SOUND_TAPGLASS, 0.2f, 0.45f, 50, 0},
		// a fish dies, a pet hatches
		{&SOUND_DIE, 0.35f, 0.1f, 200, 0}, {&SOUND_HATCH, 0.3f, 0.3f, 250, 0},
	};
	static Uint32 sLast[sizeof(kRumbles) / sizeof(kRumbles[0])] = {};
	for (size_t i = 0; i < sizeof(kRumbles) / sizeof(kRumbles[0]); i++)
	{
		const auto& r = kRumbles[i];
		if (*r.mSound != theSound)
		{
			continue;
		}
		if (!gSexyAppBase->mRumble)
		{
			return;
		}
		Uint32 now = SDL_GetTicks();
		if (sLast[i] == 0 || now - sLast[i] >= (Uint32)r.mGapMs)
		{
			sLast[i] = now;
			RumblePad(r.mLow, r.mHigh, r.mMs);
		}
		return;
	}
}

void Sexy::WinFishApp::Init()
{
	DoParseCmdLine();
	bool anIsScreenSaver = IsScreenSaver();
	mMuteOnLostFocus = true;

	SexyApp::Init();
	if (mSoundManager)
	{
		mSoundManager->mPlayHook = RumbleForSound;
	}

	if (CheckForVista())
	{
		MkDir(GetAppDataFolder());
		AllowAllAccess(GetAppDataFolder());
		InitUserDirectories("userdata");
		//	TODO WinFishAppMoveOldUserdataFolder 
		//	Migrate userdata from game folder to program data
	}

	if (!anIsScreenSaver)
	{
		SexyString aModulePath = GetExeFolder() + "WinFish.exe";

		if (CheckForVista())
		{
			std::string aFileName = GetFileName(aModulePath.c_str());
			std::string aPath = RemoveTrailingSlash(mChangeDirTo) + "/" + aFileName;
			aModulePath = aPath;
		}
		SexyString aExeDir = GetFileDir(aModulePath);

		SexyString aRegKeyDir = mScreenSaverRegKey + "Directory";

		RegistryWriteString(aRegKeyDir, aExeDir);

		bool aShouldCopy = GetBoolean("ScrCopy", false);

		if (!aShouldCopy)
		{
			SexyString aScrExeName = GetString("ScrExe", "");

			if (aScrExeName.length() == 0)
			{
				SexyString aRegKeyExe = mScreenSaverRegKey + "Executable";
				RegistryWriteString(aRegKeyExe, aModulePath);
			}
			else // If WinFish_Scr.exe exists in string map
			{
				std::string aDir = GetFileDir(aModulePath);

				std::string aDestPath = aDir + "/" + aScrExeName;

				//if (!FileExists(aDestPath))
					//CopyFileA(aModulePath.c_str(), aDestPath.c_str(), FALSE);

				SexyString aRegKeyExe = mScreenSaverRegKey + "Executable";
				RegistryWriteString(aRegKeyExe, aDestPath);
			}
		}
	}
    
	SDL_Log("boot: init");
	bool aSuccessfulResLoad = mResourceManager->ParseResourcesFile("properties/resources.xml");
	SDL_Log("boot: resources.xml parsed");

	if (aSuccessfulResLoad)
	{
		aSuccessfulResLoad = mResourceManager->LoadResources("Init") && 
			ExtractInitResources(mResourceManager) && 
			LoadImageById(mResourceManager, IMAGE_STINKY_ID);
		LogFileStats("boot: Init group loaded");

		if (aSuccessfulResLoad)
		{
			mProfileMgr->ReadUsersDat();
			mHighScoreMgr->ReadHighScoresData();

			SexyString aPath = mScreenSaverRegPath + "User";
			SexyString aUserName;

			if (RegistryReadString(aPath, &mScreenSaverUserName) && anIsScreenSaver && mScreenSaverUserName.size() != 0)
				mCurrentProfile = mProfileMgr->GetUserProfile(mScreenSaverUserName);

			bool aSuccessCurUser = true;

			if (mCurrentProfile == NULL)
			{
				if (!RegistryReadString("CurUser", &aUserName))
					aSuccessCurUser = false;
			}
			else
				aSuccessCurUser = false;

			if (aSuccessCurUser)
				mCurrentProfile = mProfileMgr->GetUserProfile(aUserName);

			if (mCurrentProfile == NULL)
				mCurrentProfile = mProfileMgr->GetFirstUserProfile();

			if (!anIsScreenSaver || mCurrentProfile != NULL)
			{
				mMaxExecutions = GetInteger("MaxExecutions", 0);
				mMaxPlays = GetInteger("MaxPlays", 0);
				mMaxTime = GetInteger("MaxTime", 60);

				Image* aImg = LoadMaskImage(IMAGE_TITLEPAGE, IMAGE_TITLEPAGEMASK, 384, 388);
				ReplaceImageById(mResourceManager, IMAGE_TITLEPAGEMASK_ID, aImg);

				mPrestoMenuData = new PrestoMenuData();

				mTitleScreen = new TitleScreen(this);
				mTitleScreen->Resize(0, 0, mWidth, mHeight);
				mWidgetManager->AddWidget(mTitleScreen);


				// switchfish: the music loads first thing in the loading thread instead of here, before the
				// first frame (decoding the MO3s took seconds on the Switch)
				mLoadTitleMusic = !anIsScreenSaver;
				SetCursorImage(CURSOR_POINTER, IMAGE_CURSOR_POINTER);
				SetCursorImage(CURSOR_HAND, IMAGE_CURSOR_HAND);
				SetCursorImage(CURSOR_DRAGGING, IMAGE_CURSOR_DRAGGING);
				SetCursorImage(CURSOR_TEXT, IMAGE_CURSOR_TEXT);
			}
			else
			{
				LogScreenSaverError("No User Profile");
				mLoadingFailed = true;
			}
		}
	}

	if (!aSuccessfulResLoad)
		WFAShowResourceError();
}

void Sexy::WinFishApp::LoadingThreadProc()
{
	SDL_Log("boot: loading thread");
	// switchfish: the music loads on its own thread next to the resources (the loading thread spends much of
	// its time waiting for decoded images anyway); joined on every way out of this function
	struct Joiner
	{
		std::thread mThread;
		~Joiner()
		{
			if (mThread.joinable())
			{
				mThread.join();
			}
		}
	} aMusic;
	if (mLoadTitleMusic)
	{
		aMusic.mThread = std::thread([this]()
		{
#ifdef __SWITCH__
			// core 2, at the main thread's priority: above the image worker there (0x2D)
			if (R_FAILED(svcSetThreadCoreMask(CUR_THREAD_HANDLE, 2, BIT(2))) ||
				R_FAILED(svcSetThreadPriority(CUR_THREAD_HANDLE, 0x2C)))
			{
				SDL_Log("music thread: couldn't move it to core 2");
			}
#endif
			std::filesystem::path rsc = Sexy::GetResourceFolder();
			mMusicInterface->LoadMusic(4, (rsc / "music" / "Insaniq2.mo3").string());
			// started in UpdateFrames once the title screen is on screen
			mTitleMusicPending = true;
			mMusicInterface->LoadMusic(1, (rsc / "music" / "Alien.mo3").string());
			mMusicInterface->LoadMusic(3, (rsc / "music" / "Lullaby.mo3").string());
			SDL_Log("boot: music loaded");
		});
	}
	SexyString aResNames[] = {"Register", "LoadingThread"};

	int arraySize = sizeof(aResNames) / sizeof(aResNames[0]);
	for (int i = 0; i < arraySize; i++)
		mNumLoadingThreadTasks += mResourceManager->GetNumResources(aResNames[i]);

	mNumLoadingThreadTasks += 33;


	for (int i = 0; i < arraySize; i++)
	{
		mResourceManager->StartLoadResources(aResNames[i]);

		while (mResourceManager->LoadNextResource())
		{
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}

		if (mResourceManager->HadError() || !ExtractResourcesByName(mResourceManager, aResNames[i].c_str()))
		{
			WFAShowResourceError(false);
			mLoadingFailed = true;
			// switchfish: stop here like the original (Insaniquarium-Decompilation WinFishApp.cpp:678) instead of
			// going on to use images that never loaded
			return;
		}

		m0x878 = true;
	}

	MemoryImage* aVertCreaseMImage = new MemoryImage(this);
	MemoryImage* aHorzCreaseMImage = new MemoryImage(this);
	aVertCreaseMImage->Create(IMAGE_VERTCREASE->mWidth, 60);
	aHorzCreaseMImage->Create(60, IMAGE_HORZCREASE->mHeight);
	Graphics aGVert(aVertCreaseMImage);
	Graphics aGHorz(aHorzCreaseMImage);

	int aHghtOrWdth = 0;
	while (aHghtOrWdth < 60)
	{
		aGVert.DrawImage(IMAGE_VERTCREASE, 0, aHghtOrWdth);
		aHghtOrWdth += IMAGE_VERTCREASE->mHeight;
	}
	aHghtOrWdth = 0;
	while (aHghtOrWdth < 60)
	{
		aGHorz.DrawImage(IMAGE_HORZCREASE, aHghtOrWdth, 0);
		aHghtOrWdth += IMAGE_HORZCREASE->mWidth;
	}

	ReplaceImageById(mResourceManager, IMAGE_VERTCREASE_ID, aVertCreaseMImage);
	ReplaceImageById(mResourceManager, IMAGE_HORZCREASE_ID, aHorzCreaseMImage);

	int aImgId = IMAGE_TANKMASK1_ID;
	int aImgYOffset = 365;
	int aImgCounter = 0;

	while (aImgCounter < 6)
	{
		Image* aMaskAquaImg = GetImageById(aImgId);
		Image* aAquaImg = GetImageById(aImgId-6);
		Image* aReplacementImg = LoadMaskImage(aAquaImg, aMaskAquaImg, 0, aImgYOffset);
		ReplaceImageById(mResourceManager, aImgId, aReplacementImg);
		if (mShutdown)
			return;

		mCompletedLoadingThreadTasks++;
		aImgId++;
		aImgCounter = aImgId - IMAGE_TANKMASK1_ID;
	}

	// switchfish: wide tank art (the tank width is set before this thread starts)
	LogFileStats("boot: loading thread resources done");
	BuildWideBackdrops(this);

	aImgId = IMAGE_SCL_STINKY_ID;
	aImgCounter = 0;
	while (aImgCounter < 24)
	{
		Image* aSclImg = GetImageById(aImgId + aImgCounter);
		MemoryImage* aSclMImage = new MemoryImage(this);
		aSclMImage->Create(40, 40);
		Graphics aGScl(aSclMImage);
		aGScl.SetFastStretch(false);

		Rect aDestRect;
		Rect aSrcRect;
		if (aImgId + aImgCounter == IMAGE_SCL_AMP_ID)
		{
			int aHght = aSclImg->mHeight;
			if (aHght > 23)
				aHght = 24;
			int aWdth = aSclImg->mWidth;
			if (aWdth > 63)
				aWdth = 64;
			aDestRect = Rect(0, 10, aSclMImage->mWidth, aSclMImage->mHeight-20);
			aSrcRect = Rect(0,0, aWdth, aHght);
		}
		else
		{
			int aHght = aSclImg->mHeight;
			if (aHght > 60)
				aHght = 60;
			int aWdth = aSclImg->mWidth;
			if (aWdth > 60)
				aWdth = 60;
			aDestRect = Rect(0, 0, aSclMImage->mWidth, aSclMImage->mHeight);
			aSrcRect = Rect(0, 0, aWdth, aHght);
		}

		aGScl.DrawImage(aSclImg, aDestRect, aSrcRect);

		mPrestoMenuData->mPetImages[aImgCounter] = aSclMImage;
		if (mShutdown)
			return;
		mCompletedLoadingThreadTasks++;
		aImgCounter++;
	}
	SDL_Log("boot: loading thread done");
}

void Sexy::WinFishApp::LoadingThreadCompleted()
{
	if (IsScreenSaver())
		return;

	if (ShouldCheckForUpdate())
		DoUpdateDialog();
}

MusicInterface* Sexy::WinFishApp::CreateMusicInterface()
{
	if (!IsScreenSaver())
		return SexyApp::CreateMusicInterface();

	return new SDLMusicInterface;
}

void Sexy::WinFishApp::URLOpenFailed(const std::string& theURL)
{
	SexyApp::URLOpenFailed(theURL);
	KillDialog(DIALOG_URL_INFO);
	CopyToClipboard(theURL);

	DoDialog(DIALOG_URL_INFO, true, "Open Browser", "Please open the following URL in your browser\n\n\n\nFor your convenience, this URL has already been copied to your clipboard.", "OK", Dialog::BUTTONS_FOOTER);
}

void Sexy::WinFishApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyApp::URLOpenSucceeded(theURL);
	KillDialog(DIALOG_URL_INFO);
}

bool Sexy::WinFishApp::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	DoDialog(DIALOG_URL_INFO, true, "Opening Browser", "Opening Browser", "", Dialog::BUTTONS_NONE);
	DrawDirtyStuff();
	return SexyApp::OpenURL(theURL, shutdownOnOpen);
}

Dialog* Sexy::WinFishApp::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	return SexyApp::DoDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
}

bool Sexy::WinFishApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		Widget* aTheScr = NULL;
		if (mDialogMap.size() == 0 && ((aTheScr = mStoreScreen) != NULL || 
			(aTheScr = mBoard) != NULL || (aTheScr = mGameSelector) != NULL))
			mWidgetManager->SetFocus(aTheScr);

		if (mBoard != NULL)
			if (!ShouldKillDialog())
				mBoard->PauseGame(false);

		return true;
	}
	return false;
}

Dialog* Sexy::WinFishApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	MoneyDialog* aDialog = new MoneyDialog(this, IMAGE_DIALOG, IMAGE_DIALOGBUTTON, 
		theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	int aStrWdth = aDialog->mHeaderFont->StringWidth(theDialogHeader);
	aStrWdth += 150;
	if (aStrWdth < 348)
		aStrWdth = 348;
	int aPrefHeight = aDialog->GetPreferredHeight(aStrWdth);
	int aMinHeight = aDialog->mComponentImage->mHeight * 2 / 3 + 10;
	if (aPrefHeight < aMinHeight)
		aPrefHeight = aMinHeight;
	aDialog->Resize(143, 142, aStrWdth, aPrefHeight);
	return aDialog;
}

void Sexy::WinFishApp::SetMusicVolume(double theVolume)
{
	SexyApp::SetMusicVolume(theVolume);
}

void Sexy::WinFishApp::ModalOpen()
{
	if (mBoard != NULL)
		if (ShouldKillDialog())
			mBoard->PauseGame(true); 
}

void WinFishApp::LostFocus()
{
	if (mBoard != NULL && mGameMode != GAMEMODE_VIRTUAL_TANK)
	{
		if (!IsScreenSaver())
		{
			mBoard->mShouldSave = true;
			mBoard->SaveCurrentGame();
			WriteToRegistry();
		}
		DoLostFocusDialog();
	}
}

bool Sexy::WinFishApp::DebugKeyDown(int theKey)
{
	return SexyApp::DebugKeyDown(theKey);
}

void Sexy::WinFishApp::ButtonDepress(int theId)
{
	int anIdVar1 = theId % 10000;
	if (anIdVar1 < 2000)
		return;
	if (anIdVar1 >= 3000)
	{
		if (anIdVar1 > 3999) return;
		anIdVar1 = theId - 3000;
		if (anIdVar1 < 10007)
		{
			if (anIdVar1 != 10006)
			{
				switch(theId)
				{
				case 3003:
					KillDialog(3);
					Shutdown();
					break;
				case 3004:
					KillDialog(4);
					break;
				case 3000 + DIALOG_USER_DIALOG:
					UserDialogOkPressed(false);
					break;
				case 3000 + DIALOG_NEW_USER:
					MakeNewUser(false);
					break;
				case 3000 + DIALOG_ARE_YOU_SURE_DELETE:
					DeleteUser(false);
					break;
				case 3000 + DIALOG_RENAME:
					RenameUser(false);
					break;
				case 3000 + DIALOG_PET_DIALOG:
					ApplyPrestoMorph(-1);
					break;
				case 3000 + DIALOG_CONFIRM_PURCHASE:
					ConfirmFishPurchase(false);
					break;
				case 3000 + DIALOG_SELL_DIALOG:
					ApplySellGameObject(false);
					break;
				case 3000 + DIALOG_TIMES_UP:
					KillDialog(DIALOG_TIMES_UP);
					SwitchToBonusScreen();
					break;
				case 3000 + DIALOG_GIVE_SHELLS:
					ApplyGiveShells(false);
					break;
				default:
					KillDialog(anIdVar1);
				}
				return;
			}

			KillDialog(10006);
			anIdVar1 = 6;
		}

		KillDialog(anIdVar1);
	}
	anIdVar1 = theId - 2000;
	if (anIdVar1 > 10006)
	{
		if (anIdVar1 == 20006)
		{
			KillDialog(20006);
			KillDialog(DIALOG_UPDATE_CHECK);
			return;
		}
		KillDialog(anIdVar1);
		return;
	}
	if (anIdVar1 == 10006)
	{
		KillDialog(10006);
		KillDialog(DIALOG_UPDATE_CHECK);
		OpenURL(mInternetManager->GetUpdateURL(), true);
		return;
	}

	switch (anIdVar1)
	{
	case 0:
		KillDialog(0);
		SwitchToGameSelector();
		break;
	case DIALOG_OPTIONS:
		ApplyOptionsSettings();
		break;
	case DIALOG_REGISTER:
	{
		RegisterDialog* aDia = (RegisterDialog*)GetDialog(DIALOG_REGISTER);
		if (aDia)
		{
			SexyString aName = aDia->mNameEditWidget->mString;
			SexyString aCode = aDia->mCodeEditWidget->mString;
			if (aCode.length() == 0)
				OpenRegisterPage();
			else
			{
				if (Validate(aName, aCode))
				{
					KillDialog(DIALOG_REGISTER);
					KillDialog(3);
					mUserName = aName;
					mRegCode = aCode;
					mIsRegistered = true;
					mWidgetManager->MarkAllDirty();
					if (mTitleScreen == nullptr)
						DoDialog(DIALOG_THANKS_FOR_REGISTER, true, "Thanks!", "Thank you for registering Insaniquarium!", "Ok", Dialog::BUTTONS_FOOTER);
					else
						mTitleScreen->RegisterSuccessful();
				}
				else
				{
					DoInvalidCodeDialog();
				}
			}
		}
		break;
	}
	case 4:
		KillDialog(4);
		StartGame();
		break;
	case DIALOG_UPDATE_ASK:
		KillDialog(DIALOG_UPDATE_ASK);
		DoUpdateCheckDialog();
		break;
	case DIALOG_QUIT_GAME:
		KillDialog(DIALOG_QUIT_GAME);
		Shutdown();
		break;
	case 13:
		KillDialog(13);
	case 3:
		DoRegisterDialog();
		break;
	case DIALOG_INFO:
		KillDialog(DIALOG_INFO);
		break;
	case DIALOG_FISH_NAMING:
		ApplyFishNameDialog();
		break;
	case DIALOG_FIRST_LVL_GAME_OVER:
		KillDialog(DIALOG_FIRST_LVL_GAME_OVER);
		mBoard->SpawnGuppyBought();
		break;
	case DIALOG_GAME_OVER:
		KillDialog(DIALOG_GAME_OVER);
		RemoveBoard();
		SwitchToGameSelector();
		break;
	case DIALOG_LOST_FOCUS:
		KillDialog(DIALOG_LOST_FOCUS);
		break;
	case DIALOG_OUT_OF_MONEY_LOAN:
		KillDialog(DIALOG_OUT_OF_MONEY_LOAN);
		mBoard->Unk07(100);
		break;
	case 20:
		KillDialog(20);
		RemovePetsScreen();
		StartGame();
		break;
	case 21:
		KillDialog(21);
		if (mBoard)
			mBoard->BonusRoundDropShell();
		break;
	case DIALOG_LEAVE_GAME:
		KillDialog(DIALOG_LEAVE_GAME);
		LeaveGameBoard();
		break;
	case DIALOG_USER_DIALOG:
		UserDialogOkPressed(true);
		break;
	case DIALOG_NEW_USER:
		MakeNewUser(true);
		break;
	case DIALOG_ARE_YOU_SURE_DELETE:
		DeleteUser(true);
		break;
	case DIALOG_RENAME:
		RenameUser(true);
		break;
	case DIALOG_INFO_NEW_USER:
	{
		KillDialog(DIALOG_INFO_NEW_USER);
		NewUserDialog* aDia = (NewUserDialog*)GetDialog(DIALOG_NEW_USER);
		if (aDia)
			mWidgetManager->SetFocus(aDia->mEditWidget);
		break;
	}
	case DIALOG_NAME_CONFLICT:
	{
		KillDialog(DIALOG_NAME_CONFLICT);
		NewUserDialog* aDia = (NewUserDialog*)GetDialog(DIALOG_RENAME);
		if (aDia)
			mWidgetManager->SetFocus(aDia->mEditWidget);
	}
		break;
	case DIALOG_RESTART_GAME:
		RestartLevel();
		break;
	case DIALOG_CONFIRM_PURCHASE:
		ConfirmFishPurchase(true);
		break;
	case DIALOG_SELL_DIALOG:
		ApplySellGameObject(true);
		break;
	case DIALOG_SCREENSAVER:
		ApplyScreenSaverSettings();
		break;
	case DIALOG_TIMES_UP:
		KillDialog(DIALOG_TIMES_UP);
		SwitchToBonusScreen();
		break;
	case DIALOG_GIVE_SHELLS:
		ApplyGiveShells(true);
		break;
	default:
		KillDialog(anIdVar1);
		break;
	}
}

// AI Generated
void Sexy::WinFishApp::InitUserDirectories(const std::string& theSubfolderName)
{
	if (!CheckForVista())
		return;

	std::string aCurrentDir = GetCurDir();
	aCurrentDir = RemoveTrailingSlash(aCurrentDir);
	std::string aRelativePath = aCurrentDir + "/" + theSubfolderName;

	std::string aAppDataPath = GetAppDataFolder();
	std::string aProperPath = aAppDataPath + "/" + theSubfolderName;

	if (FileExists(aRelativePath) && FileExists(aProperPath))
		return;

	MkDir(aRelativePath);
	MkDir(aProperPath);
}

void Sexy::WinFishApp::PlayMusic(int theSongId, int theSongOffset, bool noLoop)
{
	mCurrentSongId = theSongId;
	mCurrentSongOffset = theSongOffset;

	RemapMusicTrack(&theSongId, &theSongOffset);

	mMusicInterface->PlayMusic(theSongId, theSongOffset, noLoop);
}

void Sexy::WinFishApp::SomeMusicFunc(bool flag)
{
	if (mGameMode == GAMEMODE_VIRTUAL_TANK)
	{
		flag = false;
		StopMusic();
	}
	else if (flag)
	{
		int aTank = GetCurrentTank();
		if (aTank == 1 || aTank == 2)
			FadeOutMusic(0, false, 0.008);
		else if (aTank == 3 || aTank == 4)
			FadeOutMusic(2, false, 0.008);
	}
	else
		StopMusic();

	PlayMusic(1, 0);
	if (flag)
	{
		mMusicInterface->StopMusic(1);
		FadeInMusic(1, -1, 0.002, false);
	}
}

void Sexy::WinFishApp::FadeOutMusic(int theSongId, bool stopSong, double theSpeed)
{
	if (mCurrentSongId == theSongId)
		mCurrentSongId = -1;

	if (gCanRemapMusic && (theSongId == 0 || theSongId == 2))
		theSongId = 4;

	mMusicInterface->FadeOut(theSongId, stopSong, theSpeed);
}

void Sexy::WinFishApp::FadeInMusic(int theSongId, int theSongOffset, double theSpeed, bool noLoop)
{
	mCurrentSongOffset = theSongOffset;
	mCurrentSongId = theSongId;
	RemapMusicTrack(&theSongId, &theSongOffset);
	mMusicInterface->FadeIn(theSongId, theSongOffset, theSpeed, noLoop);
}

void Sexy::WinFishApp::RemapMusicTrack(int* theSongId, int* theSongOffset)
{
	if (!gCanRemapMusic)
		return;

	int aOriginalSongId = *theSongId;
	int aOriginalOffset = *theSongOffset;

	if (aOriginalSongId == 0)
	{
		if (aOriginalOffset == 22)
		{
			*theSongId = 4;
			*theSongOffset = 0;
			return;
		}
		if (aOriginalOffset == 0)
		{
			*theSongId = 4;
			*theSongOffset = 12;
			return;
		}
	}

	if (aOriginalSongId == 2)
	{
		if (aOriginalOffset == 32) 
		{
			*theSongId = 4;
			*theSongOffset = 25;
			return;
		}
		if (aOriginalOffset == 45) 
		{
			*theSongId = 4;
			*theSongOffset = 37;
			return;
		}
		if (aOriginalOffset == 0) 
		{
			*theSongId = 4;
			*theSongOffset = 45;
			return;
		}
		if (aOriginalOffset == 54) 
		{
			*theSongId = 4;
			*theSongOffset = 49;
			return;
		}
		if (aOriginalOffset == 5) 
		{
			*theSongId = 4;
			*theSongOffset = 50;
			return;
		}
	}

	if ((aOriginalSongId == 0) && (aOriginalOffset == 13))
	{
		*theSongId = 4;
		*theSongOffset = 58;
		return;
	}
}

Image* Sexy::WinFishApp::LoadMaskImage(Image* theImage, Image* theImageMask, int theX, int theY)
{
	int aImageWidth = theImage->GetWidth();
	int aImageHeight = theImage->GetHeight();

	int aMaskWidth = theImageMask->GetWidth();
	int aMaskHeight = theImageMask->GetHeight();

	Rect aImageRect(0, 0, aImageWidth, aImageHeight);

	Rect aMaskRect(theX, theY, aMaskWidth, aMaskHeight);

	Rect aFinalClipRect = aMaskRect.Intersection(aImageRect);

	if (aFinalClipRect.mWidth <= 0 || aFinalClipRect.mHeight <= 0)
		return NULL;

	MemoryImage* aNewImage = new MemoryImage(gSexyApp);
	aNewImage->Create(aFinalClipRect.mWidth, aFinalClipRect.mHeight);

	Graphics g(aNewImage);
	g.DrawImage(theImage, -aFinalClipRect.mX, -aFinalClipRect.mY);


	uint32_t* aNewBits = aNewImage->GetBits();
	MemoryImage* aMemoryMask = dynamic_cast<MemoryImage*>(theImageMask);
	uint32_t* aMaskBits = aMemoryMask->GetBits();

	if (aNewBits == NULL || aMaskBits == NULL)
	{
		delete aNewImage;
		return NULL;
	}

	for (int y = 0; y < aFinalClipRect.mHeight; ++y)
	{
		for (int x = 0; x < aFinalClipRect.mWidth; ++x)
		{
			int aNewImageIndex = y * aFinalClipRect.mWidth + x;

			int aMaskX = x + (aFinalClipRect.mX - theX);
			int aMaskY = y + (aFinalClipRect.mY - theY);
			int aMaskIndex = aMaskY * aMaskWidth + aMaskX;

			DWORD aColor = aNewBits[aNewImageIndex] & 0x00FFFFFF;

			DWORD aAlpha = aMaskBits[aMaskIndex] & 0xFF000000;

			aNewBits[aNewImageIndex] = aColor | aAlpha;
		}
	}

	aNewImage->BitsChanged();
	return aNewImage;
}

void Sexy::WinFishApp::Start()
{
	if (!mLoadingFailed)
	{
		SexyApp::Start();
		return;
	}

	if (IsScreenSaver() && !gScreenSaverStarted)
	{
		StartScreenSaver();
		return;
	}
}

void Sexy::WinFishApp::Shutdown()
{
	if (mShutdown)
		return;

	if (mBoard != NULL)
	{
		SaveCurrentUserData();
		RemoveBoard();
	}
	UpdatePlayData();

	Time aTodaysDate = GetTodayStartSeconds();
	int aSecondsPlayed = mUpdateCount / 36;
	mLDAccum += aSecondsPlayed;

	if ((mLDAccum >= 3600) || (aTodaysDate > mDaysSinceFirstRun)) // modified crack code i guess
	{
		mDaysSinceFirstRun = aTodaysDate;
		mLDAccum = 0;
	}
	else if (aTodaysDate < mDaysSinceFirstRun)
	{
	}

	SexyApp::Shutdown();
}

void Sexy::WinFishApp::StartScreenSaver()
{
	mScreenSaverEnabled = false;
	gScreenSaverStarted = true;
	SetScreenSaver(mScreenSaverOldPath.c_str());
}

void Sexy::WinFishApp::SetScreenSaver(const char* thePath)
{
#if 0 // !PORT
	OSVERSIONINFOA aVersionInfo;
	ZeroMemory(&aVersionInfo, sizeof(OSVERSIONINFOA));
	aVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&aVersionInfo);

	if (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		const char* aValue = thePath;
		if (thePath != NULL && strlen(thePath) == 0)
			aValue = NULL;

		WritePrivateProfileStringA("boot", "SCRNSAVE.EXE", aValue, "SYSTEM.INI");
	}
	else
	{
		HKEY aKey;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel/Desktop", 0, KEY_WRITE, &aKey) == ERROR_SUCCESS)
		{
			if (thePath != NULL && strlen(thePath) > 0)
			{
				DWORD aLen = (DWORD)strlen(thePath) + 1;
				RegSetValueExA(aKey, "SCRNSAVE.EXE", 0, REG_SZ, (const BYTE*)thePath, aLen);
			}
			else
			{
				//RegDeleteValueA(aKey, "SCRNSAVE.EXE");
			}

			RegCloseKey(aKey);
		}
	}
#endif
}

void Sexy::WinFishApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();

	mScreenSaverSound = GetBoolean("ScrSound", true);

	mScreenSaverPowerSave = GetBoolean("ScrPowersave", false);

    // !PORT
	//char aUserNameBuffer[1024];
	//DWORD aUserNameSize = sizeof(aUserNameBuffer);
	//if (GetUserNameA(aUserNameBuffer, &aUserNameSize))
	//{
	//	mScreenSaverRegPath.append(aUserNameBuffer, strlen(aUserNameBuffer));
	//	mScreenSaverRegPath += '/';
	//}

	DemoSyncString(&mScreenSaverRegPath);

	RegistryReadString(mScreenSaverRegPath + "OldPath", &mScreenSaverOldPath);
	RegistryReadBoolean(mScreenSaverRegPath + "Enabled", &mScreenSaverEnabled);
	ReadSSFromRegistry();

	if (IsScreenSaver())
	{
		if (!mScreenSaverPowerSave)
		{
			mIsWindowed = false;
			mAllowMonitorPowersave = false;
		}
		else
		{
			mIsWindowed = true;
			mFullScreenWindow = true;
		}
		if (mScreenSaverSound)
			mNoSoundNeeded = true;
		return;
	}

	RegistryReadInteger("ldaccum", &mLDAccum);

	int aTempLDInfo;
	bool aSuccessRead = RegistryReadInteger("ldinfo", &aTempLDInfo);
	Time aDaysLDInfo;
	if (aSuccessRead)
		aDaysLDInfo = (unsigned int)aTempLDInfo;

	bool aInvalidData = !aSuccessRead;

	if (aSuccessRead && aDaysLDInfo > mDaysSinceLastRun)
		aInvalidData = false;

	if (!aInvalidData)
		mLDAccum = 0;
	else
		mDaysSinceLastRun = mDaysSinceFirstRun;
}

void Sexy::WinFishApp::WriteToRegistry()
{
	if (mCurrentProfile)
	{
		if (!IsScreenSaver())
			RegistryWriteString("CurUser", mCurrentProfile->mUserName);
		mCurrentProfile->Save();
	}

	if (!IsScreenSaver())
	{
		SexyApp::WriteToRegistry();
		RegistryWriteInteger("ldinfo", mDaysSinceFirstRun);
		RegistryWriteInteger("ldaccum", mLDAccum);
        RegistryWriteInteger("MusicFishSongsPlayed", gMusicFishSongsPlayed);
	}
}

void Sexy::WinFishApp::ReadSSFromRegistry()
{
	RegistryReadBoolean(mScreenSaverRegPath + "Sound", &mScreenSaverSound);
	RegistryReadBoolean(mScreenSaverRegPath + "RotateBackDrops", &mScreenSaverRotateBackdrops);
	RegistryReadBoolean(mScreenSaverRegPath + "PeriodicDim", &mScreenSaverPeriodicDim);
	RegistryReadBoolean(mScreenSaverRegPath + "ShowMoney", &mScreenSaverShowMoney);
	RegistryReadBoolean(mScreenSaverRegPath + "Powersave", &mScreenSaverPowerSave);

	SexyString aPath2 = GetScreenSaverFilePath();
	SexyString aPathToSYSSS;
	GetSystemScreenSaverPath(aPathToSYSSS);

	bool aPathsAreTheSame = (stricmp(aPathToSYSSS.c_str(), aPath2.c_str()) == 0);

	if (!mScreenSaverEnabled)
	{
        // !PORT: screensaver
		//if (aPathsAreTheSame)
		//	mScreenSaverEnabled = true;
	}
	else
	{
		if (!aPathsAreTheSame)
			mScreenSaverEnabled = false;
	}
}

SexyString Sexy::WinFishApp::GetScreenSaverFilePath()
{
#if 0 // !PORT
	char aModulePathBuffer[260];
	GetModuleFileNameA(NULL, aModulePathBuffer, sizeof(aModulePathBuffer));
	SexyString aModulePath = aModulePathBuffer;
	SexyString aParam1;

	if (CheckForVista())
	{
		SexyString aFileName = GetFileName(aModulePath, false);
		SexyString aDir = RemoveTrailingSlash(mChangeDirTo);
		aModulePath = aDir + "/" + aFileName;
	}

	if (CheckForVista())
		aParam1 = RemoveTrailingSlash(mChangeDirTo);
	else
		aParam1 = GetFileDir(aModulePath, false);

	SexyString aDestPath = aParam1 + "/Insaniquarium.scr";

	char aShortPathBuffer[260];
	if (GetShortPathNameA(aDestPath.c_str(), aShortPathBuffer, sizeof(aShortPathBuffer)) > 0)
		aDestPath = aShortPathBuffer;

	return aDestPath;
#endif
    return _S("");
}

void Sexy::WinFishApp::GetSystemScreenSaverPath(SexyString& theDest)
{
#if 0 // !PORT
	OSVERSIONINFOA aVersionInfo;
	ZeroMemory(&aVersionInfo, sizeof(OSVERSIONINFOA));
	aVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&aVersionInfo);

	char aPathBuffer[261]; 
	aPathBuffer[0] = '\0';

	if (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		GetPrivateProfileStringA(
			"boot",
			"SCRNSAVE.EXE",
			"",
			aPathBuffer,
			sizeof(aPathBuffer),
			"SYSTEM.INI"
		);
	}
	else
	{
		HKEY aKey;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &aKey) == ERROR_SUCCESS)
		{
			DWORD aLen = sizeof(aPathBuffer);
			DWORD aType = REG_SZ;

			RegQueryValueExA(
				aKey,
				"SCRNSAVE.EXE",
				NULL,
				&aType,
				(LPBYTE)aPathBuffer,
				&aLen
			);

			RegCloseKey(aKey);

			if (aLen > 0 && aLen < sizeof(aPathBuffer))
				aPathBuffer[aLen] = '\0';
			else
				aPathBuffer[0] = '\0';
		}
	}

	theDest.assign(aPathBuffer, strlen(aPathBuffer));
#endif
}

bool Sexy::WinFishApp::ShouldKillDialog()
{
	if ((mStoreScreen == nullptr) && (mPetsScreen == nullptr) &&
		(mSimSetupScreen == nullptr) && (mSimFishScreen == nullptr))
	{
		if (mDialogList.size() != 0)
		{
			Dialog* aTopDialog = mDialogList.back();

			if (aTopDialog->mId != 31 && aTopDialog->mId != 32)
				return true;
		}
		return false;
	}
	return true;
}

void Sexy::WinFishApp::DoLeaveGameDialog()
{
	Dialog* aDia = DoDialog(DIALOG_LEAVE_GAME, true, "Leave Game?", "Do you want to return to the\nmain menu?\n\nYour game will be saved.", "", Dialog::BUTTONS_YES_NO);
	aDia->mYesButton->mLabel = "LEAVE";
	aDia->mNoButton->mLabel = "CANCEL";
}

void Sexy::WinFishApp::DoLostFocusDialog()
{
	if (mBoard != NULL)
	{
		if (!IsScreenSaver() && !mBoard->mPause)
		{
			mBoard->PauseGame(true);
			CleanDialogs();
			DoDialog(DIALOG_LOST_FOCUS, true, "GAME PAUSED", "Click to resume game", "Resume game", Dialog::BUTTONS_FOOTER);
		}
	}
}

void Sexy::WinFishApp::DoVirtualDialog()
{
	KillDialog(DIALOG_VIRTUAL);

	VirtualDialog* aDia = new VirtualDialog(this);
	aDia->Resize(320 - aDia->mWidth / 2, 70, aDia->mWidth, aDia->mHeight);

	AddDialog(DIALOG_VIRTUAL, aDia);
}

void Sexy::WinFishApp::DoScreenSaverDialog()
{
	ReadSSFromRegistry();
	ScreenSaverDialog* aDia = new ScreenSaverDialog(this);
	int aPrefHght = aDia->GetPreferredHeight(420);
	aDia->Resize(110, 40, 420, aPrefHght);
	AddDialog(DIALOG_SCREENSAVER, aDia);
}

void Sexy::WinFishApp::DoAreYouSureSellDialog(const SexyString& theLine)
{
	Dialog* aDia = DoDialog(DIALOG_SELL_DIALOG, true, "ARE YOU SURE?", theLine, "", Dialog::BUTTONS_OK_CANCEL);
	aDia->mYesButton->mLabel = "Sell";
	int aDiaY = 250;
	if (mSimFishScreen == nullptr)
		aDiaY = 60;
	aDia->Resize(aDia->mX, aDiaY, aDia->mWidth, aDia->mHeight);
}

void Sexy::WinFishApp::OpenPrestoDialog(GameObject* thePet)
{
	CleanDialogs();
	mPrestoPtr = (OtherTypePet*) thePet;
	Dialog* aDia = new PetDialog(this);
	AddDialog(DIALOG_PET_DIALOG, aDia);
}

void Sexy::WinFishApp::DoConfirmPurchaseDialog(const SexyString& theLine)
{
	Dialog* aDia = DoDialog(DIALOG_CONFIRM_PURCHASE, true, "Confirm Purchase", theLine, "", Dialog::BUTTONS_OK_CANCEL);

	aDia->mYesButton->mLabel = "BUY";
	int aPrefHght = aDia->GetPreferredHeight(350);
	aDia->Resize(aDia->mX, aDia->mY, 350, aPrefHght);
}

void Sexy::WinFishApp::DoFishNamingDialog(const SexyString& theLine, SexyString& theName, bool flag)
{
	FishNamingDialog* aDia = new FishNamingDialog(this, theLine);
	aDia->m0x168 = flag;
	aDia->mEditWidget->SetText(theName, true);
	aDia->mEditWidget->EnforceMaxPixels();
	aDia->mEditWidget->mCursorPos = aDia->mEditWidget->mString.length();
	aDia->mEditWidget->mHilitePos = 0;
	if (mStoreScreen == nullptr)
		aDia->Resize(120, 150, 400, aDia->GetPreferredHeight(400));
	else
		aDia->Resize(218, 200, 400, aDia->GetPreferredHeight(400));
	AddDialog(DIALOG_FISH_NAMING, aDia);
}

void Sexy::WinFishApp::DoFoodDialog()
{
	CleanDialogs();
	FoodDialog* aDia = new FoodDialog(this);
	AddDialog(DIALOG_FOOD, aDia);
}

void Sexy::WinFishApp::DoTimesUpDialog()
{
	CleanDialogs();
	DoDialogUnkF(DIALOG_TIMES_UP, true, "TIME\'S UP!", StrFormat("Your %d minutes are up!", mBoard->m0x3bc / 60), "Click for Results", Dialog::BUTTONS_FOOTER);
}

void Sexy::WinFishApp::DoRegisterDialog()
{
	if (mCompletedLoadingThreadTasks <= 29)
		return;

	if (!mIsRegistered)
	{
		RegisterDialog* aDia = new RegisterDialog(this);
		int aPrefHght = aDia->GetPreferredHeight(420);
		aDia->Resize((mWidth - 420) / 2, 32, 420, aPrefHght);
		AddDialog(DIALOG_REGISTER ,aDia);
	}
	else
	{
		DoDialog(DIALOG_INFO, true, "Already Registered", "You have already registered Insaniquarium.", "OK", Dialog::BUTTONS_FOOTER);
	}
}

void Sexy::WinFishApp::DoInvalidCodeDialog()
{
	DoDialog(DIALOG_INVALID_CODE, true, "Invalid Code", 
		"The license code you entered is not valid for that name.\n\nMake sure the name and registration number are entered correctly.", 
		"OK", Dialog::BUTTONS_FOOTER);
}

void Sexy::WinFishApp::DoDialogUnkF(int theId, bool isModal, const SexyString& theDiaHeader, const SexyString& theDiaLines, const SexyString& theDiaFooter, int theBtnMode)
{
	MoneyDialog* aDia = (MoneyDialog*) DoDialog(theId, isModal, theDiaHeader, theDiaLines, theDiaFooter, theBtnMode);
	aDia->DisableButtons(30);
}

void Sexy::WinFishApp::CleanDialogs()
{
	ApplyPrestoMorph(-1);
	KillDialog(32);
}

void Sexy::WinFishApp::ApplyPrestoMorph(int thePetId)
{
	KillDialog(DIALOG_PET_DIALOG);
	if (thePetId > -1 && mPrestoPtr && mBoard)
	{
		GameObject* aPet = mBoard->GetPrestoPet();
		if (aPet)
			aPet->PrestoMorph(thePetId);
	}
	mPrestoPtr = nullptr;
}

void Sexy::WinFishApp::SaveCurrentUserData()
{
	if (mCurrentProfile)
		mCurrentProfile->Save();
}

void Sexy::WinFishApp::SaveVirtualTankAndUserData()
{
	if (mGameMode != GAMEMODE_VIRTUAL_TANK)
		return;
	SaveCurrentUserData();
	if (mBoard)
		mBoard->SaveCurrentGame();
}

void Sexy::WinFishApp::ApplyGiveShells(bool doApply)
{
	UserDialog* aDia = (UserDialog*)GetDialog(DIALOG_GIVE_SHELLS);
	if (!aDia)
		return;

	if (doApply && mCurrentProfile)
	{
		int aShells = aDia->GetShellsValue();

		if (mCurrentProfile->mShells < aShells)
		{
			DoDialog(DIALOG_INFO, true, "Not Enough Shells", "You don\'t have that many shells to transfer.", "OK", Dialog::BUTTONS_FOOTER);
			return;
		}

		SexyString aSelUserName = aDia->GetSelectedUserName();
		UserProfile* aProf = mProfileMgr->GetUserProfile(aSelUserName);
		if (!aProf)
		{
			DoDialog(DIALOG_INFO, true, "Choose User", "Please choose a user from the list.", "OK", Dialog::BUTTONS_FOOTER);
			return;
		}
		if (aProf != mCurrentProfile)
		{
			aProf->LoadFromMemory();
			aProf->AddShells(aShells);
			mCurrentProfile->AddShells(-aShells);
			aProf->Save();
			mCurrentProfile->Save();
		}
	}

	KillDialog(DIALOG_GIVE_SHELLS);
}

void Sexy::WinFishApp::ApplyScreenSaverSettings()
{
	ScreenSaverDialog* aDia = (ScreenSaverDialog*)GetDialog(DIALOG_SCREENSAVER);
	if (aDia == nullptr)
		return;

	SexyString aSSFilePath = GetScreenSaverFilePath();

	if (!aDia->mSSEnabledCB->mChecked)
		StartScreenSaver();
	else
	{
		SexyString aFilePath = "";
		GetSystemScreenSaverPath(aFilePath);
		if (strcmp(aSSFilePath.c_str(), aFilePath.c_str()) != 0)
			mScreenSaverOldPath = aFilePath;

		if (FileExists(aSSFilePath))
		{
			bool copyFailed = GetBoolean("ScrCopy");
			if (copyFailed)
			{
				if (DoScrCopy(aSSFilePath))
					copyFailed = false;
			}

			if (copyFailed)
			{
				Popup("Failed to make screensaver.dat");
				mScreenSaverEnabled = false;
			}
			else
			{
                // !PORT
				//PVOID aParam = 0;
				//SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &aParam, 0);
				//if(aParam == 0)
				//	SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, 1, &aParam, 1);
				//SetScreenSaver(aSSFilePath.c_str());
				//mScreenSaverEnabled = true;
			}
		}
		else
			mScreenSaverEnabled = false;
	}

	mScreenSaverSound = aDia->mSSSoundCB->mChecked;
	mScreenSaverRotateBackdrops = aDia->mSSRotateBackdropsCB->mChecked;
	mScreenSaverPeriodicDim = aDia->mSSPeriodicDimCB->mChecked;
	mScreenSaverShowMoney = aDia->mSSShowMoneyCB->mChecked;
	mScreenSaverPowerSave = aDia->mSSPowerSaveCB->mChecked;

	if (!aDia->mSSEnabledCB->mChecked || mCurrentProfile == nullptr)
		mScreenSaverUserName = "";
	else
		mScreenSaverUserName = mCurrentProfile->mUserName;

	RegistryWriteString(mScreenSaverRegPath + "User", mScreenSaverUserName);
	RegistryWriteString(mScreenSaverRegPath + "OldPath", mScreenSaverOldPath);
	RegistryWriteBoolean(mScreenSaverRegPath + "Enabled", mScreenSaverEnabled);
	RegistryWriteBoolean(mScreenSaverRegPath + "Sound", mScreenSaverSound);
	RegistryWriteBoolean(mScreenSaverRegPath + "RotateBackDrops", mScreenSaverRotateBackdrops);
	RegistryWriteBoolean(mScreenSaverRegPath + "PeriodicDim", mScreenSaverPeriodicDim);
	RegistryWriteBoolean(mScreenSaverRegPath + "ShowMoney", mScreenSaverShowMoney);
	RegistryWriteBoolean(mScreenSaverRegPath + "Powersave", mScreenSaverPowerSave);
	KillDialog(DIALOG_SCREENSAVER);
}

void Sexy::WinFishApp::ApplyFishNameDialog()
{
	FishNamingDialog* aDia = (FishNamingDialog*)GetDialog(DIALOG_FISH_NAMING);
	SexyString aFishName = aDia->mEditWidget->mString;
	SexyString aWhitespaceChars = " \t\r\n";
	inlineTrim(aFishName);
	if (aFishName.length() == 0)
	{
		DoDialog(DIALOG_INFO, true, "Invalid Name", "Names must be one or more letters in length", "OK", Dialog::BUTTONS_FOOTER);
	}
	else
	{
		KillDialog(DIALOG_FISH_NAMING);
		if (mStoreScreen == nullptr)
		{
			if (mSimFishScreen)
			{
				mSimFishScreen->RenameSelectedFish(aFishName);
				mWidgetManager->BringToFront(mSimFishScreen);
			}
		}
		else
		{
			GameObject* aProduct = mStoreScreen->ConfirmPurchase();
			if (aProduct)
			{
				aProduct->mName = aFishName;
				InitializeSpecialNamedFish(aProduct);
				mBoard->SpawnGameObject(aProduct, true);
				mWidgetManager->BringToFront(mStoreScreen);
			}
		}
	}
}

void Sexy::WinFishApp::ApplySellGameObject(bool sell)
{
	if (mSimFishScreen)
		mSimFishScreen->SellSelectedObject(sell);
	else if (mSimSetupScreen)
		mSimSetupScreen->SellBackground(sell);
	KillDialog(DIALOG_SELL_DIALOG);
}

void Sexy::WinFishApp::ConfirmFishPurchase(bool confirmed)
{
	KillDialog(DIALOG_CONFIRM_PURCHASE);
	if (confirmed)
	{
		if (mStoreScreen)
		{
			mStoreScreen->ConfirmPurchase();
		}
		else if (mBonusScreen)
		{
			mBonusScreen->ConfirmPurchase();
		}
	}
}

void Sexy::WinFishApp::InitializeSpecialNamedFish(GameObject* theObject)
{
	if (stricmp(theObject->mName.c_str(), "1SingingFish") == 0)
	{
		theObject->mSinging = true;
	}
	else if (stricmp(theObject->mName.c_str(), "santa") == 0 && theObject->mType == TYPE_GUPPY)
	{
		for (GameObjectSet::iterator it = mBoard->mGameObjectSet.begin(); it != mBoard->mGameObjectSet.end(); ++it)
		{
			GameObject* anObj = *it;
			if (anObj->mPreNamedTypeId == SANTA)
				return;
		}
		Fish* aFish = (Fish*)theObject;
		if (!aFish->mHasSpecialColors && !aFish->mRainbowFish && aFish->mPreNamedTypeId == -1)
		{
			aFish->mPreNamedTypeId = SANTA;
			aFish->mHasSpecialColors = true;
			aFish->mColors[0] = Color(0xffffff);
			aFish->mColors[1] = Color(0xff0000);
			aFish->mColors[2] = Color(0xffffff);
			aFish->mSinging = true;
			aFish->mFoodNeededToGrow = 1;
		}
	}
}

bool Sexy::WinFishApp::CanAlienChaseAnyFish()
{
	if (mRelaxMode)
	{
		if (mUpdateCount - gUnkInt03 >= 360)
		{
			int aFishCnt = mBoard->mFishList->size();
			int anOscarCnt = mBoard->mOscarList->size();
			int anUltraCnt = mBoard->mUltraList->size();
			int aBreederCnt = mBoard->mBreederList->size();
			int aPentaCnt = mBoard->mPentaList->size();
			int aGrubberCnt = mBoard->mGrubberList->size();
			int aGekkoCnt = mBoard->mGekkoList->size();
			
			int allCnt = aFishCnt + anOscarCnt + anUltraCnt + aBreederCnt + aPentaCnt + aGrubberCnt + aGekkoCnt;
			if (allCnt > 1)
				return true;

			gUnkInt03 = mUpdateCount - 500;
		}
		return false;
	}
	return true;
}

bool Sexy::WinFishApp::DoScrCopy(SexyString& theScrSvrPath)
{
	SexyString aDatPath = "";
	if (CheckForVista())
	{
		aDatPath = GetAppDataFolder() + "/screensaver.dat";
	}
	else
	{
		aDatPath = GetFileDir(theScrSvrPath) + "/screensaver.dat";
	}

    // !PORT
	//FILETIME ft1;
	//FILETIME ft2;

	//bool isOk1 = GetFileLastWriteTime(theScrSvrPath.c_str(), &ft1);
	//bool isOk2 = GetFileLastWriteTime(aDatPath.c_str(), &ft2);

	// TODO
	return false;
}

void Sexy::WinFishApp::DoOptionsDialog(bool theFlag)
{
	CleanDialogs();
	OptionsDialog* aDia = new OptionsDialog(this, theFlag);
	int aPrefHght = aDia->GetPreferredHeight(348);
	aDia->Resize(143, 26, 348, aPrefHght);
	AddDialog(DIALOG_OPTIONS, aDia);
}

void Sexy::WinFishApp::SwitchToBoard(bool theFlag1, bool theFlag2)
{
	if (CheckTrialEnded())
	{
		SwitchToGameSelector();
		return;
	}
	RemoveBoard();
	if (mGameMode == GAMEMODE_VIRTUAL_TANK && !gFirstVirtualTankEnter)
	{
		gUnkBool05 = true;
		gFirstVirtualTankEnter = true;
		mWorkerThread->DoTask(LoadFishSongsTaskWrapper, 0);
	}
	if (theFlag1 && LoadBoardGame())
		return;

	for (int i = 0; i < 24; i++)
		mCurrentProfile->m0x5a[i] = false;

	if (theFlag2 && mGameMode == GAMEMODE_ADVENTURE && !mCurrentProfile->mFinishedGame && 
		mCurrentProfile->mTank == 1 && mCurrentProfile->mLevel < 4)
	{
		SwitchToHelpScreen(true);
		return;
	}
	if (!IsBonusLevel() && mGameMode != GAMEMODE_VIRTUAL_TANK && 
		(mCurrentProfile->mNumOfUnlockedPets > 3 && (mGameMode != GAMEMODE_ADVENTURE || mCurrentProfile->mTank != 5)))
	{
		SwitchToPetsScreen();
		return;
	}
	StartGame();
}

void Sexy::WinFishApp::DoQuitDialog()
{
#if !defined(__ANDROID__)
	Dialog* aDia = DoDialog(DIALOG_QUIT_GAME, true, "Quit", "Stop the insanity?", "", Dialog::BUTTONS_OK_CANCEL);
	aDia->mYesButton->mLabel = "Quit";
#endif
}

void Sexy::WinFishApp::SwitchToGameSelector()
{
	RemoveBoard();
	UpdatePlayData();
	if (mGameSelector)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
	}
	if (mCurrentProfile)
		mCurrentProfile->Unk01();

	mGameSelector = new GameSelector(this);
	mGameSelector->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameSelector);
	mWidgetManager->BringToBack(mGameSelector);
	mWidgetManager->SetFocus(mGameSelector);

	if (!mCurrentProfile)
	{
		DoNewUserDialog();
	}
	if (CheckTrialEnded())
		DoTrialVersionExpiredDialog();
}

void Sexy::WinFishApp::SwitchToHelpScreen(bool instructions)
{
	RemoveGameSelector();
	RemoveHelpScreen();
	mHelpScreen = new HelpScreen(this, instructions);
	mHelpScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHelpScreen);
}

void Sexy::WinFishApp::RemoveBoard()
{
	CleanDialogs();
	if (mBoard)
	{
		mBoard->SaveCurrentGame();
		if (mBoard->Unk02() > 120)
			m0x884 = true;

		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = 0;
	}
}

void Sexy::WinFishApp::RestartLevel()
{
	KillDialog(DIALOG_CONTINUE_GAME);
	KillDialog(DIALOG_RESTART_GAME);
	RemoveBoard();
	SexyString aSavePath = mCurrentProfile->GetSaveGameFilePath(mGameMode, mCurrentProfile->mId);
	EraseFile(aSavePath);
	StopMusic();
	PlayMusic(2, 0);
	if (mGameMode == GAMEMODE_ADVENTURE)
		SwitchToBoard(false, true);
	else
		SwitchToTankScreen();
}

void Sexy::WinFishApp::RemoveGameSelector()
{
	if (mGameSelector)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
		mGameSelector = 0;
	}
}

void Sexy::WinFishApp::UpdatePlayData()
{
	if (m0x884)
	{
		mPlaytimeCounter2++;
		m0x884 = false;
	}
	if (mMaxTime > 0)
	{
		int anUnk01 = mPlaytimeCounter * mFrameTime;
		mTimesPlayed = mTimesPlayed + anUnk01 / 60000;
		mPlaytimeCounter2 = 0;
		mPlaytimeCounter = 0;
		return;
	}
	if (mMaxPlays > 0)
	{
		mTimesPlayed += mPlaytimeCounter2;
	}
	mPlaytimeCounter2 = 0;
	mPlaytimeCounter = 0;
}

bool Sexy::WinFishApp::CheckTrialEnded()
{
	if (mIsRegistered || IsScreenSaver())
		return false;
	UpdatePlayData();

	if (mMaxTime > 0)
		if(mMaxTime <= mTimesPlayed)
			return true;

	if (mMaxTime <= 0 && mMaxPlays > 0)
	{
		int aPlaysLeft = mMaxPlays - mTimesPlayed;

		if (m0x884)
			aPlaysLeft--;

		if (aPlaysLeft <= 0)
			return true; 
	}

	if (mMaxTime <= 0 && mMaxPlays <= 0 && mMaxExecutions > 0)
		if (mTimesExecuted > mMaxExecutions)
			return true;

	return false;
}

void Sexy::WinFishApp::ApplyOptionsSettings()
{
	OptionsDialog* aDia = (OptionsDialog*)GetDialog(DIALOG_OPTIONS);
	if (!aDia)
		return;
	SetWidescreen(aDia->mWidescreenCB->IsChecked());
	mQuickAim = aDia->mQuickAimCB->IsChecked();
	mRumble = aDia->mRumbleCB->IsChecked();
	mFastForward = aDia->mFastForwardCB->IsChecked();
	EnableCustomCursors(aDia->mCustomCursorsCB->IsChecked());
	KillDialog(DIALOG_OPTIONS);
	ClearUpdateBacklog(true);
	if (mBoard)
		mBoard->ApplyShadowsIf3D();

    // !PORT: registry normally saved at end, won't work for emscripten
    WriteToRegistry();
}

void Sexy::WinFishApp::DoTrialVersionExpiredDialog()
{
	Dialog* aDia = DoDialog(3, true, "PLEASE REGISTER!", "Your trial version of Insaniquarium has expired!\n\nYou must register your copy\nto continue playing.", "", Dialog::BUTTONS_OK_CANCEL);
	aDia->mYesButton->mLabel = "Register";
	aDia->mNoButton->mLabel = "Quit";
}

void Sexy::WinFishApp::DoUpdateDialog()
{
	Dialog* aDia = DoDialog(DIALOG_UPDATE_ASK, true, "Updates", "Do you want to check for updates to Insaniquarium? New versions may offer new features and bug fixes.  This requires an active Internet connection.", "", Dialog::BUTTONS_YES_NO);
	int aPrefHeight = aDia->GetPreferredHeight(348);
	aDia->Resize(146, 50, 348, aPrefHeight);
}

void Sexy::WinFishApp::DoContinueDialog()
{
	ContinueDialog* aDia = new ContinueDialog(this);
	int aPrefHght = aDia->GetPreferredHeight(380);
	aDia->Resize((mWidth - 380) / 2, 70, 380, aPrefHght);
	AddDialog(DIALOG_CONTINUE_GAME, aDia);
}

void Sexy::WinFishApp::DoNewUserDialog()
{
	KillDialog(DIALOG_NEW_USER);
	NewUserDialog* aDia = new NewUserDialog(this, false);
	int aPrefHght = aDia->GetPreferredHeight(400);
	aDia->Resize((mWidth - 400) / 2, (mHeight - aPrefHght) / 2, 400, aPrefHght);
	AddDialog(DIALOG_NEW_USER, aDia);
}

void Sexy::WinFishApp::StartGame()
{
	mGameNotPlayed = false;
	if (mGameMode == GAMEMODE_VIRTUAL_TANK)
	{
		if (LoadBoardGame())
			return;
	}
	CreateBoard();
	mBoard->InitLevel();
	if (IsBonusLevel())
	{
		mBoard->InitBonusLevel();
		StartGameMusic();
		return;
	}

	if (mGameMode == GAMEMODE_VIRTUAL_TANK)
	{
		mBoard->StartVirtualTank();
		StartGameMusic();
		return;
	}

	mBoard->StartGame();
	StartGameMusic();
}

bool Sexy::WinFishApp::LoadBoardGame()
{
	CreateBoard();
	bool aSuccess = mBoard->LoadGame(mCurrentProfile->GetSaveGameFilePath(mGameMode, mCurrentProfile->mId));
	if (!aSuccess)
		RemoveBoard();
	else
	{
		mGameNotPlayed = false;
		if (mGameMode != GAMEMODE_VIRTUAL_TANK)
			DoContinueDialog();
	}
	return aSuccess;
}

void Sexy::WinFishApp::MakeNewUser(bool makeUser)
{
	NewUserDialog* aDia =(NewUserDialog*) GetDialog(DIALOG_NEW_USER);
	if (!aDia)
		return;

	SexyString aNewUserName = aDia->CleanString();

	if (makeUser && aNewUserName.size() == 0)
	{
		DoDialog(DIALOG_INFO_NEW_USER, true, "Enter Your Name", "Please enter your name to create a new user profile for storing high score data and game progress.", "OK", Dialog::BUTTONS_FOOTER);
		return;
	}
	else
	{
		if (mCurrentProfile || (makeUser && aNewUserName.size() != 0))
		{
			if (!makeUser)
			{
				KillDialog(DIALOG_NEW_USER);
				return;
			}
			else
			{
				UserProfile* aProf = mProfileMgr->MakeNewUser(&aNewUserName);
				if (!aProf)
				{
					DoDialog(DIALOG_INFO_NEW_USER, true, "Name Conflict",
						"The name you entered is already being used.  Please enter a unique player name.", 
						"OK", Dialog::BUTTONS_FOOTER);
					return;
				}
				else
				{
					mProfileMgr->SaveUsersDat();
					mCurrentProfile = aProf;
					KillDialog(DIALOG_USER_DIALOG);
					KillDialog(DIALOG_NEW_USER);
					mWidgetManager->MarkAllDirty();
					if (mGameSelector != nullptr)
                        return;
				}
			}
		}

		DoDialog(DIALOG_INFO_NEW_USER, true, "Enter Your Name",
			"Please enter your name to create a new user profile for storing high score data and game progress.", 
			"OK", Dialog::BUTTONS_FOOTER);
		return;
	}
}

void Sexy::WinFishApp::CreateBoard()
{
	RemoveBoard();
	m0x884 = false;
	// switchfish: --level, once
	if (gStartTank > 0 && mGameMode == GAMEMODE_ADVENTURE && mCurrentProfile)
	{
		mCurrentProfile->mTank = gStartTank;
		mCurrentProfile->mLevel = gStartLevel;
		gStartTank = 0;
	}
	// switchfish: a changed widescreen setting reaches the tank when a new one is made
	bool aWide = mWidescreenAware && mWideTank;
	if (aWide != (TankWidth() > 640))
	{
		SetWideTank(aWide);
		BuildWideBackdrops(this);
	}
	mBoard = new Board(this);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mCurrentProfile->m0x4c++;
}

bool Sexy::WinFishApp::IsBonusLevel()
{
	if (mGameMode == GAMEMODE_ADVENTURE && mCurrentProfile->mLevel == 6)
		return true;
	if (mBoard)
		return mBoard->mIsBonusRound;
	return false;
}

bool Sexy::WinFishApp::IsSongPlaying(int theSongId, int theSongOffset)
{
	return mCurrentSongId == theSongId && mCurrentSongOffset == theSongOffset;
}

void Sexy::WinFishApp::StopMusic()
{
	if (!mMusicInterface)
		return;

	mMusicInterface->StopMusic(0);
	mMusicInterface->StopMusic(1);
	mMusicInterface->StopMusic(2);
	mMusicInterface->StopMusic(3);
	mMusicInterface->StopMusic(4);
	mCurrentSongId = -1;
	mCurrentSongOffset = -1;
}

void Sexy::WinFishApp::StartGameMusic()
{
	StopMusic();
	if (mGameMode == GAMEMODE_VIRTUAL_TANK)
		return;

	if (IsBonusLevel())
		PlayMusic(0, 13);
	else if(GetCurrentTank() == 1)
		PlayMusic(0, 22);
	else if(GetCurrentTank() == 2)
		PlayMusic(0, 0);
	else if(GetCurrentTank() == 3)
		PlayMusic(2, 32);
	else if(GetCurrentTank() == 4)
		PlayMusic(2, 45);
}

void Sexy::WinFishApp::SomeMusicPlayFunc(bool flag)
{
	if (mGameMode == GAMEMODE_VIRTUAL_TANK)
	{
		StopMusic();
		return;
	}

	if (!flag)
		StopMusic();
	else
		FadeOutMusic(true, true, 0.008);

	double aSpeed = flag ? 0.002 : 1.0;
	int aTank = GetCurrentTank();
	if (aTank == 1)
		FadeInMusic(0, 22, aSpeed, false);
	else if(aTank == 2)
		FadeInMusic(0, 0, aSpeed, false);
	else if(aTank == 3)
		FadeInMusic(2, 32, aSpeed, false);
	else if(aTank == 4)
		FadeInMusic(2, 45, aSpeed, false);
}

bool Sexy::WinFishApp::ChangeDirHook(const char* theIntendedPath)
{
	// prevent changing directories while testing
	return true;

    if (!IsScreenSaver())
        return false;

    SexyString aRegPath = mScreenSaverRegKey + "Directory";
    SexyString aDirectoryPath;

    if (RegistryReadString(aRegPath, &aDirectoryPath))
        if (ChDir(aDirectoryPath.c_str()))
            return true;

    return false;
}

void Sexy::WinFishApp::LogScreenSaverError(const std::string& theError)
{
	if (!gScreenSaverStarted)
		StartScreenSaver();
	SexyApp::LogScreenSaverError(theError);
}

// switchfish: beside a 4:3 screen, the water the help screen is drawn on: its gradient, slow bubbles, and a
// soft shadow along the screen so it seems to float above the water
bool Sexy::WinFishApp::DrawSides(Graphics* g, const Rect& theLeft, const Rect& theRight)
{
	// once per game tick, when the bubbles have moved
	if (mSidesDrawnAt == mUpdateCount)
	{
		return false;
	}
	mSidesDrawnAt = mUpdateCount;
	static uint32_t sBands[120];
	if (sBands[0] == 0)
	{
		int aLumin = 107;
		for (int i = 0; i < 120; i++)
		{
			sBands[i] = HSLToRGB(151, 138, aLumin);
			if (--aLumin < 50)
			{
				aLumin = 50;
			}
		}
	}
	const Rect* aSides[2] = {&theLeft, &theRight};
	for (int s = 0; s < 2; s++)
	{
		const Rect& aRect = *aSides[s];
		for (int i = 0; i < 120 && i * 4 < aRect.mHeight; i++)
		{
			g->SetColor(Color(sBands[i]));
			g->FillRect(aRect.mX, i * 4, aRect.mWidth, 4);
		}
		if (mSideBubbles[s])
		{
			// BubbleMgr draws its bubbles unclipped; keep them out of the screen in the middle
			Graphics aClipG(*g);
			aClipG.ClipRect(aRect);
			mSideBubbles[s]->Draw(&aClipG);
		}
		const int aShadow = 16;
		for (int k = 0; k < aShadow && k < aRect.mWidth; k++)
		{
			int anAlpha = 140 * (aShadow - k) * (aShadow - k) / (aShadow * aShadow);
			int x = s == 0 ? aRect.mX + aRect.mWidth - 1 - k : aRect.mX + k;
			g->SetColor(Color(0, 0, 0, anAlpha));
			g->FillRect(x, aRect.mY, 1, aRect.mHeight);
		}
	}
	return true;
}

void Sexy::WinFishApp::UpdateFrames()
{
	// switchfish: the bubbles beside a 4:3 screen (DrawSides), once the images are loaded
	if (mLoadingThreadCompleted && mGLInterface && !mGLInterface->mFullCanvas && mGLInterface->mWidth > mWidth)
	{
		int aMargin = (mGLInterface->mWidth - mWidth) / 2;
		for (int s = 0; s < 2; s++)
		{
			if (!mSideBubbles[s])
			{
				mSideBubbles[s] = new BubbleMgr();
				mSideBubbles[s]->mBubbleBounds = Rect(s == 0 ? 0 : aMargin + mWidth, 0, aMargin, 480);
				mSideBubbles[s]->SetBubbleConfig(4, 3);
				mSideBubbles[s]->UpdateALot();
			}
			mSideBubbles[s]->Update();
		}
	}

	// switchfish: fast-forward, R held (an Options setting): two ticks per frame time while a level is being
	// played, never in menus, dialogs, pause or the Virtual Tank (it would speed up earning shells there)
	bool aFast = mFastForward && mFastForwardHeld && mBoard && mBoard->mVisible && !mBoard->mPause &&
		mDialogList.empty() && mGameMode != GAMEMODE_VIRTUAL_TANK;
	mUpdateMultiplier = aFast ? 2.0 : 1.0;

	// switchfish: the store, pets, fish and tank screens lie over the paused board (Board::SetCovered)
	if (mBoard)
	{
		bool aCovered = mPetsScreen || mStoreScreen || mSimSetupScreen || mSimFishScreen || mTankScreen;
		if (aCovered != mBoard->mCovered)
			mBoard->SetCovered(aCovered);
	}

	// switchfish: --screen opens a screen that takes long to reach, once the main menu is up
	if (!gTestScreen.empty() && mGameSelector && !mTitleScreen)
	{
		std::string aScreen = gTestScreen;
		gTestScreen.clear();
		if (aScreen == "credits")
		{
			RemoveGameSelector();
			SwitchToInterludeScreen();
		}
		else if (aScreen == "story")
		{
			RemoveGameSelector();
			SwitchToStoryScreen(-1);
		}
	}

	// switchfish: --quickplay, N finishes the level whichever widget has the focus
	if (gQuickPlay)
	{
		static bool sWasDown = false;
		bool aDown = mWidgetManager->mKeyDown['N'];
		if (aDown && !sWasDown && mBoard && mWidgetManager->mBaseModalWidget == nullptr)
			mBoard->QuickFinishLevel();
		sWasDown = aDown;
	}

	// switchfish: the first frame uploads every texture, so don't let the music run over a black screen
	if (mTitleMusicPending && mDrawCount > 0)
	{
		mTitleMusicPending = false;
		SDL_Log("boot: first frame");
		PlayMusic(2, 45);
	}

	if (!gUnkBool01)
	{
		if (gDoHundredUpdates)
		{
			for(int i = 0; i < 100; i++)
				SexyApp::UpdateFrames();
			return;
		}
	}
	else
	{
		if (gUpdateFramesCounter < 3)
		{
			gUpdateFramesCounter++;
			return;
		}
		gUpdateFramesCounter = 0;
	}

	SexyApp::UpdateFrames();
}

void Sexy::WinFishApp::TitleScreenIsFinished()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	mResourceManager->ReplaceImage("IMAGE_TITLEPAGE", 0);
	ForgetWideScreenBackdrop("titlescreen");
	mResourceManager->ReplaceImage("IMAGE_LOADERBAR", 0);
	mResourceManager->ReplaceImage("IMAGE_LOADERBAROVER", 0);
	mResourceManager->ReplaceImage("IMAGE_LOADERBAROVER2", 0);
	mResourceManager->ReplaceImage("IMAGE_LOADERPLAY", 0);
	mResourceManager->ReplaceImage("IMAGE_TITLEPAGEMASK", 0);
	mResourceManager->ReplaceImage("IMAGE_TITLEPAGEMASK", 0);

	if (IsScreenSaver())
	{
		mGameMode = GAMEMODE_VIRTUAL_TANK;
		SwitchToBoard(true, true);
		ClearUpdateBacklog();
		return;
	}

	SwitchToGameSelector();
}

void Sexy::WinFishApp::WFAShowResourceError(bool doExit)
{
	if (!IsScreenSaver())
	{
		ShowResourceError(doExit);
		return;
	}

	SexyString aErrText = mResourceManager->GetErrorText();
	LogScreenSaverError(aErrText);
	mLoadingFailed = true;
}

void Sexy::WinFishApp::SwitchToHighScoreScreen()
{
	RemoveGameSelector();
	RemoveHighScoreScreen();
	mHighScoreScreen = new HighScoreScreen(this);
	mHighScoreScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHighScoreScreen);
}

void Sexy::WinFishApp::SwitchToPetsScreen()
{
	CleanDialogs();
	RemovePetsScreen();
	if (mBoard)
		mBoard->PauseGame(true);
	mPetsScreen = new PetsScreen(this);
	mPetsScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mPetsScreen);
}

void Sexy::WinFishApp::SwitchToHatchScreen(int thePetId)
{
	CleanDialogs();
	m0x884 = true;
	if (mBoard)
		mBoard->mShouldSave = false;
	RemoveHatchScreen();
	mHatchScreen = new HatchScreen(this, thePetId);
	mHatchScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHatchScreen);
	SaveCurrentUserData();
	RemoveBoard();
}

void Sexy::WinFishApp::SwitchToInterludeScreen()
{
	CleanDialogs();
	RemoveInterludeScreen();
	mInterludeScreen = new InterludeScreen(this, 0);
	mInterludeScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mInterludeScreen);
	mWidgetManager->SetFocus(mInterludeScreen);
	if (mBoard == nullptr)
		mWidgetManager->BringToBack(mInterludeScreen);
	else
		mWidgetManager->PutInfront(mInterludeScreen, mBoard);
}
void Sexy::WinFishApp::SwitchToSimSetupScreen()
{
	ReadSSFromRegistry();
	CleanDialogs();
	RemoveSimSetupScreen();
	if (mBoard)
		mBoard->PauseGame(true);

	mSimSetupScreen = new SimSetupScreen(this);
	mSimSetupScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mSimSetupScreen);
}
void Sexy::WinFishApp::SwitchToTankScreen()
{
	CleanDialogs();
	RemoveTankScreen();

	mTankScreen = new TankScreen(this);
	mTankScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTankScreen);
	if (mBoard == nullptr)
		mWidgetManager->BringToBack(mTankScreen);
	else
		mWidgetManager->PutInfront(mTankScreen, mBoard);
}

void Sexy::WinFishApp::SwitchToStoryScreen(int unk)
{
	CleanDialogs();
	RemoveStoryScreen();

	int aStoryId = -1;
	int aGroupIdx = unk - 1;
	int isAdventure = false;
	if (aGroupIdx < 0)
		aGroupIdx = 0;
	else if (aGroupIdx < 4)
		isAdventure = true;
	else
		aGroupIdx = 3;

	static int aStoryTable[32] = {
		0, 1, 2, 3, 4, 20, 24, 25,
		5, 6, 7, 8, 9, 21, 26, 27,
		10, 11, 12, 13, 14, 22, 28, 29,
		15, 16, 17, 18, 19, 23, 30, 31
	};

	aStoryId = -2;
	bool storyFound = false;
	if (isAdventure)
	{
		if (mCurrentProfile->m0x7c == -1)
		{
			if (mCurrentProfile->m0x80 == 0 && mCurrentProfile->mBonusItemId > 5)
			{
				aStoryId = 32;
                mCurrentProfile->m0x80 = 1;
				SaveCurrentUserData();
				storyFound = true;
			}
			else
			{
				int aLimit = 33;
				if (mCurrentProfile->m0x80 != 1) aLimit = 32;
				aStoryId = Rand() % aLimit;
				if (aStoryId != -1)
					storyFound = true;
			}
		}

		int* aGroupStart = &aStoryTable[aGroupIdx * 8];
		if (!storyFound)
		{
			for (int i = 0; i < 8; i++)
			{
				int anId = aGroupStart[i];
				if ((mCurrentProfile->m0x7c & (1 << (anId & 0x1F))) == 0 &&
					(anId > 23 || mCurrentProfile->IsPetUnlocked(anId)))
				{
					aStoryId = anId;
					mCurrentProfile->m0x7c |= (1 << (anId & 0x1F));
					SaveCurrentUserData();
					if(aStoryId != -1)
						storyFound = true;
					break;
				}
			}
		}

		if (!storyFound)
		{
			std::vector<int> aSeenStories;
			for (int i = 0; i < 8; i++)
			{
				int anId = aGroupStart[i];
				bool seen = (mCurrentProfile->m0x7c & (1 << (anId & 0x1F))) != 0;
				if (seen)
				{
					aSeenStories.push_back(anId);
				}
			}

			if (aSeenStories.empty())
				aSeenStories.push_back(0);

			int aRandIdx = Rand() % aSeenStories.size();
			aStoryId = aSeenStories[aRandIdx];
		}
	}

	mStoryScreen = new StoryScreen(this, aStoryId);
	mStoryScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mStoryScreen);
	mWidgetManager->SetFocus(mStoryScreen);
}

void Sexy::WinFishApp::SwitchToStoreScreen()
{
	CleanDialogs();
	RemoveStoreScreen();
	if (mBoard)
		mBoard->PauseGame(true);

	if (mCurrentProfile)
		mCurrentProfile->Unk01();

	mStoreScreen = new StoreScreen(this);
	mStoreScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mStoreScreen);
	mWidgetManager->SetFocus(mStoreScreen);
}

void Sexy::WinFishApp::SwitchToSimFishScreen()
{
	CleanDialogs();
	RemoveSimFishScreen();
	if (mBoard)
		mBoard->PauseGame(true);

	mSimFishScreen = new SimFishScreen(this);
	mSimFishScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mSimFishScreen);
	mWidgetManager->SetFocus(mSimFishScreen);
}

void Sexy::WinFishApp::SwitchToBonusScreen()
{
	CleanDialogs();
	m0x884 = true;
	if (mBoard)
		mBoard->mShouldSave = false;
	RemoveBonusScreen();
	if (mCurrentProfile)
		mCurrentProfile->Unk01();

	mBonusScreen = new BonusScreen(this);
	mBonusScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBonusScreen);
	SaveCurrentUserData();
	RemoveBoard();
	StopMusic();
	PlayMusic(2, 54);
}

void Sexy::WinFishApp::RemoveHighScoreScreen()
{
	if (mHighScoreScreen)
	{
		mWidgetManager->RemoveWidget(mHighScoreScreen);
		SafeDeleteWidget(mHighScoreScreen);
		mHighScoreScreen = nullptr;
		SwitchToGameSelector();
	}
}

void Sexy::WinFishApp::RemovePetsScreen()
{
	if (mPetsScreen)
	{
		mWidgetManager->RemoveWidget(mPetsScreen);
		SafeDeleteWidget(mPetsScreen);
		mPetsScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveHatchScreen()
{
	if (mHatchScreen)
	{
		mWidgetManager->RemoveWidget(mHatchScreen);
		SafeDeleteWidget(mHatchScreen);
		mHatchScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveSimSetupScreen()
{
	if (mSimSetupScreen)
	{
		mWidgetManager->RemoveWidget(mSimSetupScreen);
		SafeDeleteWidget(mSimSetupScreen);
		mSimSetupScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveTankScreen()
{
	if (mTankScreen)
	{
		mWidgetManager->RemoveWidget(mTankScreen);
		SafeDeleteWidget(mTankScreen);
		mTankScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveStoryScreen()
{
	if (mStoryScreen)
	{
		mWidgetManager->RemoveWidget(mStoryScreen);
		SafeDeleteWidget(mStoryScreen);
		mStoryScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveStoreScreen()
{
	if (mStoreScreen)
	{
		mWidgetManager->RemoveWidget(mStoreScreen);
		SafeDeleteWidget(mStoreScreen);
		mStoreScreen = nullptr;
		if (mBoard != nullptr)
			mWidgetManager->SetFocus(mBoard);
	}
}

void Sexy::WinFishApp::RemoveSimFishScreen()
{
	if (mSimFishScreen)
	{
		mWidgetManager->RemoveWidget(mSimFishScreen);
		SafeDeleteWidget(mSimFishScreen);
		mSimFishScreen = nullptr;
		if (mBoard != nullptr)
			mWidgetManager->SetFocus(mBoard);
	}
}

void Sexy::WinFishApp::RemoveBonusScreen()
{
	if (mBonusScreen)
	{
		mWidgetManager->RemoveWidget(mBonusScreen);
		SafeDeleteWidget(mBonusScreen);
		mBonusScreen = nullptr;
	}
}

void Sexy::WinFishApp::RemoveInterludeScreen()
{
	if (mInterludeScreen)
	{
		mWidgetManager->RemoveWidget(mInterludeScreen);
		SafeDeleteWidget(mInterludeScreen);
		mInterludeScreen = nullptr;
	}
}

void Sexy::WinFishApp::LeaveGameBoard()
{
	SaveCurrentUserData();
	ApplyOptionsSettings();
	RemoveBoard();
	SwitchToGameSelector();
}

void Sexy::WinFishApp::RemoveHelpScreen()
{
	if (mHelpScreen)
	{
		mWidgetManager->RemoveWidget(mHelpScreen);
		SafeDeleteWidget(mHelpScreen);
		mHelpScreen = 0;
	}
}

void Sexy::WinFishApp::DoUpdateCheckDialog()
{
	UpdateCheckDialog* aDialog = new UpdateCheckDialog(IMAGE_DIALOG, IMAGE_DIALOGBUTTON, IMAGE_WAIT_BAR, DIALOG_UPDATE_CHECK);
	DefaultDialogSettings(aDialog);
	int aPrefHght = aDialog->GetPreferredHeight(348);
	aDialog->Resize(146, 50, 348, aPrefHght);
	AddDialog(DIALOG_UPDATE_CHECK, aDialog);
	SexyString anUrl = "http://www.popcap.com/win32updatecheck.php?prod=" + mProdName + 
		"&ver=" + mProductVersion
		+ "&referid=" + std::to_string(mWinFishReferId);
	//mInternetManager->StartUpdateCheck(anUrl);
}

void Sexy::WinFishApp::DoWhoAreYouDialog()
{
	KillDialog(DIALOG_USER_DIALOG);
	UserDialog* aDia = new UserDialog(this, false);
	int aPrefHght = aDia->GetPreferredHeight(400);
	aDia->Resize(0, 17, 400, aPrefHght);
	AddDialog(DIALOG_USER_DIALOG, aDia);
}

void Sexy::WinFishApp::DoGiveDialog()
{
	KillDialog(DIALOG_GIVE_SHELLS);
	if (mCurrentProfile && (mCurrentProfile->mFinishedGame || mCurrentProfile->mTank != 1))
	{
		UserDialog* aDia = new UserDialog(this, true);
		int aPrefHght = aDia->GetPreferredHeight(400);
		aDia->Resize(0, 100, 400, aPrefHght);
		AddDialog(DIALOG_GIVE_SHELLS, aDia);
		return;
	}

	DoDialog(DIALOG_INFO, true, "Not Allowed", "You need to beat the first tank before you can transfer shells.", "OK", Dialog::BUTTONS_FOOTER);
}

void Sexy::WinFishApp::DoDeleteWarningDialog(SexyString& theName)
{
	KillDialog(DIALOG_ARE_YOU_SURE_DELETE);
	DoDialog(DIALOG_ARE_YOU_SURE_DELETE, true, "Are You Sure?", StrFormat("This will permanently remove \'%s\' from the player roster!", theName.c_str()), "", Dialog::BUTTONS_YES_NO);
}

void Sexy::WinFishApp::RenameUser(bool makeUser)
{
	if (!makeUser)
	{
		KillDialog(DIALOG_RENAME);
		return;
	}

	UserDialog* anUserDialog = (UserDialog*)GetDialog(DIALOG_USER_DIALOG);
	NewUserDialog* aRenameDialog = (NewUserDialog*)GetDialog(DIALOG_RENAME);

	if (!anUserDialog || !aRenameDialog)
		return;

	SexyString anOldName = anUserDialog->GetSelectedUserName();
	SexyString aNewName = aRenameDialog->CleanString();

	if (aNewName.size() > 0)
	{
		UserProfile* aProf = mProfileMgr->GetUserProfile(anOldName);
		UserProfile* aCurProf = mCurrentProfile;
		bool aSuccess = mProfileMgr->RenameUser(anOldName, aNewName);
		if (!aSuccess)
		{
			DoDialog(DIALOG_NAME_CONFLICT, true, "Name Conflict", "The name you entered is already being used.  Please enter a unique player name.", "OK", Dialog::BUTTONS_FOOTER);
			return;
		}

		mProfileMgr->SaveUsersDat();
		if (aProf == aCurProf)
			mCurrentProfile = mProfileMgr->GetUserProfile(aNewName);

		int aSelIdx = anUserDialog->mListWidget->mSelectIdx;
		if (aSelIdx > 0)
			anUserDialog->mListWidget->SetLine(aSelIdx, aNewName);

		mWidgetManager->MarkAllDirty();
		KillDialog(DIALOG_RENAME);
	}
}

void Sexy::WinFishApp::DeleteUser(bool deleteUser)
{
	KillDialog(DIALOG_ARE_YOU_SURE_DELETE);
	UserDialog* aDia = (UserDialog*)GetDialog(DIALOG_USER_DIALOG);
	if (!deleteUser || !aDia)
		return;

	SexyString aUserName = "";
	if (mCurrentProfile)
		aUserName = mCurrentProfile->mUserName;

	SexyString aSelUserName = aDia->GetSelectedUserName();

	if (stricmp(aUserName.c_str(), aSelUserName.c_str()) == 0)
		mCurrentProfile = nullptr;
	mProfileMgr->DeleteUser(aSelUserName);
	int aSelIdx = aDia->mListWidget->mSelectIdx;
	aDia->mListWidget->RemoveLine(aSelIdx);
	aSelIdx--;
	if (aSelIdx < 1)
		aSelIdx = 1;
	if (aDia->mListWidget->GetLineCount() > 1)
		aDia->mListWidget->SetSelect(aSelIdx);
	if (!mCurrentProfile)
	{
		SexyString aNextUserName = aDia->GetSelectedUserName();
		mCurrentProfile = mProfileMgr->GetUserProfile(aNextUserName);
		if (!mCurrentProfile)
			mCurrentProfile = mProfileMgr->GetFirstUserProfile();
	}
	mProfileMgr->SaveUsersDat();
	if (!mCurrentProfile)
		DoNewUserDialog();
	mWidgetManager->MarkAllDirty();
}

void Sexy::WinFishApp::DoRenameDialog(SexyString theUserName)
{
	KillDialog(DIALOG_RENAME);
	NewUserDialog* aRenameDialog = new NewUserDialog(this, true);
	int aPrefHght = aRenameDialog->GetPreferredHeight(400);
	aRenameDialog->Resize((mWidth - 400) / 2, (mHeight - aPrefHght) / 2, 400, aPrefHght);
	aRenameDialog->mEditWidget->SetText(theUserName, true);
	aRenameDialog->mEditWidget->mCursorPos = theUserName.size();
	aRenameDialog->mEditWidget->mHilitePos = 0;
	AddDialog(DIALOG_RENAME, aRenameDialog);
}

void Sexy::WinFishApp::UserDialogOkPressed(bool applyChanges)
{
	UserDialog* aDia =(UserDialog*) GetDialog(DIALOG_USER_DIALOG);
	if (!aDia)
		return;

	if (applyChanges)
	{
		SexyString aUserName = aDia->GetSelectedUserName();
		UserProfile* aProf = mProfileMgr->GetUserProfile(aUserName);
		if (aProf)
		{
			mCurrentProfile = aProf;
			if (mCurrentProfile->mTank == 5 && mCurrentProfile->mLevel > 1)
			{
				mCurrentProfile->mTank = 1;
				mCurrentProfile->mLevel = 1;
				mCurrentProfile->mFinishedGameCount = 0;
				mCurrentProfile->m0x58 = false;
			}
		}
	}

	KillDialog(DIALOG_USER_DIALOG);
}

int Sexy::WinFishApp::GetCurrentTank()
{
	if (mBoard)
		return mBoard->mTank;
	return mCurrentProfile->mTank;
}

void Sexy::WinFishApp::CloseRequestAsync()
{
	Shutdown();
}
