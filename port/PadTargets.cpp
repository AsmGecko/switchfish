#include "PadTargets.h"
#include "WinFishApp.h"
#include "Board.h"
#include "Coin.h"
#include "Fish.h"
#include "Breeder.h"
#include "Food.h"
#include "Alien.h"
#include "Missle.h"
#include "Bilaterus.h"
#include "BilaterusHead.h"
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

// not collected yet, clickable at all (not a note, not the Virtual Tank feeder's star food), not a beetle too
// young to click (Coin::MouseDown)
bool Collectable(Coin* theCoin)
{
    return theCoin->mVisible && theCoin->mMouseVisible && !theCoin->m0x198 && theCoin->mCoinType != COIN_NOTE &&
        !(theCoin->mCoinType == 18 && theCoin->mUpdateCnt < 12);
}

void AddCoin(std::vector<Target>& theTargets, Coin* theCoin)
{
    if (!Collectable(theCoin))
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

// Where Fish, Breeder and Board::MouseDown take a click to drop food
bool InFoodZone(int x, int y)
{
    return x > TankLeft() + 30 && x < TankRight() - 53 && y > 60 && y < 400;
}

// A click there drops food: on the water, or on any fish that drops food when clicked (a guppy or a breeder).
// Not on a pet or a coin in front.
bool FeedsAt(WinFishApp* app, int x, int y)
{
    int wx, wy;
    Widget* hit = app->mWidgetManager->GetWidgetAt(x, y, &wx, &wy);
    if (hit == app->mBoard)
    {
        return true;
    }
    Fish* fish = dynamic_cast<Fish*>(hit);
    return (fish && fish->mIsGuppy) || dynamic_cast<Breeder*>(hit);
}

// A click there collects money: this coin, or another one lying on top of it
bool CollectsAt(WinFishApp* app, GameObject* coin, int x, int y)
{
    int wx, wy;
    Widget* hit = app->mWidgetManager->GetWidgetAt(x, y, &wx, &wy);
    Coin* other = dynamic_cast<Coin*>(hit);
    return hit == coin || (other && Collectable(other));
}

int PelletsInTank(Board* board)
{
    int n = 0;
    for (Food* f : *board->mFoodList)
    {
        // the ones DropFood counts against the food limit
        if (f->m0x18c == 0 && f->mExoticFoodType == 0)
        {
            n++;
        }
    }
    return n;
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
    theClickable = false;
    if (theKind == 0)
    {
        // Fish/Breeder::MouseDown drop the pellet 20 right of and 10 below the click, and a click inside the
        // fish (x and y 10..70) makes it uneatable for up to 30 ticks, so it sinks past and is chased. The
        // fish's left strip (x < 10) has no such timer and puts the pellet in its mouth (the eating box is
        // +-15 around its middle, 5 higher on a breeder); against the left wall or the floor that is outside
        // where food may be dropped, and the top strip (y < 10), also without a timer, drops it just above.
        int mouthY = dynamic_cast<Breeder*>(target) ? 35 : 40;
        const int spots[2][2] = {{target->mX + 8, target->mY + mouthY - 10}, {target->mX + 22, target->mY + 5}};
        for (const auto& spot : spots)
        {
            if (InFoodZone(spot[0], spot[1]) && FeedsAt(app, spot[0], spot[1]))
            {
                theX = spot[0];
                theY = spot[1];
                theClickable = true;
                break;
            }
        }
        if (!theClickable)
        {
            theX = spots[0][0];
            theY = spots[0][1];
        }
        // held, feeding pauses while the food can't be paid for or the tank holds all the pellets the food
        // upgrades allow, instead of clicking for nothing every repeat
        if (theRepeat && app->mGameMode != GAMEMODE_VIRTUAL_TANK &&
            (board->mMoney < board->m0x4ac || PelletsInTank(board) >= gFoodLimit))
        {
            theClickable = false;
        }
    }
    else
    {
        // a fish or a pet in front of the coin would take the click: try more of the coin
        for (int dy : {0, -14, 14})
        {
            for (int dx : {0, -14, 14})
            {
                if (!theClickable && CollectsAt(app, target, theX + dx, theY + dy))
                {
                    theX += dx;
                    theY += dy;
                    theClickable = true;
                }
            }
        }
    }
    return 1;
}

void AutoCollect(SexyAppBase* theApp)
{
    WinFishApp* app = (WinFishApp*)theApp;
    Board* board = app->mBoard;
    // like Quick Aim: never in a bonus round, which is all about clicking money
    if (!app->mAutoCollect || !board || !board->mVisible || board->mPause || board->mCovered ||
        !app->mDialogList.empty() || board->mIsBonusRound)
    {
        return;
    }
    for (std::vector<Coin*>* list : {board->mCoinList, board->mNikoPearlCoinList})
    {
        for (size_t i = 0; i < list->size(); i++)
        {
            Coin* coin = list->at(i);
            // half a second to see it drop first
            if (!Collectable(coin) || coin->mUpdateCnt < 18)
            {
                continue;
            }
            // under an alien Coin::MouseDown would fire at the alien instead
            int x = coin->mX + coin->mWidth / 2, y = coin->mY + coin->mHeight / 2;
            bool covered = false;
            for (Alien* alien : *board->mAlienList)
            {
                covered |= alien->Contains(x, y);
            }
            for (Missle* missle : *board->mMissleList1)
            {
                covered |= missle->Contains(x, y);
            }
            for (Bilaterus* bilaterus : *board->mBilaterusList)
            {
                covered |= bilaterus->mActiveHead && bilaterus->mActiveHead->Contains(x, y);
            }
            if (!covered)
            {
                coin->MouseDown(coin->mWidth / 2, coin->mHeight / 2, 1);
            }
        }
    }
}
