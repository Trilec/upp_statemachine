/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    VisualizerApp
    =============

    Purpose
    - Top-level window shell for the optional StateMachine visualizer example.

    Intent
    - Approximate the supplied HTML mockup with a dark/lightweight U++ Ui shell.
    - Keep the core StateMachine package untouched and Core-only.
    - Provide a clear scaffold Gary can compile, adjust, and extend.

    Thread context
    - U++ GUI thread.
*/

#ifndef _StateMachineVisualizer_VisualizerApp_h_
#define _StateMachineVisualizer_VisualizerApp_h_

#include "GraphView.h"
#include <statemachine/statemachine.h>

namespace Upp {

class VisualLogPanel : public Ctrl {
public:
    void SetModel(VisualizerModel* m) { model_ = m; Refresh(); }
    virtual void Paint(Draw& w) override;
private:
    VisualizerModel* model_ = nullptr;
};

class GuidePanel : public Ctrl {
public:
    virtual void Paint(Draw& w) override;
};

class VisualizerApp : public TopWindow {
public:
    typedef VisualizerApp CLASSNAME;

    VisualizerApp();

    virtual void Layout() override;
    virtual void Paint(Draw& w) override;

private:
    void BuildMachine();
    void ResetScenario();
    void TriggerNext();
    void TriggerQueueExample();
    void TriggerErrorExample();
    void UpdateMetrics();
    void SyncGraph();
    String CurrentVisualNode() const;

private:
    VisualizerModel model_;
    StateMachine machine_;

    GraphView graph_;
    VisualLogPanel log_;
    GuidePanel guide_;

    UiButton start_btn_;
    UiButton trigger_btn_;
    UiButton queue_btn_;
    UiButton error_btn_;
    UiButton reset_btn_;

    UiSlider speed_slider_;
    UiSlider queue_slider_;

    Label title_label_;
    Label subtitle_label_;
    Label active_label_;
    Label processed_label_;
    Label speed_label_;
    Label queue_label_;
};

}

#endif
