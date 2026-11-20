#pragma once

#include <string>

// A message over an animated aquarium, shown before the game starts.
struct Notice
{
    std::string title;
    std::string body;
    bool canContinue = false;
};

// True when the player chose to continue, false to exit.
bool ShowNotice(const Notice& notice);
