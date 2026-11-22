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
#include <mutex>
#include <exception>

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

// The log: live in the Mac's terminal when started with ./sf.sh send, in log.txt only when the game folder
// has a debug.txt (the emulator runs), and otherwise only the last lines in memory, written to crash.txt
// if the game crashes. Writing every line to the SD card cost time.
static bool gNxlink = false;
static FILE* gLogFile = nullptr;

static const int kRecentLines = 256, kLineLength = 160;
static char gRecent[kRecentLines][kLineLength];
static int gRecentNext = 0;
static std::mutex gRecentLock;

static void WriteRecent(FILE* f)
{
    for (int i = 0; i < kRecentLines; i++)
    {
        const char* line = gRecent[(gRecentNext + i) % kRecentLines];
        if (line[0])
        {
            fprintf(f, "%s\n", line);
        }
    }
}

// libnx calls this on a crash in any thread (with __nx_exception_stack); returning lets the system handle it
extern "C" {
alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
extern char __start__;

void __libnx_exception_handler(ThreadExceptionDump* ctx)
{
    // no locks here: the crashed thread may hold one
    FILE* f = fopen("sdmc:/switch/switchfish/crash.txt", "w");
    if (!f)
    {
        return;
    }
    u64 base = (u64)&__start__;
    fprintf(f, "switchfish crashed (error 0x%x)\n", ctx->error_desc);
    fprintf(f, "pc 0x%lx lr 0x%lx (in the ELF: pc 0x%lx lr 0x%lx)\n", ctx->pc.x, ctx->lr.x, ctx->pc.x - base,
        ctx->lr.x - base);
    fprintf(f, "sp 0x%lx far 0x%lx esr 0x%x\n\nlast log lines:\n", ctx->sp.x, ctx->far.x, ctx->esr);
    WriteRecent(f);
    fclose(f);
}
}

static void Terminated()
{
    if (FILE* f = fopen("sdmc:/switch/switchfish/crash.txt", "w"))
    {
        fprintf(f, "switchfish stopped on an unhandled C++ error\n\nlast log lines:\n");
        WriteRecent(f);
        fclose(f);
    }
    abort();
}

// The homebrew loader loads the next program (hbmenu, Sphaira) into the memory this one used. A block still
// lent to a service or the GPU makes that fail (hbloader stops with 2001-0106), so list what isn't plain
// memory any more before going back.
extern "C" char* fake_heap_start;
extern "C" char* fake_heap_end;

static void LogLeftoverMemory()
{
    u64 addr = (u64)fake_heap_start, end = (u64)fake_heap_end;
    int shown = 0;
    while (addr < end && shown < 40)
    {
        MemoryInfo info = {};
        u32 pageInfo = 0;
        if (R_FAILED(svcQueryMemory(&info, &pageInfo, addr)) || info.size == 0)
        {
            break;
        }
        if (info.attr != 0 || info.type != MemType_Heap)
        {
            SDL_Log("exit: memory 0x%lx +0x%lx type 0x%x attr 0x%x perm 0x%x", info.addr, info.size, info.type,
                info.attr, info.perm);
            shown++;
        }
        addr = info.addr + info.size;
    }
    SDL_Log("exit: heap 0x%lx..0x%lx checked, %d blocks not plain", (u64)fake_heap_start, end, shown);
}

static void StartLog()
{
    if (__nxlink_host.s_addr != 0 && R_SUCCEEDED(socketInitializeDefault()))
    {
        gNxlink = nxlinkStdio() >= 0;
    }
    if (FILE* flag = fopen("sdmc:/switch/switchfish/debug.txt", "rb"))
    {
        fclose(flag);
        gLogFile = fopen("sdmc:/switch/switchfish/log.txt", "wb");
    }
    std::set_terminate(Terminated);
    SDL_LogSetOutputFunction([](void*, int, SDL_LogPriority, const char* message)
    {
        if (!message)
        {
            return;
        }
        // seconds since start, to see where the time goes
        static const u64 start = armGetSystemTick();
        double t = armTicksToNs(armGetSystemTick() - start) / 1e9;
        std::lock_guard<std::mutex> lock(gRecentLock);
        snprintf(gRecent[gRecentNext], kLineLength, "%7.3f %s", t, message);
        gRecentNext = (gRecentNext + 1) % kRecentLines;
        if (gLogFile)
        {
            fprintf(gLogFile, "%7.3f %s\n", t, message);
            fflush(gLogFile);
        }
        if (gNxlink)
        {
            printf("%7.3f %s\n", t, message);
            fflush(stdout);
        }
    }, nullptr);
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
    // the game appends names straight to this ("userdata/users.dat"), so it needs the slash
    Sexy::SetAppDataFolder(path + "/");
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
        SDL_Log("exit: main loop ended");
        app.Shutdown();
        SDL_Log("exit: saved");
    }
#if defined(__SWITCH__)
    LogLeftoverMemory();
    romfsExit();
    SDL_Log("exit: unlocking");
    appletUnlockExit();
    if (gNxlink)
    {
        socketExit();
    }
#endif
    return 0;
}
