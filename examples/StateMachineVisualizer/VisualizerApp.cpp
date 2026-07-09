#include "VisualizerApp.h"
#include <Ui/UiTheme.h>

namespace Upp {

static Color VizBg()        { return Color(7, 11, 19); }
static Color VizPanelBg()   { return Color(9, 13, 22); }
static Color VizHeaderBg()  { return Color(13, 19, 36); }
static Color VizBorder()    { return Color(30, 41, 59); }
static Color VizInk()       { return Color(226, 232, 240); }
static Color VizMutedInk()  { return Color(148, 163, 184); }
static Color VizCyan()      { return Color(56, 189, 248); }
static Color VizTeal()      { return Color(45, 212, 191); }
static Color VizGreen()     { return Color(16, 185, 129); }
static Color VizAmber()     { return Color(245, 158, 11); }
static Color VizRed()       { return Color(239, 68, 68); }
static Color VizViolet()    { return Color(124, 58, 237); }

void VisualLogPanel::Paint(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(4, 8, 15));
    w.DrawRect(0, 0, sz.cx, DPI(24), VizPanelBg());
    w.DrawText(DPI(12), DPI(6), "Manufacturing Log", SansSerifZ(10).Bold(), VizMutedInk());
    w.DrawLine(0, DPI(24), sz.cx, DPI(24), 1, VizBorder());

    if(!model_)
        return;

    int y = DPI(32);
    int first = max(0, model_->log.GetCount() - 8);
    for(int i = first; i < model_->log.GetCount(); i++) {
        const VisualLogEntry& e = model_->log[i];
        Color c = VizInk();
        if(e.kind == "success") c = VizGreen();
        else if(e.kind == "warning") c = VizAmber();
        else if(e.kind == "system") c = VizCyan();
        else if(e.kind == "alert") c = VizRed();

        w.DrawText(DPI(12), y, "[" + e.source + "]", MonospaceZ(10).Bold(), VizMutedInk());
        w.DrawText(DPI(112), y, e.message, MonospaceZ(10), c);
        y += DPI(16);
    }
}

void GuidePanel::Paint(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(11, 17, 31));
    w.DrawRect(0, 0, sz.cx, DPI(24), VizPanelBg());
    w.DrawText(DPI(12), DPI(6), "Legend", SansSerifZ(10).Bold(), VizMutedInk());
    w.DrawLine(0, DPI(24), sz.cx, DPI(24), 1, VizBorder());

    int x = DPI(14);
    int y = DPI(36);
    w.DrawText(x, y, "A / B  parts", SansSerifZ(10).Bold(), VizCyan()); y += DPI(16);
    w.DrawText(x, y, "U  assembled unit", SansSerifZ(10).Bold(), VizGreen()); y += DPI(16);
    w.DrawText(x, y, "R  review unit", SansSerifZ(10).Bold(), VizAmber()); y += DPI(16);
    w.DrawText(x, y, "X  rejected/recycled", SansSerifZ(10).Bold(), VizRed()); y += DPI(16);
    w.DrawText(x, y, "N  shipment batch", SansSerifZ(10).Bold(), VizViolet());
}

