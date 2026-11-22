#ifndef __HIGHSCORESCREEN_H__
#define __HIGHSCORESCREEN_H__

#include "widget/Widget.h"
#include "WinFishCommon.h"
#include "widget/ButtonListener.h"

namespace Sexy
{
	class WinFishApp;
	class BubbleMgr;
	class DialogButton;

	extern int gHighScoreScreenPage;

	class HighScoreScreen : public Widget, public ButtonListener
	{
	public:
		FullCanvasHold			mCanvasHold;	// switchfish: holds the whole canvas while in the widget manager
		WinFishApp* mApp;
		BubbleMgr* mBubbleMgr;
		DialogButton* mMenuButton;
		DialogButton* mAdvButton;
		DialogButton* mTimeTrialButton;
		DialogButton* mChallengeButton;
		DialogButton* mPersonalButton;
		int mPage;

		enum HighScoreScreenPages
		{
			PAGE_ADV,
			PAGE_TIME,
			PAGE_CHAL,
			PAGE_PERSONAL
		};

	public:
		HighScoreScreen(WinFishApp* theApp);
		virtual ~HighScoreScreen();

		virtual void			AddedToManager(WidgetManager* theWidgetManager);
		virtual void			RemovedFromManager(WidgetManager* theWidgetManager);

		virtual void			Update();
		virtual void			Draw(Graphics* g);

		virtual void			DrawGameModePage(Graphics* g);
		virtual void			DrawPersonalRecordsPage(Graphics* g);

		virtual void			ButtonPress(int theId);
		virtual void			ButtonDepress(int theId);
	};
}

#endif