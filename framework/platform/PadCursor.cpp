#include "PadCursor.h"
#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "widget/WidgetManager.h"
#include "widget/ButtonWidget.h"
#include "widget/Checkbox.h"
#include "widget/Dialog.h"
#include "misc/KeyCodes.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

using namespace Sexy;

PadInput Sexy::gScriptedPad;
int (*Sexy::gPadTargetHook)(SexyAppBase*, int, int, bool, int&, int&, bool&) = nullptr;

static float gX = 320, gY = 240;
static bool gShown = false;
static PadInput gLast;
static double gFullTime = 0;
static Uint64 gLastTicks = 0;
// buttons still held when touch or a mouse took over count only once released and pressed again
static bool gBlockClick = false, gBlockRightClick = false;
static Uint64 gLastClickTicks = 0;
// B pressed while a dialog was open went to Dialog::Cancel, so its release isn't a right click either
static bool gBCancelled = false;
// quick aim: pressing the same button again soon, without moving, goes to the next target
static int gAimKind = -1, gAimIndex = 0, gAimX = 0, gAimY = 0;
static double gAimAge = 0;	// seconds since the last jump (in logic ticks for test plans, like dt)
static double gAimHeld[2];	// how long X/Y has been held since its last action
static int gLastClickX = -100, gLastClickY = -100;

// Design units per second for a stick at radius r: deadzone 0.12, full at 0.95, quadratic in between.
static float StickSpeed(float r, float theTop)
{
	const float deadzone = 0.12f, full = 0.95f, slowest = 20;
	if (r <= deadzone)
	{
		return 0;
	}
	float m = std::min(1.0f, (r - deadzone) / (full - deadzone));
	return slowest + (theTop - slowest) * m * m;
}

// What can be clicked: the whole canvas on the wide tank, the 640 design space elsewhere.
static Rect Clickable(SexyAppBase* theApp)
{
	bool full = theApp->mGLInterface && theApp->mGLInterface->mFullCanvas;
	return full ? Rect(theApp->mScreenBounds.mX, 0, theApp->mScreenBounds.mWidth, theApp->mHeight)
		: Rect(0, 0, theApp->mWidth, theApp->mHeight);
}

// Buttons and checkboxes that a click at their middle would actually reach (not covered, not under a
// modal dialog), with their middles in widget coordinates.
static void CollectButtons(WidgetManager* theManager, WidgetContainer* theParent, std::vector<Point>& theOut)
{
	for (Widget* w : theParent->mWidgets)
	{
		if (!w->mVisible || w->mDisabled)
		{
			continue;
		}
		if (dynamic_cast<ButtonWidget*>(w) || dynamic_cast<Checkbox*>(w))
		{
			Point p = w->GetAbsPos();
			Point middle(p.mX + w->mWidth / 2, p.mY + w->mHeight / 2);
			if (theManager->GetWidgetAt(middle.mX, middle.mY, nullptr, nullptr) == w)
			{
				theOut.push_back(middle);
			}
		}
		CollectButtons(theManager, w, theOut);
	}
}

// D-pad: the nearest button roughly in that direction (within about 60 degrees), favouring straight ahead.
static bool FindButton(WidgetManager* theManager, int theX, int theY, int theDirX, int theDirY, Point& theFound)
{
	std::vector<Point> buttons;
	CollectButtons(theManager, theManager, buttons);
	float best = 1e9f;
	for (const Point& b : buttons)
	{
		float vx = (float)(b.mX - theX), vy = (float)(b.mY - theY);
		float along = vx * theDirX + vy * theDirY;
		float across = std::abs(vx * theDirY - vy * theDirX);
		if (along <= 4 || across > along * 1.7f)
		{
			continue;
		}
		float score = along + 2 * across;
		if (score < best)
		{
			best = score;
			theFound = b;
		}
	}
	return best < 1e9f;
}

bool Sexy::ClampPointer(SexyAppBase* theApp, int& theX, int& theY)
{
	Rect r = Clickable(theApp);
	bool inside = r.Contains(theX, theY);
	theX = std::clamp(theX, r.mX, r.mX + r.mWidth - 1);
	theY = std::clamp(theY, r.mY, r.mY + r.mHeight - 1);
	return inside;
}

