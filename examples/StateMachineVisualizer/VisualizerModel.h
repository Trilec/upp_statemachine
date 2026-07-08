#ifndef _StateMachineVisualizer_VisualizerModel_h_
#define _StateMachineVisualizer_VisualizerModel_h_

/*
    Apache License 2.0

    StateMachineVisualizer manufacturing model
    ==========================================

    Purpose
    - Lightweight view-model data for the manufacturing-flow visualizer.

    Intent
    - Keep layout, edge geometry, tokens, counters, and logs separate from the
      reusable StateMachine core.
    - Expose stable identifiers so the graph can animate explicit routes.
*/

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

enum class VisualTokenKind {
    PartA,
    PartB,
    AssembledUnit,
    ReviewUnit,
    RejectedUnit,
    ShipmentBatch,
    RecycledPartA,
    RecycledPartB
};

struct VisualNodeSpec : Moveable<VisualNodeSpec> {
    String id;
    String title;
    String subtitle;
    String copy;
    int row = 0;
    int col = 0;
    int part_a = 0;
    int part_b = 0;
    int assembled = 0;
    int under_review = 0;
    int rejected = 0;
    int recycled = 0;
    int packaging_buffer = 0;
    int shipping = 0;
    bool active = false;
};

struct VisualEdgeSpec : Moveable<VisualEdgeSpec> {
    String id;
    String from;
    String to;
    String label;
    Color color = Color(56, 189, 248);
    bool dashed = false;
    double curve_bias = 0.45;
};

struct VisualToken : Moveable<VisualToken> {
    String id;
    String edge_id;
    VisualTokenKind kind = VisualTokenKind::PartA;
    String short_label;
    double progress = 0.0;
    double speed = 1.0;
    Color color = Color(56, 189, 248);
};

struct VisualLogEntry : Moveable<VisualLogEntry> {
    String source;
    String message;
    String kind;
};

struct VisualizerModel {
    Vector<VisualNodeSpec> nodes;
    Vector<VisualEdgeSpec> edges;
    Vector<VisualToken> tokens;
    Vector<VisualLogEntry> log;
    int token_counter = 0;
    int part_a_generated = 0;
    int part_b_generated = 0;
    int units_assembled = 0;
    int units_checking = 0;
    int units_under_review = 0;
    int rejected_units = 0;
    int recycled_units = 0;
    int completed_shipments = 0;
    int completed_units = 0;
    int accepted_units_waiting = 0;
    int fsm_queued_events = 0;
    String last_fsm_error;

    void ResetManufacturingGraph()
    {
        nodes.Clear();
        edges.Clear();
        tokens.Clear();
        log.Clear();
        token_counter = 0;
        part_a_generated = 0;
        part_b_generated = 0;
        units_assembled = 0;
        units_checking = 0;
        units_under_review = 0;
        rejected_units = 0;
        recycled_units = 0;
        completed_shipments = 0;
        completed_units = 0;
        accepted_units_waiting = 0;
        fsm_queued_events = 0;
        last_fsm_error.Clear();

        AddNode("GEN_A",          "Generator A",        "Part source A",       "Injects A parts",      0, 0);
        AddNode("ASSEMBLY",       "Assembly Collector",  "Queues both parts",   "Waits for one A + B",  0, 1);
        AddNode("QUALITY_CHECK",  "Quality Check",       "Inspection gate",     "Pass or review",       0, 2);
        AddNode("PACKAGING",      "Packaging Buffer",    "Accepted units",      "N / 5 shipment batch", 0, 4);
        AddNode("SHIPPING",       "Shipping",            "Completed units",     "Shipment leaves here", 0, 5);
        AddNode("GEN_B",          "Generator B",         "Part source B",       "Injects B parts",      2, 0);
        AddNode("DISASSEMBLY",    "Disassembly",         "Recycle / split",     "Rejected units split", 2, 1);
        AddNode("QUALITY_REVIEW", "Quality Review",      "Manual review",       "Approve or reject",    2, 3);

        AddEdge("gen_a_to_assembly",        "GEN_A",         "ASSEMBLY",       "Part A",       Color(56, 189, 248), false, 0.35);
        AddEdge("gen_b_to_assembly",        "GEN_B",         "ASSEMBLY",       "Part B",       Color(45, 212, 191), false, 0.35);
        AddEdge("assembly_to_check",        "ASSEMBLY",      "QUALITY_CHECK",  "Assembled unit", Color(34, 197, 94), false, 0.45);
        AddEdge("check_pass_to_packaging",  "QUALITY_CHECK", "PACKAGING",      "Passed",       Color(16, 185, 129), false, 0.45);
        AddEdge("check_review_to_quality_review", "QUALITY_CHECK", "QUALITY_REVIEW", "Needs review", Color(245, 158, 11), false, 0.45);
        AddEdge("review_approve_to_packaging", "QUALITY_REVIEW", "PACKAGING",   "Approved",     Color(16, 185, 129), false, 0.42);
        AddEdge("review_reject_to_disassembly", "QUALITY_REVIEW", "DISASSEMBLY", "Rejected",     Color(239, 68, 68), false, 0.42);
        AddEdge("disassembly_to_gen_a",     "DISASSEMBLY",   "GEN_A",          "Recycle A",    Color(239, 68, 68), true, 0.25);
        AddEdge("disassembly_to_gen_b",     "DISASSEMBLY",   "GEN_B",          "Recycle B",    Color(239, 68, 68), true, 0.25);
        AddEdge("packaging_to_shipping",     "PACKAGING",     "SHIPPING",       "Batch of 5",   Color(124, 58, 237), false, 0.50);

        SetActive("GEN_A");
        AddLog("System", "Manufacturing flow initialized.", "system");
    }

