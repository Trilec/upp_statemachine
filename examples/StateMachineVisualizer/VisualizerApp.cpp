#include "VisualizerApp.h"

namespace Upp {

static Color Bg()        { return Color(7, 11, 19); }
static Color PanelBg()   { return Color(9, 13, 22); }
static Color HeaderBg()  { return Color(13, 19, 36); }
static Color Border()    { return Color(30, 41, 59); }
static Color Ink()       { return Color(226, 232, 240); }
static Color MutedInk()  { return Color(148, 163, 184); }
static Color Cyan()      { return Color(56, 189, 248); }
static Color Emerald()   { return Color(16, 185, 129); }
static Color Amber()     { return Color(245, 158, 11); }

void VisualLogPanel::Paint(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(4, 8, 15));
    w.DrawRect(0, 0, sz.cx, DPI(24), PanelBg());
    w.DrawText(DPI(12), DPI(6), "State Engine Transition Log", SansSerifZ(10).Bold(), MutedInk());
    w.DrawLine(0, DPI(24), sz.cx, DPI(24), 1, Border());

    if(!model_)
        return;

    int y = DPI(32);
    int first = max(0, model_->log.GetCount() - 8);
    for(int i = first; i < model_->log.GetCount(); i++) {
        const VisualLogEntry& e = model_->log[i];
        Color c = Ink();
        if(e.kind == "success") c = Emerald();
        else if(e.kind == "warning") c = Amber();
        else if(e.kind == "system") c = Cyan();

        w.DrawText(DPI(12), y, "[" + e.source + "]", MonospaceZ(10).Bold(), MutedInk());
        w.DrawText(DPI(110), y, e.message, MonospaceZ(10), c);
        y += DPI(16);
    }
}

void GuidePanel::Paint(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(11, 17, 31));
    w.DrawRect(0, 0, sz.cx, DPI(24), PanelBg());
    w.DrawText(DPI(12), DPI(6), "Visualizer Guide", SansSerifZ(10).Bold(), MutedInk());
    w.DrawLine(0, DPI(24), sz.cx, DPI(24), 1, Border());

    int x = DPI(14);
    int y = DPI(38);
    w.DrawText(x, y, "1. UiTitleCard nodes", SansSerifZ(11).Bold(), Emerald()); y += DPI(18);
    w.DrawText(x, y, "States are real themed controls placed on a grid.", SansSerifZ(10), MutedInk()); y += DPI(24);
    w.DrawText(x, y, "2. GraphView edges", SansSerifZ(11).Bold(), Cyan()); y += DPI(18);
    w.DrawText(x, y, "Curved lines and moving tokens are custom-painted.", SansSerifZ(10), MutedInk()); y += DPI(24);
    w.DrawText(x, y, "3. StateMachine core", SansSerifZ(11).Bold(), Amber()); y += DPI(18);
    w.DrawText(x, y, "Buttons call the real StateMachine API.", SansSerifZ(10), MutedInk());
}