VisualizerApp::VisualizerApp()
{
    Title("StateMachine Manufacturing Visualizer").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1200), DPI(780));

    BuildControlMachine();
    model_.ResetManufacturingGraph();

    Add(title_label_);
    Add(subtitle_label_);
    Add(status_label_);
    Add(counters_label_);
    Add(buffer_label_);
    Add(run_pause_btn_);
    Add(inject_a_btn_);
    Add(inject_b_btn_);
    Add(force_review_btn_);
    Add(force_reject_btn_);
    Add(reset_btn_);
    Add(speed_slider_);
    Add(ingest_slider_);
    Add(review_slider_);
    Add(reject_slider_);
    Add(package_slider_);
    Add(speed_caption_);
    Add(speed_value_);
    Add(ingest_caption_);
    Add(ingest_value_);
    Add(review_caption_);
    Add(review_value_);
    Add(reject_caption_);
    Add(reject_value_);
    Add(package_caption_);
    Add(package_value_);
    Add(graph_);
    Add(log_);
    Add(guide_);

    title_label_.SetLabel("Manufacturing Flow Visualizer");
    title_label_.SetInk(White());
    title_label_.SetFont(SansSerifZ(16).Bold());
    subtitle_label_.SetLabel("Parts, assembly, inspection, review, recycling, packaging, shipping.");
    subtitle_label_.SetInk(VizMutedInk());
    subtitle_label_.SetFont(SansSerifZ(11));

    run_pause_btn_.SetText("Run");
    inject_a_btn_.SetText("Inject A");
    inject_b_btn_.SetText("Inject B");
    force_review_btn_.SetText("Force Review");
    force_reject_btn_.SetText("Force Reject");
    reset_btn_.SetText("Reset");

    speed_slider_.SetRange(0.5, 3.0).SetStep(0.25).SetValue(1.0).SetSizeMin(DPI(120), DPI(24));
    ingest_slider_.SetRange(0, 25).SetStep(1).SetValue(4).SetSizeMin(DPI(120), DPI(24));
    review_slider_.SetRange(0, 100).SetStep(5).SetValue(40).SetSizeMin(DPI(120), DPI(24));
    reject_slider_.SetRange(0, 100).SetStep(5).SetValue(50).SetSizeMin(DPI(120), DPI(24));
    package_slider_.SetRange(1, 20).SetStep(1).SetValue(5).SetSizeMin(DPI(120), DPI(24));
    speed_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Accent));
    ingest_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Subtle));
    review_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Subtle));
    reject_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Alert));
    package_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Accent));
    for(Label* l : { &speed_caption_, &speed_value_, &ingest_caption_, &ingest_value_, &review_caption_, &review_value_, &reject_caption_, &reject_value_, &package_caption_, &package_value_ }) {
        l->SetInk(VizMutedInk());
        l->SetFont(SansSerifZ(10).Bold());
    }
    speed_value_.SetFont(MonospaceZ(10).Bold());
    ingest_value_.SetFont(MonospaceZ(10).Bold());
    review_value_.SetFont(MonospaceZ(10).Bold());
    reject_value_.SetFont(MonospaceZ(10).Bold());
    package_value_.SetFont(MonospaceZ(10).Bold());

    SetControlStyle();

    run_pause_btn_.WhenAction = [=] { ToggleRunPause(); };
    inject_a_btn_.WhenAction = [=] { InjectPartA(); };
    inject_b_btn_.WhenAction = [=] { InjectPartB(); };
    force_review_btn_.WhenAction = [=] {
        force_next_review_ = true;
        model_.AddLog("Config", "Next quality check will be routed to review.", "system");
        SyncGraph();
    };
    force_reject_btn_.WhenAction = [=] {
        force_next_reject_ = true;
        model_.AddLog("Config", "Next quality review will be rejected.", "system");
        SyncGraph();
    };
    reset_btn_.WhenAction = [=] { ResetScenario(); };

    speed_slider_.WhenAction = [=] {
        model_.AddLog("Config", Format("Flow speed set to %.2fx.", speed_slider_.GetValue()), "system");
    };
    ingest_slider_.WhenAction = [=] {
        auto_ingest_rate_ = (int)ingest_slider_.GetValue();
        model_.AddLog("Config", Format("Auto ingest set to %d/s.", auto_ingest_rate_), "system");
    };
    review_slider_.WhenAction = [=] {
        review_probability_ = (int)review_slider_.GetValue();
        model_.AddLog("Config", Format("Review rate set to %d%%.", review_probability_), "system");
    };
    reject_slider_.WhenAction = [=] {
        reject_probability_ = (int)reject_slider_.GetValue();
        model_.AddLog("Config", Format("Reject rate set to %d%%.", reject_probability_), "system");
    };
    package_slider_.WhenAction = [=] {
        package_size_ = (int)package_slider_.GetValue();
        model_.AddLog("Config", Format("Package size set to %d.", package_size_), "system");
    };

    graph_.SetModel(&model_);
    log_.SetModel(&model_);
    UpdateMetrics();
    ResetScenario();
    last_tick_ms_ = (double)msecs();
    tick_.Set(16, [=] { UpdateFrame(); });
}