    VisualNodeSpec* FindNode(const String& id)
    {
        for(int i = 0; i < nodes.GetCount(); i++)
            if(nodes[i].id == id)
                return &nodes[i];
        return nullptr;
    }

    const VisualNodeSpec* FindNode(const String& id) const
    {
        for(int i = 0; i < nodes.GetCount(); i++)
            if(nodes[i].id == id)
                return &nodes[i];
        return nullptr;
    }

    VisualEdgeSpec* FindEdge(const String& id)
    {
        for(int i = 0; i < edges.GetCount(); i++)
            if(edges[i].id == id)
                return &edges[i];
        return nullptr;
    }

    const VisualEdgeSpec* FindEdge(const String& id) const
    {
        for(int i = 0; i < edges.GetCount(); i++)
            if(edges[i].id == id)
                return &edges[i];
        return nullptr;
    }

    void SetActive(const String& id)
    {
        for(int i = 0; i < nodes.GetCount(); i++)
            nodes[i].active = nodes[i].id == id;
    }

    void AddToken(const String& edge_id, VisualTokenKind kind, const String& short_label, Color c, double speed = 1.0)
    {
        VisualToken t;
        t.id = Format("tok-%d", ++token_counter);
        t.edge_id = edge_id;
        t.kind = kind;
        t.short_label = short_label;
        t.color = c;
        t.speed = speed;
        tokens.Add(t);
    }

    void AddLog(const String& source, const String& message, const String& kind = "info")
    {
        VisualLogEntry e;
        e.source = source;
        e.message = message;
        e.kind = kind;
        log.Add(e);
        while(log.GetCount() > 120)
            log.Remove(0);
    }

    static String KindText(VisualTokenKind kind)
    {
        switch(kind) {
        case VisualTokenKind::PartA: return "A";
        case VisualTokenKind::PartB: return "B";
        case VisualTokenKind::AssembledUnit: return "U";
        case VisualTokenKind::ReviewUnit: return "R";
        case VisualTokenKind::RejectedUnit: return "X";
        case VisualTokenKind::ShipmentBatch: return "5";
        case VisualTokenKind::RecycledPartA: return "A";
        case VisualTokenKind::RecycledPartB: return "B";
        }
        return "?";
    }

private:
    void AddNode(const String& id, const String& title, const String& subtitle,
                 const String& copy, int row, int col)
    {
        VisualNodeSpec n;
        n.id = id;
        n.title = title;
        n.subtitle = subtitle;
        n.copy = copy;
        n.row = row;
        n.col = col;
        nodes.Add(n);
    }

    void AddEdge(const String& id, const String& from, const String& to, const String& label,
                 Color color, bool dashed, double curve_bias)
    {
        VisualEdgeSpec e;
        e.id = id;
        e.from = from;
        e.to = to;
        e.label = label;
        e.color = color;
        e.dashed = dashed;
        e.curve_bias = curve_bias;
        edges.Add(e);
    }
};

}

#endif
