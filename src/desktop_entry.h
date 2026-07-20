#pragma once

#include "item.h"
#include <QString>

class DesktopEntry {
public:
    // Given an app id like "firefox", "kitty", "org.gnome.Nautilus",
    // searches XDG application directories and returns a fully populated MenuItem.
    // Returns an empty MenuItem (label is empty) if not found.
    static MenuItem resolve(const QString &appId);
};
