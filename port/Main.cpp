#include "WinFishApp.h"
#include "WinFishCommon.h"
#include "Notice.h"
#include "TestPlan.h"
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

static const Notice kAlbumNotice = {
    "Started from the album",
    "Homebrew started from the album gets little memory and no keyboard for player names.\n\n"
    "For the full game, hold R while starting any game, then pick switchfish in the Homebrew Menu.",
    true};

#if defined(__SWITCH__)
#include <switch.h>

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

static void StartLog()
{
    static FILE* logfile = fopen("sdmc:/switch/switchfish/log.txt", "wb");
    SDL_LogSetOutputFunction([](void* userdata, int, SDL_LogPriority, const char* message)
    {
        if (userdata && message)
        {
            fprintf((FILE*)userdata, "%s\n", message);
            fflush((FILE*)userdata);
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
    // keep SDL button positions, the framework maps them itself
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
#endif
    Sexy::ChDir(path);

    Sexy::WinFishApp app;
    app.Init();
    // before Start() launches the loading thread, which builds the wide tank art
    Sexy::SetWideTank(app.mWidescreenAware && app.mWideTank);
    if (!planPath.empty())
    {
        const char* timeout = Arg(argc, argv, "--timeout");
        LoadTestPlan(&app, planPath.c_str(), outPath.c_str(), timeout ? atoi(timeout) : 60);
    }
    app.Start();
    app.Shutdown();
    return 0;
}
