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

    enum class ProcessingStage {
        Assembly,
        QualityCheck,
        QualityReview,
        Disassembly
    };

    struct ProcessingJob {
        int work_item_id = 0;
        ProcessingStage stage = ProcessingStage::Assembly;
        double remaining_seconds = 0.0;
    };

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
    void ForceReject();
    void UpdateFrame();
    void TryAssemble();
    void StartProcessingJob(int work_item_id, ProcessingStage stage, double seconds);
    void TickProcessing();
    void SpawnPart(const String& edge_id, VisualTokenKind kind, const String& label, Color c, bool recycle = false, int work_item_id = 0);
    void SpawnManufacturingToken(const String& edge_id, VisualTokenKind kind, const String& label, Color c, double speed = 1.0, int work_item_id = 0);
    void ProcessArrival(const VisualToken& token);
    void ProcessCheckResult(int work_item_id, bool force_review);
    void ProcessReviewResult(int work_item_id);
    void ProcessDisassemblyResult(int work_item_id);
    void UpdateNodeStats();
    void UpdateMetrics();
    void SyncGraph();
    void SetControlStyle();
    String StatusText() const;

private:
    VisualizerModel model_;
    StateMachine control_;
    bool running_ = false;
    bool force_next_review_ = false;
    bool force_next_reject_ = false;
    int review_probability_ = 40;
    int reject_probability_ = 50;
    int auto_ingest_rate_ = 4;
    int package_size_ = 5;
    int next_work_item_id_ = 0;
    int accepted_units_ = 0;
    int shipped_units_ = 0;
    double last_tick_ms_ = 0.0;
    double generation_accumulator_ = 0.0;
    double flow_speed_ = 1.0;
    Array<ProcessingJob> processing_jobs_;
    TimeCallback tick_;

    GraphView graph_;
    VisualLogPanel log_;
    GuidePanel guide_;

    UiButton run_pause_btn_;
    UiButton inject_a_btn_;
    UiButton inject_b_btn_;
    UiButton force_review_btn_;
    UiButton force_reject_btn_;
    UiButton reset_btn_;

    UiSlider speed_slider_;
    UiSlider ingest_slider_;
    UiSlider review_slider_;
    UiSlider reject_slider_;
    UiSlider package_slider_;

    Label title_label_;
    Label subtitle_label_;
    Label status_label_;
    Label speed_caption_;
    Label speed_value_;
    Label ingest_caption_;
    Label ingest_value_;
    Label review_caption_;
    Label review_value_;
    Label reject_caption_;
    Label reject_value_;
    Label package_caption_;
    Label package_value_;
    Label counters_label_;
    Label buffer_label_;
};

}

#endif
