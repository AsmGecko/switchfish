#include "TestPlan.h"
#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/MemoryImage.h"
#include "imagelib/ImageLib.h"
#include "widget/WidgetManager.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <typeinfo>
#include "widget/Widget.h"
#include "platform/PadCursor.h"
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <unistd.h>

using namespace Sexy;

namespace {

struct Step
{
    int tick;
    std::string action;
    std::string arg;
    int x = 0, y = 0;
};

std::vector<Step> gSteps;
size_t gNext = 0;
std::string gOutDir;
Uint32 gDeadline = 0;
int gReleaseTick = -1, gReleaseX = 0, gReleaseY = 0, gReleaseButton = 1;
int gKeyReleaseTick = -1;
KeyCode gKeyHeld = KEYCODE_UNKNOWN;

KeyCode KeyFromName(const std::string& name)
{
    if (name == "escape")
    {
        return KEYCODE_ESCAPE;
    }
    if (name == "return")
    {
        return KEYCODE_RETURN;
    }
    if (name == "space")
    {
        return KEYCODE_SPACE;
    }
    return (KeyCode)toupper((unsigned char)name[0]);
}

void Run(SexyAppBase* app, const Step& s)
{
    WidgetManager* wm = app->mWidgetManager;
    SDL_Log("plan %d: %s %s", s.tick, s.action.c_str(), s.arg.c_str());
    if (s.action == "click" || s.action == "rclick")
    {
        int button = s.action == "click" ? 1 : -1;
        int wx = 0, wy = 0;
        Widget* target = wm->GetWidgetAt(s.x, s.y, &wx, &wy);
        SDL_Log("plan: click %d,%d hits %s at %d,%d %dx%d", s.x, s.y, target ? typeid(*target).name() : "nothing",
                target ? target->mX : 0, target ? target->mY : 0, target ? target->mWidth : 0, target ? target->mHeight : 0);
        wm->MouseMove(s.x, s.y);
        wm->MouseDown(s.x, s.y, button);
        gReleaseTick = s.tick + 2;
        gReleaseX = s.x;
        gReleaseY = s.y;
        gReleaseButton = button;
    }
    else if (s.action == "move")
    {
        wm->MouseMove(s.x, s.y);
    }
    else if (s.action == "pad")
    {
        std::istringstream in(s.arg);
        PadInput pad;
        in >> pad.mLeftX >> pad.mLeftY;
        std::string button;
        while (in >> button)
        {
            pad.mClick |= button == "a";
            pad.mRightClick |= button == "b";
            pad.mPrecision |= button == "zl";
            pad.mMenu |= button == "plus";
            pad.mPause |= button == "minus";
            pad.mUp |= button == "up";
            pad.mDown |= button == "down";
            pad.mLeft |= button == "left";
            pad.mRight |= button == "right";
        }
        gScriptedPad = pad;
        int x, y;
        bool shown = GetPadCursor(x, y);
        SDL_Log("plan: pad cursor at %d,%d%s", x, y, shown ? "" : " (hidden)");
    }
    else if (s.action == "key")
    {
        KeyCode key = KeyFromName(s.arg);
        wm->KeyDown(key);
        if (s.arg.size() == 1)
        {
            wm->KeyChar(s.arg[0]);
        }
        gKeyHeld = key;
        gKeyReleaseTick = s.tick + 2;
    }
    else if (s.action == "shot")
    {
        MemoryImage canvas(app);
        app->mGLInterface->ReadCanvas(&canvas);
        ImageLib::Image png;
        png.mWidth = canvas.mWidth;
        png.mHeight = canvas.mHeight;
        png.mBits = new uint32_t[canvas.mWidth * canvas.mHeight];
        memcpy(png.mBits, canvas.GetBits(), canvas.mWidth * canvas.mHeight * sizeof(uint32_t));
        std::string path = gOutDir + "/" + s.arg + ".png";
        if (!ImageLib::WritePNGImage(path, &png))
        {
            SDL_Log("plan: could not write %s", path.c_str());
        }
    }
    else if (s.action == "quit")
    {
        app->Shutdown();
    }
}

} // namespace

bool LoadTestPlan(SexyAppBase* app, const char* planPath, const char* outDir, int timeoutSeconds)
{
    gDeadline = SDL_GetTicks() + timeoutSeconds * 1000;
    std::ifstream file(planPath);
    if (!file)
    {
        SDL_Log("plan: can't read %s", planPath);
        return false;
    }
    gOutDir = outDir;
    std::error_code ec;
    std::filesystem::create_directories(gOutDir, ec);

    std::string line;
    int last = 0;
    while (std::getline(file, line))
    {
        std::istringstream in(line);
        std::string tickText;
        if (!(in >> tickText) || tickText[0] == '#')
        {
            continue;
        }
        Step s;
        s.tick = tickText[0] == '+' ? last + std::stoi(tickText.substr(1)) : std::stoi(tickText);
        in >> s.action;
        if (s.action == "click" || s.action == "rclick" || s.action == "move")
        {
            in >> s.x >> s.y;
        }
        else if (s.action == "pad")
        {
            std::getline(in, s.arg);
        }
        else
        {
            in >> s.arg;
        }
        last = s.tick;
        gSteps.push_back(s);
    }

    app->mScriptedInput = true;
    // test runs are silent
    app->Mute(false);
#if !defined(__SWITCH__)
    // no waiting for the display either: a sleeping screen never sends the vsync a swap waits for
    SDL_GL_SetSwapInterval(0);
#endif
    // and if the main loop hangs anyway, the run still ends a few seconds after its time is up
    std::thread([timeoutSeconds]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(timeoutSeconds + 5));
        SDL_Log("plan: main loop stuck past the time limit, exiting");
        _exit(3);
    }).detach();
    app->mTickHook = [app]()
    {
        int tick = app->mUpdateCount;
        // a test window may never get focus from the OS, and buttons only react with focus
        if (!app->mHasFocus)
        {
            app->ForceFocus();
        }
        if (SDL_GetTicks() > gDeadline)
        {
            SDL_Log("plan: time limit reached at tick %d, quitting", tick);
            app->Shutdown();
            return;
        }
        if (tick == gKeyReleaseTick)
        {
            app->mWidgetManager->KeyUp(gKeyHeld);
            gKeyReleaseTick = -1;
        }
        if (tick == gReleaseTick)
        {
            app->mWidgetManager->MouseUp(gReleaseX, gReleaseY, gReleaseButton);
            gReleaseTick = -1;
        }
        while (gNext < gSteps.size() && gSteps[gNext].tick <= tick)
        {
            Run(app, gSteps[gNext++]);
        }
    };
    return true;
}
