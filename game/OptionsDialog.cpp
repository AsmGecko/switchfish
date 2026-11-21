#include "widget/Slider.h"
#include "widget/WidgetManager.h"
#include "widget/DialogButton.h"

#include "CheatsDialog.h"
#include "OptionsDialog.h"
#include "HelpScreen.h"
#include "WinFishApp.h"
#include "WinFishCommon.h"
#include "Board.h"
#include "Res.h"

using namespace Sexy;

Sexy::OptionsDialog::OptionsDialog(WinFishApp* theApp, bool theFlag)
	: MoneyDialog(theApp, IMAGE_DIALOG, IMAGE_DIALOGBUTTON, DIALOG_OPTIONS, true, "Options",
		"", "OK", BUTTONS_FOOTER)
{
	mApp2 = theApp;
	SetColor(3, Color(0xff, 0xff, 100));
	mFlag = theFlag;
	mRegisterButton = MakeDialogButton(0, this, "Register", NULL);
	mHelpButton = MakeDialogButton(1, this, "Help", NULL);
	mControlsButton = MakeDialogButton(13, this, "Controls", NULL);
	SexyString aWebLinkStr = mApp->GetString("weblinktext", "");
	mWebLinkButton = MakeDialogButton(2, this, aWebLinkStr, NULL);

	mCheckUpdatesButton = MakeDialogButton(4, this, "Check Updates", NULL);
	mBackButton = MakeDialogButton(3, this, "Back to Main Menu", NULL);

	mMusicSlider = new Slider(IMAGE_SLIDERTRACK, IMAGE_SLIDERTHUMB, 5, this);
	double aMusicValue = mApp->GetMusicVolume();
	if (aMusicValue > 1.0) aMusicValue = 1.0; if (aMusicValue < 0) aMusicValue = 0;
	mMusicSlider->SetValue(aMusicValue);

	mSFXSlider = new Slider(IMAGE_SLIDERTRACK, IMAGE_SLIDERTHUMB, 6, this);
	double aSfxValue = mApp->GetSfxVolume();
	if (aSfxValue > 1.0) aSfxValue = 1.0; if (aSfxValue < 0) aSfxValue = 0;
	mSFXSlider->SetValue(aSfxValue);

	mCustomCursorsCB = MakeCheckbox(8, this, mApp->mCustomCursorsEnabled);
	mWidescreenCB = MakeCheckbox(11, this, mApp->mWidescreenAware);
	mQuickAimCB = MakeCheckbox(12, this, mApp->mQuickAim);
    mCheatsButton = MakeDialogButton(10, this, "Cheats", NULL);

	if (theFlag)
		mBackButton->SetVisible(false);
	if (mApp->mDontUpdate)
		mCheckUpdatesButton->SetVisible(false);
	if(mApp->mBuildUnlocked)
		mRegisterButton->SetVisible(false);
	if(mApp->mBoard == NULL)
		mCheatsButton->SetVisible(false);
	mWebLinkButton->SetVisible(mWebLinkButton->mLabel.size() != 0);
}

Sexy::OptionsDialog::~OptionsDialog()
{
	if (mRegisterButton)
		delete mRegisterButton;
	if (mCheckUpdatesButton)
		delete mCheckUpdatesButton;
	if (mWebLinkButton)
		delete mWebLinkButton;
	if (mMusicSlider)
		delete mMusicSlider;
	if (mSFXSlider)
		delete mSFXSlider;
	if (mCustomCursorsCB)
		delete mCustomCursorsCB;
	if (mWidescreenCB)
		delete mWidescreenCB;
	if (mQuickAimCB)
		delete mQuickAimCB;
	if (mHelpButton)
		delete mHelpButton;
	if (mControlsButton)
		delete mControlsButton;
	if (mBackButton)
		delete mBackButton;
    if (mCheatsButton)
        delete mCheatsButton;
}

void Sexy::OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterButton);
	theWidgetManager->AddWidget(mCheckUpdatesButton);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mControlsButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSFXSlider);
	theWidgetManager->AddWidget(mCustomCursorsCB);
	theWidgetManager->AddWidget(mWidescreenCB);
	theWidgetManager->AddWidget(mQuickAimCB);
	theWidgetManager->AddWidget(mWebLinkButton);
	theWidgetManager->AddWidget(mBackButton);
    theWidgetManager->AddWidget(mCheatsButton);
}

void Sexy::OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MoneyDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterButton);
	theWidgetManager->RemoveWidget(mCheckUpdatesButton);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mControlsButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSFXSlider);
	theWidgetManager->RemoveWidget(mCustomCursorsCB);
	theWidgetManager->RemoveWidget(mWidescreenCB);
	theWidgetManager->RemoveWidget(mQuickAimCB);
	theWidgetManager->RemoveWidget(mWebLinkButton);
	theWidgetManager->RemoveWidget(mBackButton);
    theWidgetManager->RemoveWidget(mCheatsButton);
}

void Sexy::OptionsDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);
	g->SetFont(FONT_JUNGLEFEVER10OUTLINE);
	g->SetColorizeImages(true);
	g->SetColor(mColors[4]);
	g->DrawString("Music", 48, 78);
	g->DrawString("Sound Fx", 48, 110);
	g->DrawString("Custom Cursors", mCustomCursorsCB->mX - mX + 43, mCustomCursorsCB->mY - mY + 24);
	g->DrawString("Widescreen", mWidescreenCB->mX - mX + 43, mWidescreenCB->mY - mY + 24);
	g->DrawString("Quick Aim (Y/X)", mQuickAimCB->mX - mX + 43, mQuickAimCB->mY - mY + 24);
	g->SetColorizeImages(false);
}

