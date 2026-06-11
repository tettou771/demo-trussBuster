// =============================================================================
// main.cpp - TRUSS BUSTER entry point
// =============================================================================

#include "tcApp.h"
#include "Mobile.h"
#include <cstdlib>

int main() {
    tc::WindowSettings settings;
    // TB_WIN_W / TB_WIN_H override the size (responsive-layout testing)
    int w = std::getenv("TB_WIN_W") ? atoi(std::getenv("TB_WIN_W")) : 1280;
    int h = std::getenv("TB_WIN_H") ? atoi(std::getenv("TB_WIN_H")) : 800;
    settings.setSize(w, h);
    settings.setTitle("TRUSS BUSTER");

#ifdef __EMSCRIPTEN__
    // Mobile browsers: PBR shades every pixel, and a 3x-DPR retina canvas is
    // ~9x the pixels of CSS resolution — fps tanks and big physics timesteps
    // start tunneling. Render at 1x there (MSAA still smooths the edges).
    if (isMobileDevice()) settings.setHighDpi(false);
#endif

    return TC_RUN_APP(tcApp, settings);
}