void VisualizerApp::BuildControlMachine()
{
    control_.Clear();
    control_.AddState({"PAUSED", {}, {}});
    control_.AddState({"RUNNING", {}, {}});
    control_.SetInitial("PAUSED");
    control_.AddTransition({"run",   "PAUSED",  "RUNNING"});
    control_.AddTransition({"pause", "RUNNING", "PAUSED"});
    if(!control_.Start())
        model_.AddLog("FSM", control_.GetLastErrorText(), "alert");
}

void VisualizerApp::SetControlStyle()
{
    run_pause_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    inject_a_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    inject_b_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    force_review_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    reset_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
}

String VisualizerApp::StatusText() const
{
    return running_ ? "Running" : "Paused";
}

void VisualizerApp::ResetScenario()
{
    tick_.Kill();
    running_ = false;
    force_next_review_ = false;
    force_next_reject_ = false;
    review_probability_ = 40;
    reject_probability_ = 50;
    auto_ingest_rate_ = 4;
    package_size_ = 5;
    review_slider_.SetData(40);
    reject_slider_.SetData(50);
    ingest_slider_.SetData(4);
    package_slider_.SetData(5);
    flow_speed_ = 1.0;
    speed_slider_.SetData(1.0);
    generation_accumulator_ = 0.0;
    processing_jobs_.Clear();
    next_work_item_id_ = 0;
    accepted_units_ = 0;
    shipped_units_ = 0;
    last_tick_ms_ = (double)msecs();

    model_.ResetManufacturingGraph();
    BuildControlMachine();
    UpdateNodeStats();
    model_.AddLog("System", "Scenario reset.", "system");
    SyncGraph();
    run_pause_btn_.SetText("Run");
    status_label_.SetLabel("Paused");
    tick_.Set(16, [=] { UpdateFrame(); });
}

void VisualizerApp::ToggleRunPause()
{
    if(running_) {
        running_ = false;
        control_.TriggerEvent("pause");
        run_pause_btn_.SetText("Run");
        model_.AddLog("Control", "Paused.", "system");
    }
    else {
        running_ = true;
        control_.TriggerEvent("run");
        run_pause_btn_.SetText("Pause");
        model_.AddLog("Control", "Running.", "success");
    }
    status_label_.SetLabel(StatusText());
}

void VisualizerApp::SpawnManufacturingToken(const String& edge_id, VisualTokenKind kind, const String& label, Color c, double speed, int work_item_id)
{
    const VisualEdgeSpec* e = model_.FindEdge(edge_id);
    if(!e) {
        model_.last_fsm_error = "Missing edge: " + edge_id;
        model_.AddLog("FSM", model_.last_fsm_error, "alert");
        return;
    }
    model_.AddToken(edge_id, kind, label, c, speed, work_item_id);
}

void VisualizerApp::SpawnPart(const String& edge_id, VisualTokenKind kind, const String& label, Color c, bool recycle, int work_item_id)
{
    SpawnManufacturingToken(edge_id, kind, label, c, recycle ? 0.9 : 1.0, work_item_id);
}

void VisualizerApp::InjectPartA()
{
    if(!running_) ToggleRunPause();
    SpawnPart("gen_a_to_assembly", VisualTokenKind::PartA, "A", VizCyan());
    model_.part_a_generated++;
    model_.AddLog("Generator A", "Part A injected.", "success");
}

void VisualizerApp::InjectPartB()
{
    if(!running_) ToggleRunPause();
    SpawnPart("gen_b_to_assembly", VisualTokenKind::PartB, "B", VizTeal());
    model_.part_b_generated++;
    model_.AddLog("Generator B", "Part B injected.", "success");
}

void VisualizerApp::ForceReview()
{
    force_next_review_ = true;
    model_.AddLog("Config", "Next quality check will be routed to review.", "system");
    SyncGraph();
}

void VisualizerApp::ForceReject()
{
    force_next_reject_ = true;
    model_.AddLog("Config", "Next quality review will be rejected.", "system");
    SyncGraph();
}