// AI Generated
void Sexy::OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	MoneyDialog::Resize(theX, theY, theWidth, theHeight);
	int aWidgetY = mYesButton->mY - 34;

	if (mBackButton->mVisible)
	{
		int aButtonWidth = mWidth - mContentInsets.mLeft - mContentInsets.mRight - mButtonSidePadding * 2;
		int aButtonX = mX + mContentInsets.mLeft;

		mBackButton->Resize(aButtonX, aWidgetY, aButtonWidth, 33);
		aWidgetY -= 34;
	}

	int aHalfWidth = (mWidth - mContentInsets.mLeft - mContentInsets.mRight - 6) / 2;
	if (mWebLinkButton->mVisible)
	{
		int aButtonX = mX + mContentInsets.mLeft;
		mWebLinkButton->Resize(aButtonX, aWidgetY, aHalfWidth, 33);
		aWidgetY -= 34;
	}

	std::vector<ButtonWidget*> aBottomButtons;
	if (mHelpButton->mVisible) aBottomButtons.push_back(mHelpButton);
	if (mControlsButton->mVisible) aBottomButtons.push_back(mControlsButton);
	if (mRegisterButton->mVisible) aBottomButtons.push_back(mRegisterButton);
	if (mCheckUpdatesButton->mVisible) aBottomButtons.push_back(mCheckUpdatesButton);
    if (mCheatsButton->mVisible) aBottomButtons.push_back(mCheatsButton);

	// switchfish: 1 only (was 1 or 3). Three buttons (Help, Controls, Cheats) share one row, the dialog has
	// no room for a second
	if (aBottomButtons.size() == 1)
	{
		int aButtonX = mX + mContentInsets.mLeft;
		int aButtonWidth = mWidth - mContentInsets.mLeft - mContentInsets.mRight;
		aBottomButtons[0]->Resize(aButtonX, aWidgetY, aButtonWidth, 33);
		aWidgetY -= 34;
	}
	if (aBottomButtons.size() > 1)
	{
		int aButtonX = mX + mContentInsets.mLeft;
		int aButtonWidth = (mWidth - mContentInsets.mLeft - mContentInsets.mRight - 3 * (aBottomButtons.size() - 1)) / aBottomButtons.size();

		for (size_t i = 0; i < aBottomButtons.size(); ++i)
		{
			aBottomButtons[i]->Resize(aButtonX, aWidgetY, aButtonWidth, 33);
			aButtonX += aButtonWidth + 3;
		}
	}

	int aSliderWidth = mWidth - 192;

	mMusicSlider->Resize(mX + 152, mY + 58, aSliderWidth, 33);
	mSFXSlider->Resize(mX + 152, mY + 90, aSliderWidth, 33);

	mCustomCursorsCB->Resize(mX + 161, mY + 128, 45, 46);
	// switchfish: Widescreen takes Fullscreen's place (the Mac window goes fullscreen itself, the Switch always is)
	mWidescreenCB->Resize(mX + 33, mY + 128, 45, 46);
	// switchfish: where Hardware Acceleration was (this port always draws with the GPU)
	mQuickAimCB->Resize(mX + 33, mY + 172, 45, 46);
}

int Sexy::OptionsDialog::GetPreferredHeight(int theWidth)
{
	int aPrefHght = 362;
	if (!mBackButton->mVisible)
		aPrefHght = 328;
	if (mRegisterButton->mVisible && mCheckUpdatesButton->mVisible)
		aPrefHght += 34;
	if (mWebLinkButton->mVisible)
		aPrefHght += 34;
	return aPrefHght;
}

void Sexy::OptionsDialog::WidgetClicked(int theId, bool theFlag)
{
	MoneyDialog::WidgetClicked(theId, theFlag);
}

void Sexy::OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	WidgetClicked(theId, checked);
}

void Sexy::OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == 5)
		mApp->SetMusicVolume(theVal);
	else if (theId == 6)
	{
		mApp->SetSfxVolume(theVal);
		if (!mSFXSlider->mDragging)
			mApp->PlaySample(SOUND_BUTTONCLICK);
	}
}

void Sexy::OptionsDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void Sexy::OptionsDialog::ButtonDepress(int theId)
{
	MoneyDialog::ButtonDepress(theId);
	switch (theId)
	{
	case 0:
		mApp->DoRegisterDialog();
		return;
	case 1:
		mApp->SwitchToHelpScreen(false);
		return;
	case 13:
		gHelpScreenTab = HELP_PAGE_CONTROLS;
		mApp->SwitchToHelpScreen(false);
		return;
	case 2:
		mApp->OpenURL(mApp->GetString("weblink", "http://www.popcap.com"));
		return;
	case 3:
		break;
	case 4:
		mApp->DoUpdateCheckDialog();
		return;
    case 10:
	{
		CheatsDialog *aDia = new CheatsDialog(mApp, 0);
		aDia->Resize(46, 26, 548, 348);
		mApp->AddDialog(DIALOG_CHEATS, aDia);
		return;
	}
	default:
		return;
	}

	if (mApp->mGameMode != GAMEMODE_VIRTUAL_TANK && mApp->mBoard &&
		mApp->mBoard->mShouldSave && mApp->mGameMode != GAMEMODE_SANDBOX)
	{
		mApp->DoLeaveGameDialog();
		return;
	}
	mApp->LeaveGameBoard();
}
