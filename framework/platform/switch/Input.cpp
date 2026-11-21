/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 * 
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#include <switch.h>
#include <algorithm>
#include <vector>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"
#include "platform/PadCursor.h"

using namespace Sexy;

// switchfish: the pad drives the cursor in platform/PadCursor.cpp, touch points directly
// (docs/CONTROLS.md). libnx owns the pad; SDL's game controller layer isn't started on the Switch.
static PadState gPad;
static bool gTouching = false;
static u32 gTouchFinger = 0;
static int gTouchX = 0, gTouchY = 0, gTouchButton = 1;
// fingers that started in the letterbox or alongside the tracked one, ignored until they lift
static std::vector<u32> gIgnoredFingers;

void SexyAppBase::InitInput()
{
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	// handheld plus the first controller
	padInitializeDefault(&gPad);
	hidInitializeTouchScreen();
	hidInitializeMouse();

	if (!mMouseIn)
		mMouseIn = true;
}

bool SexyAppBase::StartTextInput(std::string& theInput, int theMaxChars)
{
	// the keyboard blocks, a test plan would wait forever
	if (mScriptedInput)
	{
		return false;
	}

	char buf[512];

	SwkbdConfig kbd;
	swkbdCreate(&kbd, 0);
	swkbdConfigMakePresetDefault(&kbd);
	swkbdConfigSetType(&kbd, SwkbdType_Normal);
	swkbdConfigSetInitialText(&kbd, theInput.c_str());
	if (theMaxChars > 0)
	{
		swkbdConfigSetStringLenMax(&kbd, (u32)std::min(theMaxChars, (int)sizeof(buf) - 1));
	}

	swkbdConfigSetGuideText(&kbd, "Enter text...");
	swkbdConfigSetOkButtonText(&kbd, "OK");

	Result rc = swkbdShow(&kbd, buf, sizeof(buf));
	swkbdClose(&kbd);

	if (R_SUCCEEDED(rc))
	{
		theInput = buf;
		return true;
	}

	return false;
}

void SexyAppBase::StopTextInput()
{
	
}

static PadInput ReadPad()
{
	PadInput in;
	u64 held = padGetButtons(&gPad);
	HidAnalogStickState left = padGetStickPos(&gPad, 0);
	HidAnalogStickState right = padGetStickPos(&gPad, 1);
	in.mLeftX = left.x / 32767.0f;
	in.mLeftY = left.y / 32767.0f;
	in.mRightX = right.x / 32767.0f;
	in.mRightY = right.y / 32767.0f;
	in.mClick = held & (HidNpadButton_A | HidNpadButton_ZR);
	in.mRightClick = held & HidNpadButton_B;
	in.mPrecision = held & HidNpadButton_ZL;
	in.mMenu = held & HidNpadButton_Plus;
	in.mPause = held & HidNpadButton_Minus;
	in.mUp = held & HidNpadButton_Up;
	in.mDown = held & HidNpadButton_Down;
	in.mLeft = held & HidNpadButton_Left;
	in.mRight = held & HidNpadButton_Right;
	return in;
}

// A USB mouse (through the dock) moves the drawn cursor. HID samples faster than we draw, so every sample
// since the last frame counts (after insaniquarium_nx's nx_pointer).
static void ReadMouse(PadInput& theInput)
{
	static u64 sSeen = 0;
	HidMouseState states[16];
	int count = (int)hidGetMouseStates(states, 16);
	if (count <= 0)
	{
		return;
	}
	s32 dx = 0, dy = 0;
	u32 buttons = 0;
	u64 newest = sSeen, buttonsSample = 0;
	for (int i = 0; i < count; i++)
	{
		const HidMouseState& m = states[i];
		if (m.sampling_number > buttonsSample)
		{
			buttonsSample = m.sampling_number;
			buttons = m.buttons;
		}
		if (m.sampling_number <= sSeen)
		{
			continue;
		}
		dx += m.delta_x;
		dy += m.delta_y;
		newest = std::max(newest, m.sampling_number);
	}
	// the first time, only catch up with what is buffered
	bool first = sSeen == 0;
	sSeen = newest;
	if (first)
	{
		return;
	}
	// the screen is 1280 wide for 640 design units, so a mouse count is half a unit
	theInput.mMouseX = dx * 0.5f;
	theInput.mMouseY = dy * 0.5f;
	theInput.mClick |= (buttons & HidMouseButton_Left) != 0;
	theInput.mRightClick |= (buttons & HidMouseButton_Right) != 0;
}

