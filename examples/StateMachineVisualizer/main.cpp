/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    StateMachineVisualizer entry point
    ==================================

    Purpose
    - Runnable GUI entry point for the optional manufacturing-flow visualizer.

    Intent
    - Keep the example separate from the Core-only StateMachine package.
    - Launch the manufacturing-flow demo cleanly.
*/

#include "VisualizerApp.h"

using namespace Upp;

GUI_APP_MAIN
{
    Ctrl::GlobalBackPaint();
    VisualizerApp().Run();
}
