#include "WinFishApp.h"
#include "WinFishCommon.h"
#include "Notice.h"
#include "TestPlan.h"
#include "PadTargets.h"
#include "platform/PadCursor.h"
#include <SDL2/SDL.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

static bool IsGameDirectory(const std::filesystem::path& p)
{
    return Sexy::FileExists((p / "properties" / "resources.xml").string());
}

static const char* Arg(int argc, char** argv, const char* name)
{
    for (int i = 1; i + 1 < argc; i++)
    {
        if (strcmp(argv[i], name) == 0)
        {
            return argv[i + 1];
        }
    }
    return nullptr;
}

// --quickplay (or a quickplay.txt in the game folder, for the Switch) and --level T-L, see WinFishCommon.h
static void ReadTestFlags(int argc, char** argv)
{
    std::string level;
    if (const char* arg = Arg(argc, argv, "--level"))
    {
        level = arg;
        Sexy::gQuickPlay = true;
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--quickplay") == 0)
        {
            Sexy::gQuickPlay = true;
        }
    }
    if (FILE* f = fopen("quickplay.txt", "r"))
    {
        char buf[16] = {};
        if (fgets(buf, sizeof(buf), f) && level.empty())
        {
            level = buf;
        }
        fclose(f);
        Sexy::gQuickPlay = true;
    }
    int tank = 0, lvl = 0;
    if (sscanf(level.c_str(), "%d-%d", &tank, &lvl) == 2 && tank >= 1 && tank <= 5 && lvl >= 1 && lvl <= 5)
    {
        Sexy::gStartTank = tank;
        Sexy::gStartLevel = lvl;
    }
    if (Sexy::gQuickPlay)
    {
        SDL_Log("quickplay: N finishes the level%s", Sexy::gStartTank ? ", starting at the given level" : "");
    }
}

static const Notice kAlbumNotice = {
    "Started from the album",
    "Homebrew started from the album gets little memory and no keyboard for player names.\n\n"
    "For the full game, hold R while starting any game, then pick switchfish in the Homebrew Menu.",
    true};

#if defined(__SWITCH__)
#include <switch.h>
#include <netinet/in.h>

extern "C" {
    u32 __nx_stack_size = 4 * 1024 * 1024;
}

static const char* kDataPath = "sdmc:/switch/switchfish";

static bool IsAlbumMode()
{
    AppletType type = appletGetAppletType();
    return type != AppletType_Application && type != AppletType_SystemApplication;
}

static std::string GameDataPath(int, char**)
{
    return kDataPath;
}

static Notice MissingDataNotice(const std::string& path)
{
    return {"Game files not found",
            "Copy the folders from your Insaniquarium Deluxe install (properties, images, sounds, music, data, "
            "fishsongs) to\n\n" + path + "/",
            false};
}

static bool gNxlink = false;

static void StartLog()
{
    // started with ./sf.sh send: the log also goes to the Mac's terminal (nxlink -s)
    if (__nxlink_host.s_addr != 0 && R_SUCCEEDED(socketInitializeDefault()))
    {
        gNxlink = nxlinkStdio() >= 0;
    }
    static FILE* logfile = fopen("sdmc:/switch/switchfish/log.txt", "wb");
    SDL_LogSetOutputFunction([](void* userdata, int, SDL_LogPriority, const char* message)
    {
        if (!message)
        {
            return;
        }
        // seconds since start, to see where the time goes
        static const u64 start = armGetSystemTick();
        double t = armTicksToNs(armGetSystemTick() - start) / 1e9;
        if (userdata)
        {
            fprintf((FILE*)userdata, "%7.3f %s\n", t, message);
            fflush((FILE*)userdata);
        }
        if (gNxlink)
        {
            printf("%7.3f %s\n", t, message);
            fflush(stdout);
        }
    }, logfile);
}

#else

static bool IsAlbumMode()
{
    return false;
}

// --data <dir>, then $SWITCHFISH_DATA, then ./gameData
static std::string GameDataPath(int argc, char** argv)
{
    if (const char* arg = Arg(argc, argv, "--data"))
    {
        return arg;
    }
    if (const char* env = getenv("SWITCHFISH_DATA"))
    {
        return env;
    }
    return "gameData";
}

static Notice MissingDataNotice(const std::string& path)
{
    return {"Game files not found",
            "Nothing found in " + path + ".\n\nCopy the folders from your Insaniquarium Deluxe install "
            "(properties, images, sounds, music, data, fishsongs) into gameData.",
            false};
}

static void StartLog()
{
}

#endif

int main(int argc, char* argv[])
{
#if defined(__SWITCH__)
    romfsInit();
#endif
    StartLog();
    std::string path = GameDataPath(argc, argv);
    // before ChDir moves us into the game folder
    std::string planPath, outPath = ".";
    if (const char* plan = Arg(argc, argv, "--plan"))
    {
        planPath = std::filesystem::absolute(plan).string();
        // a test run on the Mac stays in the background: no Dock icon, doesn't take the focus
        SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "1");
    }
    if (const char* out = Arg(argc, argv, "--out"))
    {
        outPath = std::filesystem::absolute(out).string();
    }

    // --notice album|nodata shows a screen without its condition, to check how it looks
    const char* preview = Arg(argc, argv, "--notice");
    if (preview)
    {
        ShowNotice(strcmp(preview, "album") == 0 ? kAlbumNotice : MissingDataNotice(path));
        return 0;
    }

    if (IsAlbumMode() && !ShowNotice(kAlbumNotice))
    {
        return 0;
    }
    if (!IsGameDirectory(path))
    {
        SDL_Log("No game data in %s", path.c_str());
        ShowNotice(MissingDataNotice(path));
        return 1;
    }

#if !defined(__SWITCH__)
    path = std::filesystem::absolute(path).string();
#endif
    Sexy::SetResourceFolder(path);
#if defined(__SWITCH__)
    Sexy::SetAppDataFolder(path);
#endif
    Sexy::ChDir(path);
    ReadTestFlags(argc, argv);
    // a plan.txt in the game folder runs as a test plan (for the emulator, where there are no arguments);
    // screenshots go to plan-out/
    if (planPath.empty() && std::filesystem::exists("plan.txt"))
    {
        planPath = std::filesystem::absolute("plan.txt").string();
        outPath = std::filesystem::absolute("plan-out").string();
    }

    // in a block, so the app (whose destructor saves the profile) is gone before the Switch may close us
    {
        SDL_Log("boot: main");
        Sexy::WinFishApp app;
        Sexy::gPadTargetHook = FindPadTarget;
        app.Init();
        SDL_Log("boot: app ready");
        // before Start() launches the loading thread, which builds the wide tank art
        Sexy::SetWideTank(app.mWidescreenAware && app.mWideTank);
        if (!planPath.empty())
        {
            const char* timeout = Arg(argc, argv, "--timeout");
            LoadTestPlan(&app, planPath.c_str(), outPath.c_str(), timeout ? atoi(timeout) : 60);
        }
        app.Start();
        app.Shutdown();
    }
#if defined(__SWITCH__)
    appletUnlockExit();
#endif
    return 0;
}
