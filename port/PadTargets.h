#pragma once

namespace Sexy
{
    class SexyAppBase;
}

// Quick aim for the controller (docs/QOL.md): theIndex'th target of theKind, 0 the hungriest fish, 1 the money
// that will be gone first. 1 found, 0 none, -1 not now. Installed as gPadTargetHook (framework/platform/PadCursor.h).
int FindPadTarget(Sexy::SexyAppBase* theApp, int theKind, int theIndex, int& theX, int& theY);