VisualizerApp::VisualizerApp()
{
    Title("StateMachine Visualizer").Sizeable().Zoomable();
    SetRect(0, 0, DPI(1180), DPI(760));

    model_.ResetDemoGraph();
    BuildMachine();

    Add(title_label_);
    Add(subtitle_label_);
    Add(active_label_);
    Add(processed_label_);
    Add(speed_label_);
    Add(queue_label_);

    Add(start_btn_);
    Add(trigger_btn_);
    Add(queue_btn_);
    Add(error_btn_);
    Add(reset_btn_);
    Add(speed_slider_);
    Add(queue_slider_);
    Add(graph_);
    Add(log_);
    Add(guide_);

    title_label_.SetLabel("State Machine Resiliency Visualizer");
    title_label_.SetInk(White());
    title_label_.SetFont(SansSerifZ(16).Bold());
    subtitle_label_.SetLabel("UiTitleCard nodes + animated graph edges + real StateMachine core");
    subtitle_label_.SetInk(MutedInk());
    subtitle_label_.SetFont(SansSerifZ(11));

    start_btn_.SetText("Start");
    trigger_btn_.SetText("Trigger Event");
    queue_btn_.SetText("Queue Example");
    error_btn_.SetText("Error Path");
    reset_btn_.SetText("Reset");

    speed_slider_.SetRange(0.25, 3.0).SetStep(0.25).SetValue(1.0).SetSizeMin(DPI(120), DPI(24));
    queue_slider_.SetRange(1, 16).SetStep(1).SetValue(8).SetSizeMin(DPI(120), DPI(24));

    start_btn_.WhenAction = THISBACK(ResetScenario);
    trigger_btn_.WhenAction = THISBACK(TriggerNext);
    queue_btn_.WhenAction = THISBACK(TriggerQueueExample);
    error_btn_.WhenAction = THISBACK(TriggerErrorExample);
    reset_btn_.WhenAction = THISBACK(ResetScenario);

    speed_slider_.WhenAction = [=] {
        model_.AddLog("Config", Format("Speed slider scaffold set to %.2f", speed_slider_.GetValue()), "system");
        log_.Refresh();
    };
    queue_slider_.WhenAction = [=] {
        machine_.SetMaxQueuedEvents((int)queue_slider_.GetValue());
        model_.AddLog("Config", Format("Max queued events set to %d", (int)queue_slider_.GetValue()), "system");
        SyncGraph();
    };

    graph_.SetModel(&model_);
    log_.SetModel(&model_);
    UpdateMetrics();
}

void VisualizerApp::BuildMachine()
{
    machine_.Clear();
    machine_.AddState({"IDLE", {}, {}});
    machine_.AddState({"ACTIVE", {}, {}});
    machine_.AddState({"QUEUED", {}, {}});
    machine_.AddState({"ERROR", {}, {}});
    machine_.AddState({"DONE", {}, {}});
    machine_.SetInitial("IDLE");
    machine_.AddTransition({"activate", "IDLE", "ACTIVE"});
    machine_.AddTransition({"finish",   "ACTIVE", "DONE"});
    machine_.AddTransition({"queue",    "ACTIVE", "QUEUED"});
    machine_.AddTransition({"drain",    "QUEUED", "DONE"});
    machine_.AddTransition({"fail",     "ACTIVE", "ERROR"});
    machine_.SetEventPolicy(EventPolicy::QueueWhileTransitioning);
    machine_.SetMaxQueuedEvents(8);
}

void VisualizerApp::ResetScenario()
{
    model_.ResetDemoGraph();
    BuildMachine();
    if(machine_.Start()) {
        model_.SetActive("IDLE");
        model_.AddLog("Start", "Machine started at IDLE.", "success");
    }
    else {
        model_.AddLog("Start", machine_.GetLastErrorText(), "warning");
    }
    SyncGraph();
}

String VisualizerApp::CurrentVisualNode() const
{
    String s = machine_.GetCurrent();
    return s.IsEmpty() ? "IDLE" : s;
}

void VisualizerApp::TriggerNext()
{
    if(!machine_.IsStarted()) {
        ResetScenario();
        return;
    }

    String from = CurrentVisualNode();
    String event;
    if(from == "IDLE") event = "activate";
    else if(from == "ACTIVE") event = "finish";
    else if(from == "QUEUED") event = "drain";
    else event = "activate";

    if(machine_.TriggerEvent(event)) {
        String to = CurrentVisualNode();
        model_.SetActive(to);
        model_.AddToken(from, to, Cyan());
        model_.AddLog("TriggerEvent", Format("%s -> %s via %s", from, to, event), "success");
    }
    else {
        model_.AddLog("TriggerEvent", machine_.GetLastErrorText(), "warning");
    }
    SyncGraph();
}