void VisualizerApp::ProcessArrival(const VisualToken& token)
{
    const VisualEdgeSpec* e = model_.FindEdge(token.edge_id);
    if(!e)
        return;

    VisualNodeSpec* node = model_.FindNode(e->to);
    if(!node)
        return;

    if(token.kind == VisualTokenKind::PartA && e->to == "ASSEMBLY") {
        node->part_a++;
        model_.AddLog("Assembly", "Part A waiting in collector.", "system");
        TryAssemble();
    }
    else if(token.kind == VisualTokenKind::PartB && e->to == "ASSEMBLY") {
        node->part_b++;
        model_.AddLog("Assembly", "Part B waiting in collector.", "system");
        TryAssemble();
    }
    else if(token.kind == VisualTokenKind::AssembledUnit && e->to == "QUALITY_CHECK") {
        node->assembled++;
        model_.units_checking++;
        StartProcessingJob(token.work_item_id, ProcessingStage::QualityCheck, 0.80);
        model_.AddLog("Quality Check", "Assembled unit arrived for inspection.", "system");
    }
    else if(token.kind == VisualTokenKind::ReviewUnit && e->to == "QUALITY_REVIEW") {
        node->under_review++;
        model_.units_under_review++;
        StartProcessingJob(token.work_item_id, ProcessingStage::QualityReview, 1.00);
        model_.AddLog("Quality Review", "Unit queued for review.", "system");
    }
    else if(token.kind == VisualTokenKind::RejectedUnit && e->to == "DISASSEMBLY") {
        node->rejected++;
        model_.rejected_units++;
        StartProcessingJob(token.work_item_id, ProcessingStage::Disassembly, 0.80);
        model_.AddLog("Disassembly", "Rejected unit received.", "warning");
    }
    else if(token.kind == VisualTokenKind::ShipmentBatch && e->to == "SHIPPING") {
        node->shipping++;
        model_.completed_shipments++;
        shipped_units_ += 5;
        model_.AddLog("Shipping", "Shipment batch completed.", "success");
    }
    else if(token.kind == VisualTokenKind::RecycledPartA && e->to == "ASSEMBLY") {
        node->recycled++;
        model_.recycled_units++;
        if(VisualNodeSpec* a = model_.FindNode("ASSEMBLY"))
            a->part_a++;
        model_.AddLog("Assembly", "Recovered Part A returned.", "warning");
    }
    else if(token.kind == VisualTokenKind::RecycledPartB && e->to == "ASSEMBLY") {
        node->recycled++;
        model_.recycled_units++;
        if(VisualNodeSpec* a = model_.FindNode("ASSEMBLY"))
            a->part_b++;
        model_.AddLog("Assembly", "Recovered Part B returned.", "warning");
    }
    else if(token.kind == VisualTokenKind::AssembledUnit && e->to == "PACKAGING") {
        if(VisualNodeSpec* p = model_.FindNode("PACKAGING")) {
            p->packaging_buffer++;
            model_.accepted_units_waiting++;
            accepted_units_++;
            model_.AddLog("Packaging", Format("Packaging buffer %d / 5", p->packaging_buffer), "system");
            if(p->packaging_buffer >= package_size_) {
                p->packaging_buffer -= package_size_;
                model_.accepted_units_waiting = max(0, model_.accepted_units_waiting - package_size_);
                SpawnManufacturingToken("packaging_to_shipping", VisualTokenKind::ShipmentBatch, Format("%d", package_size_), VizViolet(), 0.95, token.work_item_id);
                model_.AddLog("Packaging", Format("%d accepted units became one shipment.", package_size_), "success");
            }
        }
    }
}

void VisualizerApp::ProcessCheckResult(int work_item_id, bool force_review)
{
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_CHECK")) {
        if(n->assembled <= 0)
            return;
        n->assembled--;
        model_.units_checking = max(0, model_.units_checking - 1);
    }
    bool review = force_review || Random(100) < review_probability_;
    if(review) {
        SpawnManufacturingToken("check_review_to_quality_review", VisualTokenKind::ReviewUnit, "R", VizAmber(), 1.0, work_item_id);
        model_.AddLog("Quality Check", Format("[Unit %d] Routed to review.", work_item_id), "warning");
    }
    else {
        SpawnManufacturingToken("check_pass_to_packaging", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0, work_item_id);
        model_.AddLog("Quality Check", Format("[Unit %d] Passed inspection.", work_item_id), "success");
    }
}

