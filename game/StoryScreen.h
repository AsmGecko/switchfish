#ifndef __STORYSCREEN_H__
#define __STORYSCREEN_H__

#include "widget/Widget.h"
#include "WinFishCommon.h"
#include "widget/ButtonListener.h"

namespace Sexy
{
	class WinFishApp;
	class BubbleMgr;
	class HyperlinkWidget;

	class StoryScreen : public Widget, public ButtonListener
	{
	public:
		FullCanvasHold			mCanvasHold;	// switchfish: holds the whole canvas while in the widget manager
		WinFishApp* mApp;
		HyperlinkWidget* mBackToMainMenuButton;
		HyperlinkWidget* mNextButton;
		HyperlinkWidget* mBackButton;
		BubbleMgr* mBubbleMgr;
		Image* mTalkShowImg;
		int mStoryId;

	public:
		StoryScreen(WinFishApp* theApp, int theStoryId);
		virtual ~StoryScreen();

		virtual void			AddedToManager(WidgetManager* theWidgetManager);
		virtual void			RemovedFromManager(WidgetManager* theWidgetManager);
		virtual void			Update();
		void					UpdateWide();	// switchfish
		virtual void			Draw(Graphics* g);

		virtual void			ButtonPress(int theId);
		virtual void			ButtonDepress(int theId);

		void ChangeStory(int theDirection);
		void LoadTalkShow();
	};
}

#endif