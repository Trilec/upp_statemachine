/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    StateMachineVisualizer entry point
    ==================================

    Purpose
    - Runnable GUI entry point for the optional animated visualizer example.

    Intent
    - Keep the example separate from the Core-only StateMachine package.
    - Provide a first visual scaffold Gary can compile and incrementally harden.
*/

#include "VisualizerApp.h"

using namespace Upp;

GUI_APP_MAIN
{
    Ctrl::GlobalBackPaint();
    VisualizerApp().Run();
}
