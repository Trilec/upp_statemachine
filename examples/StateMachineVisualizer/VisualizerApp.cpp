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
    w.DrawText(x, y, "5  shipment batch", SansSerifZ(10).Bold(), VizViolet());
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
    Add(reset_btn_);
    Add(speed_slider_);
    Add(review_slider_);
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
    reset_btn_.SetText("Reset");

    speed_slider_.SetRange(0.5, 3.0).SetStep(0.25).SetValue(1.0).SetSizeMin(DPI(120), DPI(24));
    review_slider_.SetRange(0, 100).SetStep(5).SetValue(70).SetSizeMin(DPI(120), DPI(24));
    speed_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Accent));
    review_slider_.SetCustomStyle(UiTheme::ResolveSlider(UiRole::Subtle));

    SetControlStyle();

    run_pause_btn_.WhenAction = [=] { ToggleRunPause(); };
    inject_a_btn_.WhenAction = [=] { InjectPartA(); };
    inject_b_btn_.WhenAction = [=] { InjectPartB(); };
    force_review_btn_.WhenAction = [=] {
        force_next_review_ = true;
        model_.AddLog("Config", "Next quality check will be routed to review.", "system");
        SyncGraph();
    };
    reset_btn_.WhenAction = [=] { ResetScenario(); };

    speed_slider_.WhenAction = [=] {
        model_.AddLog("Config", Format("Speed set to %.2f.", speed_slider_.GetValue()), "system");
        tick_.KillSet(AutoDelayMs(), [=] { AdvanceScenario(); });
    };
    review_slider_.WhenAction = [=] {
        review_bias_ = (int)review_slider_.GetValue();
        model_.AddLog("Config", Format("Review probability set to %d%% pass / %d%% review.", review_bias_, 100 - review_bias_), "system");
    };

    graph_.SetModel(&model_);
    log_.SetModel(&model_);
    UpdateMetrics();
    ResetScenario();
}

void VisualizerApp::BuildControlMachine()
{
    control_.Clear();
    control_.AddState({"PAUSED", {}, {}});
    control_.AddState({"RUNNING", {}, {}});
    control_.SetInitial("PAUSED");
    control_.AddTransition({"run",   "PAUSED",  "RUNNING"});
    control_.AddTransition({"pause", "RUNNING", "PAUSED"});
    control_.Start();
}

void VisualizerApp::SetControlStyle()
{
    run_pause_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Accent));
    inject_a_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    inject_b_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
    force_review_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Alert));
    reset_btn_.SetCustomStyle(UiTheme::ResolveButton(UiRole::Subtle));
}

int VisualizerApp::AutoDelayMs() const
{
    return clamp((int)(900.0 / max(0.5, speed_slider_.GetValue())), 70, 1400);
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
    review_bias_ = 70;
    approve_bias_ = 60;
    review_slider_.SetData(70);

    model_.ResetManufacturingGraph();
    BuildControlMachine();
    UpdateNodeStats();
    model_.AddLog("System", "Scenario reset.", "system");
    SyncGraph();
    run_pause_btn_.SetText("Run");
    status_label_.SetLabel("Paused");
    tick_.Set(AutoDelayMs(), [=] { AdvanceScenario(); });
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

void VisualizerApp::SpawnManufacturingToken(const String& edge_id, VisualTokenKind kind, const String& label, Color c, double speed)
{
    const VisualEdgeSpec* e = model_.FindEdge(edge_id);
    if(!e) {
        model_.last_fsm_error = "Missing edge: " + edge_id;
        model_.AddLog("FSM", model_.last_fsm_error, "alert");
        return;
    }
    model_.AddToken(edge_id, kind, label, c, speed);
}

void VisualizerApp::SpawnPart(const String& edge_id, VisualTokenKind kind, const String& label, Color c, bool recycle)
{
    SpawnManufacturingToken(edge_id, kind, label, c, recycle ? 0.9 : 1.0);
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
    }
    else if(token.kind == VisualTokenKind::PartB && e->to == "ASSEMBLY") {
        node->part_b++;
        model_.AddLog("Assembly", "Part B waiting in collector.", "system");
    }
    else if(token.kind == VisualTokenKind::AssembledUnit && e->to == "QUALITY_CHECK") {
        node->assembled++;
        model_.units_checking++;
        model_.AddLog("Quality Check", "Assembled unit arrived for inspection.", "system");
    }
    else if(token.kind == VisualTokenKind::ReviewUnit && e->to == "QUALITY_REVIEW") {
        node->under_review++;
        model_.units_under_review++;
        model_.AddLog("Quality Review", "Unit queued for review.", "system");
    }
    else if(token.kind == VisualTokenKind::RejectedUnit && e->to == "DISASSEMBLY") {
        node->rejected++;
        model_.rejected_units++;
        model_.AddLog("Disassembly", "Rejected unit received.", "warning");
    }
    else if(token.kind == VisualTokenKind::ShipmentBatch && e->to == "SHIPPING") {
        node->shipping++;
        model_.completed_shipments++;
        model_.AddLog("Shipping", "Shipment batch completed.", "success");
    }
    else if(token.kind == VisualTokenKind::RecycledPartA && e->to == "GEN_A") {
        node->recycled++;
        node->part_a++;
        model_.recycled_units++;
        model_.AddLog("Generator A", "Recycled Part A returned.", "warning");
    }
    else if(token.kind == VisualTokenKind::RecycledPartB && e->to == "GEN_B") {
        node->recycled++;
        node->part_b++;
        model_.recycled_units++;
        model_.AddLog("Generator B", "Recycled Part B returned.", "warning");
    }
}

