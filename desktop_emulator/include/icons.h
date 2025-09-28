#pragma once
#include "desktop_display_sdl2.h"
#include <cmath>

inline void drawIconPokeball(DesktopDisplay& d, int cx, int cy, int r) {
    // Outer ring (2px thickness)
    d.einkDrawCircle(cx, cy, r, /*filled*/false, /*black*/true);
    d.einkDrawCircle(cx, cy, r-1, false, true);

    // Center seam
    d.einkDrawLine(cx - r, cy, cx + r, cy, true);
    d.einkDrawLine(cx - r, cy+1, cx + r, cy+1, true);

    // Button ring + fill
    int br = std::max(2, r/4);
    d.einkDrawCircle(cx, cy, br+2, false, true);
    d.einkDrawCircle(cx, cy, br, /*filled*/true, /*black*/true);
}

inline void drawIconHydrogen(DesktopDisplay& d, int cx, int cy, int r) {
    // Nucleus
    int nr = std::max(2, r/5);
    d.einkDrawCircle(cx, cy, nr, /*filled*/true, /*black*/true);

    // Single circular orbit
    d.einkDrawCircle(cx, cy, r, /*filled*/false, /*black*/true);

    // Electron (dot) at 45°
    double ang = M_PI/4.0;
    int ex = cx + int(std::round(r * std::cos(ang)));
    int ey = cy - int(std::round(r * std::sin(ang)));
    d.einkDrawCircle(ex, ey, std::max(1, r/8), /*filled*/true, /*black*/true);
}
