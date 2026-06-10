// =============================================================================
// main.cpp - TRUSS BUSTER entry point
// =============================================================================

#include "tcApp.h"

int main() {
    tc::WindowSettings settings;
    settings.setSize(1280, 800);
    settings.setTitle("TRUSS BUSTER");

    return TC_RUN_APP(tcApp, settings);
}