void VisualizerApp::ProcessCheckResult(bool force_review)
{
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_CHECK")) {
        if(n->assembled <= 0)
            return;
        n->assembled--;
        model_.units_checking = max(0, model_.units_checking - 1);
    }
    bool review = force_review || Random(100) >= review_bias_;
    if(review) {
        SpawnManufacturingToken("check_review_to_quality_review", VisualTokenKind::ReviewUnit, "R", VizAmber(), 1.0);
        model_.AddLog("Quality Check", "Needs review.", "warning");
    }
    else {
        SpawnManufacturingToken("check_pass_to_packaging", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0);
        model_.AddLog("Quality Check", "Passed inspection.", "success");
    }
}

void VisualizerApp::ProcessReviewResult()
{
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_REVIEW")) {
        if(n->under_review <= 0)
            return;
        n->under_review--;
        model_.units_under_review = max(0, model_.units_under_review - 1);
    }
    bool approve = Random(100) < approve_bias_;
    if(approve) {
        SpawnManufacturingToken("review_approve_to_packaging", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0);
        model_.AddLog("Quality Review", "Approved.", "success");
    }
    else {
        SpawnManufacturingToken("review_reject_to_disassembly", VisualTokenKind::RejectedUnit, "X", VizRed(), 1.0);
        model_.AddLog("Quality Review", "Rejected and sent to disassembly.", "alert");
    }
}

void VisualizerApp::TryAssemble()
{
    VisualNodeSpec* n = model_.FindNode("ASSEMBLY");
    if(!n || n->part_a <= 0 || n->part_b <= 0)
        return;
    n->part_a--;
    n->part_b--;
    model_.units_assembled++;
    SpawnManufacturingToken("assembly_to_check", VisualTokenKind::AssembledUnit, "U", VizGreen(), 1.0);
    model_.AddLog("Assembly", "One A + one B formed an assembled unit.", "success");
}

void VisualizerApp::TickProcessing()
{
    VisualNodeSpec* packaging = model_.FindNode("PACKAGING");
    if(packaging && model_.accepted_units_waiting >= 5) {
        model_.accepted_units_waiting -= 5;
        SpawnManufacturingToken("packaging_to_shipping", VisualTokenKind::ShipmentBatch, "5", VizViolet(), 0.95);
        model_.AddLog("Packaging", "Five accepted units became one shipment.", "success");
    }
}

void VisualizerApp::ProduceParts()
{
    if(Random(100) < 48) {
        SpawnPart("gen_a_to_assembly", VisualTokenKind::PartA, "A", VizCyan());
        model_.part_a_generated++;
    }
    if(Random(100) < 48) {
        SpawnPart("gen_b_to_assembly", VisualTokenKind::PartB, "B", VizTeal());
        model_.part_b_generated++;
    }
}

