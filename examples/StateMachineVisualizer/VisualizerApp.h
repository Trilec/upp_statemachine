#ifndef _StateMachineVisualizer_VisualizerApp_h_
#define _StateMachineVisualizer_VisualizerApp_h_

/*
    Apache License 2.0

    VisualizerApp
    =============

    Purpose
    - Top-level window shell for the manufacturing-flow visualizer.
*/

#include "GraphView.h"
#include <statemachine/statemachine.h>
#include <CtrlCore/CtrlCore.h>

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
    void BuildControlMachine();
    void ResetScenario();
    void ToggleRunPause();
    void InjectPartA();
    void InjectPartB();
    void ForceReview();
    void AdvanceScenario();
    void ProduceParts();
    void TryAssemble();
    void TickProcessing();
    void SpawnPart(const String& edge_id, VisualTokenKind kind, const String& label, Color c, bool recycle = false);
    void SpawnManufacturingToken(const String& edge_id, VisualTokenKind kind, const String& label, Color c, double speed = 1.0);
    void ProcessArrival(const VisualToken& token);
    void ProcessCheckResult(bool force_review);
    void ProcessReviewResult();
    void UpdateNodeStats();
    void UpdateMetrics();
    void SyncGraph();
    void SetControlStyle();
    int  AutoDelayMs() const;
    String StatusText() const;

private:
    VisualizerModel model_;
    StateMachine control_;
    bool running_ = false;
    bool force_next_review_ = false;
    int review_bias_ = 70;
    int approve_bias_ = 60;
    TimeCallback tick_;

    GraphView graph_;
    VisualLogPanel log_;
    GuidePanel guide_;

    UiButton run_pause_btn_;
    UiButton inject_a_btn_;
    UiButton inject_b_btn_;
    UiButton force_review_btn_;
    UiButton reset_btn_;

    UiSlider speed_slider_;
    UiSlider review_slider_;

    Label title_label_;
    Label subtitle_label_;
    Label status_label_;
    Label counters_label_;
    Label buffer_label_;
};

}

#endif