// One finger points (the first one down); holding ZL makes it a right click.
static void ReadTouch(SexyAppBase* theApp, bool theRightClick)
{
	HidTouchScreenState state = {0};
	hidGetTouchScreenStates(&state, 1);
	auto present = [&](u32 theFinger)
	{
		for (s32 i = 0; i < state.count; i++)
		{
			if (state.touches[i].finger_id == theFinger)
			{
				return true;
			}
		}
		return false;
	};
	gIgnoredFingers.erase(std::remove_if(gIgnoredFingers.begin(), gIgnoredFingers.end(),
		[&](u32 theFinger) { return !present(theFinger); }), gIgnoredFingers.end());
	auto ignored = [](u32 theFinger)
	{
		return std::find(gIgnoredFingers.begin(), gIgnoredFingers.end(), theFinger) != gIgnoredFingers.end();
	};

	const HidTouchState* touch = nullptr;
	for (s32 i = 0; i < state.count; i++)
	{
		u32 finger = state.touches[i].finger_id;
		if (gTouching ? finger == gTouchFinger : !ignored(finger))
		{
			touch = &state.touches[i];
			break;
		}
	}

	WidgetManager* wm = theApp->mWidgetManager;
	if (touch)
	{
		// the panel is 1280x720, the same as the screen
		int x = (int)touch->x, y = (int)touch->y;
		wm->RemapMouse(x, y);
		bool inside = ClampPointer(theApp, x, y);
		if (!gTouching)
		{
			// a press in the letterbox does nothing, not even once dragged into the picture, and neither
			// do the fingers down with it
			if (!inside)
			{
				for (s32 i = 0; i < state.count; i++)
				{
					gIgnoredFingers.push_back(state.touches[i].finger_id);
				}
				return;
			}
			gTouching = true;
			gTouchFinger = touch->finger_id;
			gTouchButton = theRightClick ? -1 : 1;
			HidePadCursor(theApp, x, y);
			wm->MouseMove(x, y);
			wm->MouseDown(x, y, gTouchButton);
		}
		else if (x != gTouchX || y != gTouchY)
		{
			wm->MouseMove(x, y);
		}
		gTouchX = x;
		gTouchY = y;
		theApp->mLastUserInputTick = theApp->mLastTimerTime;
	}
	else if (gTouching)
	{
		gTouching = false;
		// other fingers still down don't take over
		for (s32 i = 0; i < state.count; i++)
		{
			gIgnoredFingers.push_back(state.touches[i].finger_id);
		}
		wm->MouseUp(gTouchX, gTouchY, gTouchButton);
		// nothing stays lit once the finger is gone
		Finger* f = wm->GetFinger(0);
		if (f && f->mOverWidget)
		{
			Widget* over = f->mOverWidget;
			f->mOverWidget = nullptr;
			wm->MouseLeave(over);
		}
		HidePadCursor(theApp, gTouchX, gTouchY);
	}
}

bool SexyAppBase::ProcessDeferredMessages(bool singleMessage)
{
	if (!appletMainLoop())
	{
		mShutdown = true;
		return false;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		HandleEvent(&event);
	}

	padUpdate(&gPad);

	if (mScriptedInput)
	{
		UpdatePadCursor(this, gScriptedPad);
		return false;
	}

	// Home, sleep and the like: pause and mute as alt-tab does on the desktop
	bool focused = appletGetFocusState() == AppletFocusState_InFocus;
	if (focused != mActive)
	{
		mActive = focused;
		RehupFocus();
		if (focused)
		{
			mWidgetManager->MarkAllDirty();
		}
	}

	// out of focus, the held buttons come back up
	PadInput in = focused ? ReadPad() : PadInput();
	if (focused)
	{
		ReadMouse(in);
	}
	// touch first, so a finger landing this frame already owns the pointer
	if (focused)
	{
		ReadTouch(this, in.mPrecision);
	}
	in.mPointerElsewhere = gTouching;
	UpdatePadCursor(this, in);

	return false;
}