void VisualizerApp::TriggerQueueExample()
{
    if(!machine_.IsStarted())
        ResetScenario();

    String from = CurrentVisualNode();
    if(from != "ACTIVE") {
        machine_.TriggerEvent("activate");
        from = "IDLE";
        model_.AddToken("IDLE", "ACTIVE", Cyan());
    }

    if(machine_.TriggerEvent("queue")) {
        String to = CurrentVisualNode();
        model_.SetActive(to);
        if(VisualNodeSpec* n = model_.FindNode("QUEUED"))
            n->queued_count = machine_.GetQueuedEventCount();
        model_.AddToken("ACTIVE", "QUEUED", Color(56, 189, 248));
        model_.AddLog("Queue", "Queue scenario triggered. Gary should wire this to async queue cases next.", "system");
    }
    else {
        model_.AddLog("Queue", machine_.GetLastErrorText(), "warning");
    }
    SyncGraph();
}

void VisualizerApp::TriggerErrorExample()
{
    if(!machine_.IsStarted())
        ResetScenario();

    if(CurrentVisualNode() == "IDLE")
        machine_.TriggerEvent("activate");

    String from = CurrentVisualNode();
    if(machine_.TriggerEvent("fail")) {
        String to = CurrentVisualNode();
        model_.SetActive(to);
        model_.MarkError(to, true);
        model_.AddToken(from, to, Amber(), true);
        model_.AddLog("Failure", "Visual error route triggered.", "warning");
    }
    else {
        model_.AddLog("Failure", machine_.GetLastErrorText(), "warning");
    }
    SyncGraph();
}

void VisualizerApp::UpdateMetrics()
{
    active_label_.SetLabel(Format("Active Tasks: %d", model_.tokens.GetCount()));
    active_label_.SetInk(Cyan());
    active_label_.SetFont(MonospaceZ(11).Bold());

    processed_label_.SetLabel(Format("Processed: %d", model_.processed_count));
    processed_label_.SetInk(Emerald());
    processed_label_.SetFont(MonospaceZ(11).Bold());

    speed_label_.SetLabel("Speed");
    speed_label_.SetInk(MutedInk());
    queue_label_.SetLabel("Queue Max");
    queue_label_.SetInk(MutedInk());
}

void VisualizerApp::SyncGraph()
{
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
    const int footer_h = DPI(160);
    const int pad = DPI(12);

    title_label_.SetRect(DPI(22), DPI(10), DPI(430), DPI(24));
    subtitle_label_.SetRect(DPI(22), DPI(34), DPI(600), DPI(20));
    active_label_.SetRect(sz.cx - DPI(320), DPI(14), DPI(140), DPI(22));
    processed_label_.SetRect(sz.cx - DPI(170), DPI(14), DPI(140), DPI(22));

    int y = header_h + DPI(10);
    int x = DPI(18);
    start_btn_.SetRect(x, y, DPI(86), DPI(30)); x += DPI(94);
    trigger_btn_.SetRect(x, y, DPI(128), DPI(30)); x += DPI(136);
    queue_btn_.SetRect(x, y, DPI(128), DPI(30)); x += DPI(136);
    error_btn_.SetRect(x, y, DPI(110), DPI(30)); x += DPI(118);
    reset_btn_.SetRect(x, y, DPI(86), DPI(30)); x += DPI(112);

    speed_label_.SetRect(x, y + DPI(7), DPI(55), DPI(20)); x += DPI(55);
    speed_slider_.SetRect(x, y + DPI(2), DPI(120), DPI(26)); x += DPI(140);
    queue_label_.SetRect(x, y + DPI(7), DPI(75), DPI(20)); x += DPI(75);
    queue_slider_.SetRect(x, y + DPI(2), DPI(120), DPI(26));

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

    w.DrawRect(sz, Bg());
    w.DrawRect(0, 0, sz.cx, header_h, HeaderBg());
    w.DrawLine(0, header_h - 1, sz.cx, header_h - 1, 1, Border());

    w.DrawRect(0, header_h, sz.cx, controls_h, PanelBg());
    w.DrawLine(0, header_h + controls_h - 1, sz.cx, header_h + controls_h - 1, 1, Border());
}

}
