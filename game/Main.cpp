#include "WinFishApp.h"
#include <SDL2/SDL.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

static bool IsGameDirectory(const std::filesystem::path& p)
{
    return Sexy::FileExists((p / "properties" / "resources.xml").string());
}

#if defined(__SWITCH__)
#include <switch.h>

extern "C" {
    u32 __nx_stack_size = 4 * 1024 * 1024;
}

// Text console straight on the framebuffer, so it also works before SDL is up and in album mode.
static void ShowMissingData(const char* path)
{
    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\n\n  switchfish\n\n");
    printf("  Game files not found.\n\n");
    printf("  Copy the folders from your Insaniquarium Deluxe install\n");
    printf("  (properties, images, sounds, music, data, fishsongs) to\n\n");
    printf("    %s/\n\n\n", path);
    printf("  Press + to exit.\n");

    while (appletMainLoop())
    {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
        {
            break;
        }
        consoleUpdate(nullptr);
    }
    consoleExit(nullptr);
}

static bool FindGameData(int, char**)
{
    const char* path = "sdmc:/switch/switchfish";
    Sexy::SetResourceFolder(path);
    Sexy::SetAppDataFolder(path);
    Sexy::ChDir(path);

    static FILE* logfile = fopen("sdmc:/switch/switchfish/log.txt", "wb");
    SDL_LogSetOutputFunction([](void* userdata, int, SDL_LogPriority, const char* message)
    {
        if (userdata && message)
        {
            fprintf((FILE*)userdata, "%s\n", message);
            fflush((FILE*)userdata);
        }
    }, logfile);

    // keep SDL button positions, the framework maps them itself
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    if (!IsGameDirectory(path))
    {
        SDL_Log("No game data in %s, copy your own Insaniquarium Deluxe files there.", path);
        ShowMissingData(path);
        return false;
    }
    return true;
}

#else

// --data <dir>, then $SWITCHFISH_DATA, then ./gameData
static bool FindGameData(int argc, char** argv)
{
    std::string path;
    for (int i = 1; i + 1 < argc; i++)
    {
        if (strcmp(argv[i], "--data") == 0)
        {
            path = argv[i + 1];
        }
    }
    if (path.empty() && getenv("SWITCHFISH_DATA"))
    {
        path = getenv("SWITCHFISH_DATA");
    }
    if (path.empty())
    {
        path = "gameData";
    }

    if (!IsGameDirectory(path))
    {
        std::string msg = "Game files not found in " + path + ".\n\n"
            "Copy the folders from your Insaniquarium Deluxe install "
            "(properties, images, sounds, music, data, fishsongs) to gameData/, or start with --data <folder>.";
        SDL_Log("%s", msg.c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "switchfish", msg.c_str(), nullptr);
        return false;
    }
    path = std::filesystem::absolute(path).string();
    Sexy::SetResourceFolder(path);
    Sexy::ChDir(path);
    return true;
}

#endif

int main(int argc, char* argv[])
{
    if (!FindGameData(argc, argv))
    {
        return 1;
    }

    Sexy::WinFishApp app;
    app.Init();
    app.Start();
    app.Shutdown();
    return 0;
}