void VisualizerApp::ProcessReviewResult(int work_item_id)
{
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_REVIEW")) {
        if(n->under_review <= 0)
            return;
        n->under_review--;
        model_.units_under_review = max(0, model_.units_under_review - 1);
    }
    bool rejected = force_next_reject_ || Random(100) < reject_probability_;
    force_next_reject_ = false;
    if(rejected) {
        SpawnManufacturingToken("review_reject_to_disassembly", VisualTokenKind::RejectedUnit, "X", VizRed(), 1.0, work_item_id);
        model_.AddLog("Quality Review", Format("[Unit %d] Rejected.", work_item_id), "alert");
    }
    else {
        SpawnManufacturingToken("review_approve_to_packaging", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0, work_item_id);
        model_.AddLog("Quality Review", Format("[Unit %d] Approved.", work_item_id), "success");
    }
}

void VisualizerApp::ProcessDisassemblyResult(int work_item_id)
{
    SpawnManufacturingToken("disassembly_to_assembly_a", VisualTokenKind::RecycledPartA, "A", Color(239, 140, 79), 0.95, work_item_id);
    SpawnManufacturingToken("disassembly_to_assembly_b", VisualTokenKind::RecycledPartB, "B", Color(239, 140, 79), 0.95, work_item_id);
    model_.AddLog("Disassembly", Format("[Unit %d] Recycled into A and B.", work_item_id), "warning");
}

void VisualizerApp::TryAssemble()
{
    VisualNodeSpec* n = model_.FindNode("ASSEMBLY");
    if(!n || n->part_a <= 0 || n->part_b <= 0)
        return;
    n->part_a--;
    n->part_b--;
    model_.units_assembled++;
    StartProcessingJob(++next_work_item_id_, ProcessingStage::Assembly, 0.50);
    model_.AddLog("Assembly", "One A + one B entered assembly processing.", "success");
}

void VisualizerApp::UpdateFrame()
{
    if(!IsOpen())
        return;

    double now = (double)msecs();
    double dt = (now - last_tick_ms_) / 1000.0;
    last_tick_ms_ = now;
    if(dt < 0.0)
        dt = 0.0;
    dt = min(dt, 0.1);

    if(running_) {
        flow_speed_ = max(0.5, speed_slider_.GetValue());
        double interval = auto_ingest_rate_ > 0 ? 1.0 / auto_ingest_rate_ : 0.0;
        if(interval > 0.0) {
            generation_accumulator_ += dt * flow_speed_;
            while(generation_accumulator_ >= interval) {
                generation_accumulator_ -= interval;
                if(Random(100) < 88) {
                    SpawnPart("gen_a_to_assembly", VisualTokenKind::PartA, "A", VizCyan());
                    model_.part_a_generated++;
                }
                if(Random(100) < 88) {
                    SpawnPart("gen_b_to_assembly", VisualTokenKind::PartB, "B", VizTeal());
                    model_.part_b_generated++;
                }
            }
        }

        for(int i = model_.tokens.GetCount() - 1; i >= 0; i--) {
            VisualToken& token = model_.tokens[i];
            token.progress += dt * token.speed * flow_speed_;
        }

        Vector<VisualToken> arrivals;
        for(int i = model_.tokens.GetCount() - 1; i >= 0; i--) {
            if(model_.tokens[i].progress < 1.0)
                continue;
            arrivals.Add(model_.tokens[i]);
            model_.tokens.Remove(i);
        }

        for(int i = 0; i < arrivals.GetCount(); i++)
            ProcessArrival(arrivals[i]);

        TickProcessing();
        TryAssemble();
    }

    UpdateNodeStats();
    SyncGraph();
    tick_.Set(16, [=] { UpdateFrame(); });
}

void VisualizerApp::StartProcessingJob(int work_item_id, ProcessingStage stage, double seconds)
{
    ProcessingJob& job = processing_jobs_.Add();
    job.work_item_id = work_item_id;
    job.stage = stage;
    job.remaining_seconds = seconds;
}

