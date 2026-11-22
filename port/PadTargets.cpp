#include "PadTargets.h"
#include "WinFishApp.h"
#include "Board.h"
#include "Coin.h"
#include "Fish.h"
#include "Breeder.h"
#include "widget/WidgetManager.h"
#include <algorithm>
#include <vector>

using namespace Sexy;

namespace {

struct Target
{
    GameObject* mObject;
    int mRank;  // lower first
};

void AddFish(std::vector<Target>& theTargets, GameObject* theFish)
{
    // only fish the game already shows as hungry; the hungriest first
    if (theFish->mVisible && theFish->IsHungryVisible())
    {
        theTargets.push_back({theFish, theFish->mHunger});
    }
}

void AddCoin(std::vector<Target>& theTargets, Coin* theCoin)
{
    // collected, the notes (not clickable) and beetles too young to click (Coin::MouseDown)
    if (!theCoin->mVisible || theCoin->m0x198 || theCoin->mCoinType == COIN_NOTE ||
        (theCoin->mCoinType == 18 && theCoin->mUpdateCnt < 12))
    {
        return;
    }
    // bombs first (they kill the fish they touch), then what has lain on the floor longest (it vanishes
    // first), then what falls lowest
    int rank;
    if (theCoin->mCoinType == COIN_SHRAPNEL_BOMB)
    {
        rank = -100000;
    }
    else if (theCoin->mYD >= 370.0)
    {
        rank = -10000 - theCoin->m0x19c;
    }
    else
    {
        rank = -(int)theCoin->mYD;
    }
    theTargets.push_back({theCoin, rank});
}

} // namespace

int FindPadTarget(SexyAppBase* theApp, int theKind, int theIndex, bool theRepeat, int& theX, int& theY,
    bool& theClickable)
{
    WinFishApp* app = (WinFishApp*)theApp;
    Board* board = app->mBoard;
    if (!board || board->mPause || !app->mDialogList.empty() || !board->mVisible)
    {
        return -1;
    }
    std::vector<Target> targets;
    if (theKind == 0)
    {
        for (Fish* f : *board->mFishList)
        {
            AddFish(targets, f);
        }
        for (Breeder* b : *board->mBreederList)
        {
            AddFish(targets, b);
        }
    }
    else
    {
        // a bonus round is all about clicking money, the button would take that away
        if (board->mIsBonusRound)
        {
            return -1;
        }
        for (Coin* c : *board->mCoinList)
        {
            AddCoin(targets, c);
        }
        for (Coin* c : *board->mNikoPearlCoinList)
        {
            AddCoin(targets, c);
        }
    }
    if (targets.empty())
    {
        return 0;
    }
    std::stable_sort(targets.begin(), targets.end(), [](const Target& a, const Target& b) { return a.mRank < b.mRank; });
    GameObject* target = targets[theIndex % targets.size()].mObject;
    theX = target->mX + target->mWidth / 2;
    theY = target->mY + target->mHeight / 2;
    if (theKind == 0)
    {
        // food falls, so drop it a little above the fish rather than on it, where it would fall past and be
        // chased: about 10 above near the top of the tank, 40 near the floor, and never up into the
        // buttons at the top
        const int top = 90, floor = 400;
        int lead = 10 + 30 * std::clamp(theY - top, 0, floor - top) / (floor - top);
        theY = std::max(top, theY - lead);
    }
    // the click has to reach the target (or the tank behind it, when fish ignore the mouse): a pet on top,
    // like a charged Amp, would take it
    int aWidgetX, aWidgetY;
    Widget* aHit = app->mWidgetManager->GetWidgetAt(theX, theY, &aWidgetX, &aWidgetY);
    theClickable = aHit == target || aHit == board;
    // held, feeding stops when the food can't be paid for, instead of buzzing every repeat
    if (theRepeat && theKind == 0 && app->mGameMode != GAMEMODE_VIRTUAL_TANK && board->mMoney < board->m0x4ac)
    {
        theClickable = false;
    }
    return 1;
}