void Sexy::UpdatePadCursor(SexyAppBase* theApp, const PadInput& theInput)
{
	PadInput in = theInput;
	gBlockClick = in.mClick && (gBlockClick || in.mPointerElsewhere);
	gBlockRightClick = in.mRightClick && (gBlockRightClick || in.mPointerElsewhere);
	in.mClick &= !gBlockClick;
	in.mRightClick &= !gBlockRightClick;

	Uint64 now = SDL_GetPerformanceCounter();
	double dt = gLastTicks ? (double)(now - gLastTicks) / SDL_GetPerformanceFrequency() : 0;
	gLastTicks = now;
	// test plans count in logic ticks, so a run moves the cursor the same way every time
	static int sLastUpdate = -1;
	if (theApp->mScriptedInput)
	{
		dt = sLastUpdate < 0 ? 0 : (theApp->mUpdateCount - sLastUpdate) * theApp->mFrameTime / 1000.0;
	}
	sLastUpdate = theApp->mUpdateCount;
	dt = std::min(dt, 0.05);

	if (in.mPointerElsewhere)
	{
		in.mLeftX = in.mLeftY = in.mRightX = in.mRightY = 0;
		in.mMouseX = in.mMouseY = 0;
	}
	float r = std::hypot(in.mLeftX, in.mLeftY);
	float m = std::min(1.0f, std::max(0.0f, (r - 0.12f) / (0.95f - 0.12f)));
	gFullTime = m > 0.9f ? gFullTime + dt : 0;
	// held at full tilt, the top speed ramps up for long trips across the wide tank
	float ramp = std::clamp((float)(gFullTime - 0.25) / 0.3f, 0.0f, 1.0f);
	float dx = 0, dy = 0;
	if (float v = StickSpeed(r, 1100 + 500 * ramp))
	{
		dx += in.mLeftX / r * v;
		dy -= in.mLeftY / r * v;
	}
	float rr = std::hypot(in.mRightX, in.mRightY);
	if (float v = StickSpeed(rr, 1100) * 0.25f)
	{
		dx += in.mRightX / rr * v;
		dy -= in.mRightY / rr * v;
	}
	if (in.mPrecision)
	{
		dx *= 0.35f;
		dy *= 0.35f;
	}
	// slower over things that can be clicked (the game shows its hand cursor), easier to stop on a coin
	else if (gShown && theApp->mCursorNum == CURSOR_HAND)
	{
		dx *= 0.5f;
		dy *= 0.5f;
	}

	WidgetManager* wm = theApp->mWidgetManager;
	int oldX = (int)gX, oldY = (int)gY;
	gX += dx * (float)dt + in.mMouseX;
	gY += dy * (float)dt + in.mMouseY;
	bool mouseMoved = in.mMouseX != 0 || in.mMouseY != 0;
	// every frame: the clickable area changes with the screen and the Widescreen option
	Rect area = Clickable(theApp);
	gX = std::clamp(gX, (float)area.mX, (float)(area.mX + area.mWidth - 1));
	gY = std::clamp(gY, (float)area.mY, (float)(area.mY + area.mHeight - 1));
	int x = (int)gX, y = (int)gY;

	// D-pad: arrow keys (lists, the Konami code), and the cursor jumps to the next button that way
	struct { bool PadInput::*mHeld; KeyCode mKey; int mDirX, mDirY; } dpad[] = {
		{&PadInput::mUp, KEYCODE_UP, 0, -1}, {&PadInput::mDown, KEYCODE_DOWN, 0, 1},
		{&PadInput::mLeft, KEYCODE_LEFT, -1, 0}, {&PadInput::mRight, KEYCODE_RIGHT, 1, 0},
	};
	bool jumped = false;
	for (auto& d : dpad)
	{
		if (in.*d.mHeld == gLast.*d.mHeld)
		{
			continue;
		}
		if (!(in.*d.mHeld))
		{
			wm->KeyUp(d.mKey);
			continue;
		}
		wm->KeyDown(d.mKey);
		Point target;
		if (!in.mPointerElsewhere && FindButton(wm, x, y, d.mDirX, d.mDirY, target))
		{
			gX = (float)target.mX;
			gY = (float)target.mY;
			x = target.mX;
			y = target.mY;
			jumped = true;
		}
	}

	// quick aim: X to the hungriest fish, Y to money (QOL.md). A press jumps there and clicks (ZL held: only
	// jumps); held, it does so again every 0.3 s
	gAimAge += dt;
	bool aimClick = false;
	for (int kind = 0; kind < 2; kind++)
	{
		bool held = kind == 0 ? in.mJumpFish : in.mJumpMoney;
		bool was = kind == 0 ? gLast.mJumpFish : gLast.mJumpMoney;
		bool fresh = held && !was;
		bool repeat = false;
		if (held && was)
		{
			gAimHeld[kind] += dt;
			if (gAimHeld[kind] >= 0.3)
			{
				gAimHeld[kind] = 0;
				repeat = true;
			}
		}
		else
		{
			gAimHeld[kind] = 0;
		}
		if (!(fresh || repeat) || in.mPointerElsewhere || !theApp->mQuickAim || !gPadTargetHook)
		{
			continue;
		}
		// A held is already clicking; with A just let go or B down, the down would go to that widget instead
		bool click = !in.mPrecision && !in.mClick && !gLast.mClick && !in.mRightClick;
		bool again = kind == gAimKind && gAimAge < 1.5 && x == gAimX && y == gAimY;
		// clicked money is gone, so a clicking press starts at the top again; a fed fish stays hungry until
		// it eats, so the next press goes on to the next one
		int index = (again && !(click && kind == 1)) ? gAimIndex + 1 : 0;
		int tx, ty;
		bool clickable = false;
		int found = gPadTargetHook(theApp, kind, index, repeat, tx, ty, clickable);
		if (found > 0)
		{
			gAimKind = kind;
			gAimIndex = index;
			gAimAge = 0;
			gX = (float)tx;
			gY = (float)ty;
			x = gAimX = tx;
			y = gAimY = ty;
			jumped = true;
			aimClick |= click && clickable;
		}
		else if (found == 0 && fresh && theApp->mRumble)
		{
			// in the tank, but nothing to aim at
			RumblePad(0.15f, 0.0f, 40);
		}
	}

	bool anyButton = in.mClick || in.mRightClick || in.mMenu || in.mPause || in.mUp || in.mDown || in.mLeft || in.mRight ||
		in.mJumpFish || in.mJumpMoney;
	bool changed = anyButton || gLast.mClick || gLast.mRightClick || gLast.mMenu || gLast.mPause ||
		gLast.mUp || gLast.mDown || gLast.mLeft || gLast.mRight || gLast.mJumpFish || gLast.mJumpMoney;
	if (!changed && dx == 0 && dy == 0 && !mouseMoved && !jumped)
	{
		if (gShown && (x != oldX || y != oldY))
		{
			wm->MouseMove(x, y);
			theApp->mCustomCursorDirty = true;
		}
		gLast = in;
		return;
	}

	theApp->mLastUserInputTick = theApp->mLastTimerTime;
	// only pointing shows the cursor, + and - don't (and nothing does while a finger points)
	bool pointing = dx != 0 || dy != 0 || mouseMoved || jumped || in.mClick || in.mRightClick || gLast.mClick || gLast.mRightClick;
	if (pointing && (!gShown || x != oldX || y != oldY))
	{
		gShown = true;
		wm->MouseMove(x, y);
		theApp->mCustomCursorDirty = true;
	}

	if (aimClick)
	{
		wm->MouseDown(x, y, 1);
		wm->MouseUp(x, y, 1);
	}

	if (in.mClick != gLast.mClick)
	{
		if (in.mClick)
		{
			// what was aimed at may be gone now (a collected coin), so the next X/Y starts from the top
			gAimKind = -1;
			// a second press soon after, in the same spot, is a double click (as SDL reports on the desktop)
			bool twice = now - gLastClickTicks < SDL_GetPerformanceFrequency() * 4 / 10 &&
				std::abs(x - gLastClickX) <= 6 && std::abs(y - gLastClickY) <= 6;
			wm->MouseDown(x, y, twice ? 2 : 1);
			gLastClickTicks = twice ? 0 : now;
			gLastClickX = x;
			gLastClickY = y;
		}
		else
		{
			wm->MouseUp(x, y, 1);
		}
	}
	// B and + close the topmost dialog (Dialog::Cancel), whatever has the focus inside it
	auto cancelDialog = [&]()
	{
		return !theApp->mDialogList.empty() && theApp->mDialogList.back()->Cancel();
	};
	if (in.mRightClick != gLast.mRightClick)
	{
		if (in.mRightClick)
		{
			gBCancelled = cancelDialog();
			if (!gBCancelled)
			{
				wm->MouseDown(x, y, -1);
			}
		}
		else if (!gBCancelled)
		{
			wm->MouseUp(x, y, -1);
		}
	}
	if (in.mMenu != gLast.mMenu)
	{
		if (!in.mMenu)
		{
			wm->KeyUp(KEYCODE_ESCAPE);
		}
		else if (!cancelDialog())
		{
			wm->KeyDown(KEYCODE_ESCAPE);
		}
	}
	if (in.mPause != gLast.mPause)
	{
		if (in.mPause)
		{
			wm->KeyDown(KEYCODE_SPACE);
			wm->KeyChar(' ');
		}
		else
		{
			wm->KeyUp(KEYCODE_SPACE);
		}
	}
	gLast = in;
}

void Sexy::HidePadCursor(SexyAppBase* theApp, int theX, int theY)
{
	// let go of what the pad holds, so the other pointer starts clean
	WidgetManager* wm = theApp->mWidgetManager;
	if (gLast.mClick)
	{
		wm->MouseUp((int)gX, (int)gY, 1);
		gLast.mClick = false;
		gBlockClick = true;
	}
	if (gLast.mRightClick && !gBCancelled)
	{
		wm->MouseUp((int)gX, (int)gY, -1);
		gLast.mRightClick = false;
		gBlockRightClick = true;
	}
	gX = (float)theX;
	gY = (float)theY;
	if (gShown)
	{
		gShown = false;
		theApp->mCustomCursorDirty = true;
	}
}

bool Sexy::GetPadCursor(int& theX, int& theY)
{
	theX = (int)gX;
	theY = (int)gY;
	return gShown;
}
