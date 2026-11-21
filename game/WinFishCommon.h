#ifndef __WINFISHAPPCOMMON_H__
#define __WINFISHAPPCOMMON_H__

#include "graphics/Font.h"
#include "widget/Dialog.h"
#include "widget/EditWidget.h"
#include "widget/Checkbox.h"
#include "widget/HyperlinkWidget.h"
#include "graphics/MemoryImage.h"
#include "FishSongMgr.h"
#include "GameObject.h"

namespace Sexy
{
	class Graphics;
	// switchfish: the tank's edges in design coordinates. 0..640 as in the original unless the wide tank is
	// on, then -107..747 (the whole 854 wide canvas). Every bound in the sim is written so it gives the
	// original number at 0..640.
	extern int			gTankLeft;
	extern int			gTankRight;
	inline int			TankLeft() { return gTankLeft; }
	inline int			TankRight() { return gTankRight; }
	inline int			TankWidth() { return gTankRight - gTankLeft; }
	void				SetWideTank(bool wide);
	void				FixLoadedXBounds(int& theMin, int& theMax);
	// switchfish: g's translation in widget coordinates. In widescreen the widget manager also translates by
	// the canvas offset, so code that read mTransX as a widget's position goes through these (same in 4:3).
	float				WidgetTransX(Graphics* g);
	float				WidgetTransY(Graphics* g);

	// switchfish: test flags (--quickplay, --level T-L). N finishes the current level, and a new
	// adventure board starts at gStartTank-gStartLevel once.
	extern bool			gQuickPlay;
	extern int			gStartTank;
	extern int			gStartLevel;

	DialogButton*		MakeDialogButton(int theId, ButtonListener* theListener, SexyString theText, Font* theFont);
	DialogButton*		MakeDialogButton2(int theId, ButtonListener* theListener, SexyString theText, Image* theComponentImage);
	void				DefaultDialogSettings(Dialog* theDialog);
	void				DefaultDialogButtonSettings(DialogButton* theDialogButton);
	EditWidget*			MakeEditWidget(int theId, EditListener* theEditListener);
	Checkbox*			MakeCheckbox(int theId, CheckboxListener* theCheckboxListener, bool theChecked);
	HyperlinkWidget*	MakeHyperlinkWidget(int theId, ButtonListener* theButtonListener, SexyString theLabel);

	void				DrawStringWithOutline(Graphics* g, SexyString theLine, int theX, int theY, Font* theFont, uint32_t theOutlineColor);
	void				DrawWaves(Graphics* g, int theY, int theUpdateCnt);
	void				DrawHorzCrease(Graphics* g, Image* theImage,int theX, int theY, int theWidth);
	void				DrawVertCrease(Graphics* g, Image* theImage,int theX, int theY, int theHeight);
	void				DrawCheckboxString(Graphics* g, const SexyString& theLine, Checkbox* theCB, const char* theExtensionString);

	int					InterpolateInt(int startValue, int endValue, int currentProgress, int totalDuration, bool swap);

	void				SetVaryingBlueColorGraphics(Graphics* g, int theUpdateCnt, int theY);

	SexyString			GetCyraxEndGameString(int theUnk01);
	SexyString			GetPlayTimeString(int theTimeInSec);
	SexyString			GetMentalStateString(GameObject* theObject);

	void				KnuthShuffleFishSongs(FishSongData** first, FishSongData** last);
	FishSongData*		GetSongData(int theSpecId);

	Time    			GetTodayStartSeconds();

	void				ButtonHoleHelper(MemoryImage* theImage, MemoryImage* theHoleImage, int theX, int theY);
	SexyString			GetPetName(int thePetId);

	void				NormalizeVector3(float* theVector);

	int					GetLevelIndex(int theTank, int theLevel);
}

#endif