void VisualizerApp::AdvanceScenario()
{
    if(!IsOpen())
        return;

    if(running_) {
        ProduceParts();
        for(int i = model_.tokens.GetCount() - 1; i >= 0; i--) {
            VisualToken token = model_.tokens[i];
            model_.tokens[i].progress += 0.018 * token.speed;
            if(model_.tokens[i].progress < 1.0)
                continue;
            model_.tokens.Remove(i);
            ProcessArrival(token);
            const VisualEdgeSpec* e = model_.FindEdge(token.edge_id);
            if(!e)
                continue;
            if(e->id == "gen_a_to_assembly" || e->id == "gen_b_to_assembly") {
                // arrival already updates waiting counters
            }
            else if(e->id == "assembly_to_check") {
                ProcessCheckResult(force_next_review_);
                force_next_review_ = false;
            }
            else if(e->id == "check_pass_to_packaging") {
                if(VisualNodeSpec* p = model_.FindNode("PACKAGING")) {
                    p->packaging_buffer++;
                    model_.accepted_units_waiting++;
                    model_.completed_units++;
                    model_.AddLog("Packaging", Format("Packaging buffer %d / 5", p->packaging_buffer), "system");
                }
            }
            else if(e->id == "check_review_to_quality_review") {
                ProcessReviewResult();
            }
            else if(e->id == "review_approve_to_packaging") {
                if(VisualNodeSpec* p = model_.FindNode("PACKAGING")) {
                    p->packaging_buffer++;
                    model_.accepted_units_waiting++;
                    model_.completed_units++;
                    model_.AddLog("Packaging", Format("Packaging buffer %d / 5", p->packaging_buffer), "system");
                }
            }
            else if(e->id == "review_reject_to_disassembly") {
                if(VisualNodeSpec* d = model_.FindNode("DISASSEMBLY")) {
                    d->rejected++;
                }
                SpawnManufacturingToken("disassembly_to_gen_a", VisualTokenKind::RecycledPartA, "A", VizRed(), 1.0);
                SpawnManufacturingToken("disassembly_to_gen_b", VisualTokenKind::RecycledPartB, "B", VizRed(), 1.0);
                model_.AddLog("Disassembly", "Rejected unit recycled into A and B.", "warning");
            }
            else if(e->id == "disassembly_to_gen_a" || e->id == "disassembly_to_gen_b") {
                // arrival already updates generator waiting counters
            }
            else if(e->id == "packaging_to_shipping") {
                // arrival handled in ProcessArrival
            }
        }

        TryAssemble();
        TickProcessing();
        UpdateNodeStats();
        SyncGraph();
    }

    tick_.Set(AutoDelayMs(), [=] { AdvanceScenario(); });
}

void VisualizerApp::UpdateNodeStats()
{
    if(VisualNodeSpec* n = model_.FindNode("GEN_A")) {
        n->part_a = model_.part_a_generated;
        n->active = running_ && !model_.tokens.IsEmpty();
    }
    if(VisualNodeSpec* n = model_.FindNode("GEN_B"))
        n->part_b = model_.part_b_generated;
    if(VisualNodeSpec* n = model_.FindNode("ASSEMBLY")) {
        n->assembled = model_.units_assembled;
    }
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_CHECK"))
        n->assembled = model_.units_checking;
    if(VisualNodeSpec* n = model_.FindNode("QUALITY_REVIEW"))
        n->under_review = model_.units_under_review;
    if(VisualNodeSpec* n = model_.FindNode("PACKAGING"))
        n->packaging_buffer = model_.accepted_units_waiting;
    if(VisualNodeSpec* n = model_.FindNode("DISASSEMBLY"))
        n->rejected = model_.rejected_units;
    if(VisualNodeSpec* n = model_.FindNode("SHIPPING"))
        n->shipping = model_.completed_shipments;
}

void VisualizerApp::UpdateMetrics()
{
    status_label_.SetLabel(Format("Status: %s", StatusText()));
    status_label_.SetInk(running_ ? VizGreen() : VizAmber());
    status_label_.SetFont(MonospaceZ(11).Bold());

    counters_label_.SetLabel(Format("A:%d  B:%d  U:%d  R:%d  X:%d  5:%d",
        model_.part_a_generated, model_.part_b_generated, model_.units_assembled,
        model_.units_under_review, model_.rejected_units, model_.completed_shipments));
    counters_label_.SetInk(VizCyan());
    counters_label_.SetFont(MonospaceZ(11).Bold());

    buffer_label_.SetLabel(Format("Packaging: %d / 5   FSM queued events: %d   Last error: %s",
        model_.accepted_units_waiting, model_.fsm_queued_events,
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
    const int controls_h = DPI(52);
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
    reset_btn_.SetRect(x, y, DPI(82), DPI(30)); x += DPI(92);
    speed_slider_.SetRect(x, y + DPI(2), DPI(140), DPI(26)); x += DPI(156);
    review_slider_.SetRect(x, y + DPI(2), DPI(140), DPI(26));

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
    const int controls_h = DPI(52);

    w.DrawRect(sz, VizBg());
    w.DrawRect(0, 0, sz.cx, header_h, VizHeaderBg());
    w.DrawLine(0, header_h - 1, sz.cx, header_h - 1, 1, VizBorder());
    w.DrawRect(0, header_h, sz.cx, controls_h, VizPanelBg());
    w.DrawLine(0, header_h + controls_h - 1, sz.cx, header_h + controls_h - 1, 1, VizBorder());
}

}
