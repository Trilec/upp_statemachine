/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    StateMachineVisualizer model
    ============================

    Purpose
    - Lightweight view-model data for the optional animated StateMachine
      visualizer example.

    Intent
    - Keep visual state separate from the StateMachine core.
    - Store grid-positioned node metadata, edge metadata, animated tokens, and
      log rows used by the GUI harness.
    - Keep this example readable and easy for Gary to extend after the first
      compile pass.

    Thread context
    - GUI thread only.

    Usage
    - VisualizerApp owns one VisualizerModel instance.
    - GraphView reads the model and paints/places the visible graph.
*/

#ifndef _StateMachineVisualizer_VisualizerModel_h_
#define _StateMachineVisualizer_VisualizerModel_h_

#include <Core/Core.h>
#include <Draw/Draw.h>

namespace Upp {

struct VisualNodeSpec : Moveable<VisualNodeSpec> {
    String id;
    String title;
    String subtitle;
    String copy;
    int row = 0;
    int col = 0;
    int active_count = 0;
    int queued_count = 0;
    int processed_count = 0;
    bool active = false;
    bool error = false;
    bool complete = false;
};

struct VisualEdgeSpec : Moveable<VisualEdgeSpec> {
    String id;
    String from;
    String to;
    bool interrupt = false;
    bool active = false;
};

struct VisualToken : Moveable<VisualToken> {
    String id;
    String from;
    String to;
    double progress = 0.0;
    Color color = Color(14, 165, 233);
    bool interrupt = false;
    bool batch = false;
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
    int processed_count = 0;

    void ResetDemoGraph()
    {
        nodes.Clear();
        edges.Clear();
        tokens.Clear();
        log.Clear();
        token_counter = 0;
        processed_count = 0;

        AddNode("IDLE",     "Idle",      "Initial state",      "Ready for Start()",              1, 0);
        AddNode("ACTIVE",   "Active",    "Async transition",   "Events may reject/drop/queue",   1, 1);
        AddNode("QUEUED",   "Queued",    "Bounded FIFO",       "TriggerEvent() names only",      0, 2);
        AddNode("ERROR",    "Error",     "Rollback path",      "Last error stays inspectable",   2, 2);
        AddNode("DONE",     "Completed", "Terminal state",     "History committed",             1, 3);

        AddEdge("idle_active", "IDLE",   "ACTIVE");
        AddEdge("active_done", "ACTIVE", "DONE");
        AddEdge("active_queue","ACTIVE", "QUEUED");
        AddEdge("queue_done",  "QUEUED", "DONE");
        AddEdge("active_error","ACTIVE", "ERROR", true);

        SetActive("IDLE");
        AddLog("System", "StateMachineVisualizer scaffold initialized.", "system");
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

    void SetActive(const String& id)
    {
        for(int i = 0; i < nodes.GetCount(); i++) {
            nodes[i].active = nodes[i].id == id;
            nodes[i].active_count = nodes[i].active ? 1 : 0;
        }
    }

    void MarkError(const String& id, bool on = true)
    {
        if(VisualNodeSpec* n = FindNode(id))
            n->error = on;
    }

    void MarkComplete(const String& id, bool on = true)
    {
        if(VisualNodeSpec* n = FindNode(id))
            n->complete = on;
    }

    void AddToken(const String& from, const String& to, Color c, bool interrupt = false, bool batch = false)
    {
        VisualToken t;
        t.id = Format("tok-%d", ++token_counter);
        t.from = from;
        t.to = to;
        t.color = c;
        t.interrupt = interrupt;
        t.batch = batch;
        tokens.Add(t);
    }

    void AddLog(const String& source, const String& message, const String& kind = "info")
    {
        VisualLogEntry e;
        e.source = source;
        e.message = message;
        e.kind = kind;
        log.Add(e);
        while(log.GetCount() > 80)
            log.Remove(0);
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

    void AddEdge(const String& id, const String& from, const String& to, bool interrupt = false)
    {
        VisualEdgeSpec e;
        e.id = id;
        e.from = from;
        e.to = to;
        e.interrupt = interrupt;
        edges.Add(e);
    }
};

}

#endif