void VisualizerApp::TickProcessing()
{
    for(int i = processing_jobs_.GetCount() - 1; i >= 0; i--) {
        ProcessingJob& job = processing_jobs_[i];
        job.remaining_seconds -= 0.016 * max(0.5, speed_slider_.GetValue());
        if(job.remaining_seconds > 0.0)
            continue;

        switch(job.stage) {
        case ProcessingStage::Assembly:
            SpawnManufacturingToken("assembly_to_check", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0, job.work_item_id);
            model_.AddLog("Assembly", Format("[Unit %d] Assembly complete.", job.work_item_id), "success");
            break;
        case ProcessingStage::QualityCheck:
            ProcessCheckResult(job.work_item_id, force_next_review_);
            force_next_review_ = false;
            break;
        case ProcessingStage::QualityReview:
            ProcessReviewResult(job.work_item_id);
            break;
        case ProcessingStage::Disassembly:
            ProcessDisassemblyResult(job.work_item_id);
            break;
        }
        processing_jobs_.Remove(i);
    }
}

void VisualizerApp::UpdateNodeStats()
{
    if(VisualNodeSpec* n = model_.FindNode("GEN_A")) {
        n->part_a = model_.part_a_generated;
        n->active = running_ && (n->part_a > 0 || !model_.tokens.IsEmpty());
    }
    if(VisualNodeSpec* n = model_.FindNode("GEN_B")) {
        n->part_b = model_.part_b_generated;
        n->active = running_ && (n->part_b > 0 || !model_.tokens.IsEmpty());
    }
    if(VisualNodeSpec* n = model_.FindNode("ASSEMBLY")) {
        n->assembled = model_.units_assembled;
        n->active = n->part_a > 0 || n->part_b > 0 || !processing_jobs_.IsEmpty();
    }
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_CHECK")) {
        n->assembled = model_.units_checking;
        n->active = n->assembled > 0;
    }
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_REVIEW")) {
        n->under_review = model_.units_under_review;
        n->active = n->under_review > 0;
    }
    if(VisualNodeSpec* n = model_.FindNode("PACKAGING")) {
        n->packaging_buffer = model_.accepted_units_waiting;
        n->active = n->packaging_buffer > 0;
    }
    if(VisualNodeSpec* n = model_.FindNode("DISASSEMBLY")) {
        n->rejected = model_.rejected_units;
        n->active = n->rejected > 0;
    }
    if(VisualNodeSpec* n = model_.FindNode("SHIPPING")) {
        n->shipping = model_.completed_shipments;
        n->active = n->shipping > 0;
    }
}

void VisualizerApp::UpdateMetrics()
{
    status_label_.SetLabel(Format("Status: %s", StatusText()));
    status_label_.SetInk(running_ ? VizGreen() : VizAmber());
    status_label_.SetFont(MonospaceZ(11).Bold());

    counters_label_.SetLabel(Format("Accepted:%d  Shipped:%d  Jobs:%d  Recycled:%d",
        accepted_units_, shipped_units_, processing_jobs_.GetCount(), model_.recycled_units));
    counters_label_.SetInk(VizCyan());
    counters_label_.SetFont(MonospaceZ(11).Bold());

    speed_caption_.SetLabel("Flow Speed");
    speed_value_.SetLabel(Format("%.1fx", speed_slider_.GetValue()));
    ingest_caption_.SetLabel("Auto Ingest");
    ingest_value_.SetLabel(Format("%d/s", auto_ingest_rate_));
    review_caption_.SetLabel("Review Rate");
    review_value_.SetLabel(Format("%d%%", review_probability_));
    reject_caption_.SetLabel("Reject Rate");
    reject_value_.SetLabel(Format("%d%%", reject_probability_));
    package_caption_.SetLabel("Package Size");
    package_value_.SetLabel(Format("%d", package_size_));
    if(VisualEdgeSpec* e = model_.FindEdge("packaging_to_shipping"))
        e->label = Format("Batch of %d", package_size_);
    buffer_label_.SetLabel(Format("Packaging: %d / %d   Last error: %s",
        model_.accepted_units_waiting, package_size_,
        model_.last_fsm_error.IsEmpty() ? String("none") : model_.last_fsm_error));
    buffer_label_.SetInk(VizMutedInk());
    buffer_label_.SetFont(MonospaceZ(10));
}

void VisualizerApp::SyncGraph()
{
    UpdateNodeStats();
    UpdateMetrics();
    graph_.SyncNodeCards();
    log_.Refresh();
    graph_.Refresh();
}

void VisualizerApp::Layout()
{
    Size sz = GetSize();
    const int header_h = DPI(64);
    const int controls_h = DPI(104);
    const int footer_h = DPI(156);

    title_label_.SetRect(DPI(22), DPI(10), DPI(430), DPI(24));
    subtitle_label_.SetRect(DPI(22), DPI(34), DPI(700), DPI(20));
    status_label_.SetRect(sz.cx - DPI(260), DPI(10), DPI(240), DPI(20));
    counters_label_.SetRect(sz.cx - DPI(480), DPI(32), DPI(460), DPI(20));
    buffer_label_.SetRect(DPI(22), DPI(56), sz.cx - DPI(44), DPI(18));

    int y = header_h + DPI(10);
    int x = DPI(18);
    run_pause_btn_.SetRect(x, y, DPI(82), DPI(30)); x += DPI(90);
    inject_a_btn_.SetRect(x, y, DPI(88), DPI(30)); x += DPI(96);
    inject_b_btn_.SetRect(x, y, DPI(88), DPI(30)); x += DPI(96);
    force_review_btn_.SetRect(x, y, DPI(112), DPI(30)); x += DPI(120);
    force_reject_btn_.SetRect(x, y, DPI(112), DPI(30)); x += DPI(120);
    reset_btn_.SetRect(x, y, DPI(82), DPI(30));

    int row_y = header_h + DPI(48);
    int col_x = DPI(18);
    int col_w = DPI(226);
    int label_w = DPI(140);
    int value_w = DPI(70);
    int label_h = DPI(14);

    speed_caption_.SetRect(col_x, row_y, label_w, label_h);
    speed_value_.SetRect(col_x + label_w, row_y, value_w, label_h);
    speed_slider_.SetRect(col_x, row_y + DPI(16), DPI(150), DPI(24));

    col_x += col_w;
    ingest_caption_.SetRect(col_x, row_y, label_w, label_h);
    ingest_value_.SetRect(col_x + label_w, row_y, value_w, label_h);
    ingest_slider_.SetRect(col_x, row_y + DPI(16), DPI(150), DPI(24));

    col_x += col_w;
    review_caption_.SetRect(col_x, row_y, label_w, label_h);
    review_value_.SetRect(col_x + label_w, row_y, value_w, label_h);
    review_slider_.SetRect(col_x, row_y + DPI(16), DPI(150), DPI(24));

    col_x += col_w;
    reject_caption_.SetRect(col_x, row_y, label_w, label_h);
    reject_value_.SetRect(col_x + label_w, row_y, value_w, label_h);
    reject_slider_.SetRect(col_x, row_y + DPI(16), DPI(150), DPI(24));

    col_x += col_w;
    package_caption_.SetRect(col_x, row_y, label_w, label_h);
    package_value_.SetRect(col_x + label_w, row_y, value_w, label_h);
    package_slider_.SetRect(col_x, row_y + DPI(16), DPI(150), DPI(24));

    int graph_top = header_h + controls_h;
    int graph_h = max(0, sz.cy - graph_top - footer_h);
    graph_.SetRect(0, graph_top, sz.cx, graph_h);

    int footer_y = sz.cy - footer_h;
    int log_w = sz.cx * 3 / 5;
    log_.SetRect(0, footer_y, log_w, footer_h);
    guide_.SetRect(log_w, footer_y, sz.cx - log_w, footer_h);
}

void VisualizerApp::Paint(Draw& w)
{
    Size sz = GetSize();
    const int header_h = DPI(64);
    const int controls_h = DPI(104);

    w.DrawRect(sz, VizBg());
    w.DrawRect(0, 0, sz.cx, header_h, VizHeaderBg());
    w.DrawLine(0, header_h - 1, sz.cx, header_h - 1, 1, VizBorder());
    w.DrawRect(0, header_h, sz.cx, controls_h, VizPanelBg());
    w.DrawLine(0, header_h + controls_h - 1, sz.cx, header_h + controls_h - 1, 1, VizBorder());
}

}
