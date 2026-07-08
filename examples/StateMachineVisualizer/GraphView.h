#ifndef _StateMachineVisualizer_GraphView_h_
#define _StateMachineVisualizer_GraphView_h_

/*
    Apache License 2.0

    GraphView
    =========

    Purpose
    - Passive graph surface for the manufacturing-flow visualizer.
*/

#include "StateNodeCard.h"

namespace Upp {

class GraphView : public Ctrl {
public:
    typedef GraphView CLASSNAME;

    GraphView();

    void SetModel(VisualizerModel* model);
    void RebuildNodeCards();
    void SyncNodeCards();
    void AddToken(const String& edge_id, VisualTokenKind kind, const String& short_label, Color c, double speed = 1.0);

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

private:
    struct EdgePath {
        Rect from;
        Rect to;
        Pointf p0;
        Pointf p1;
        Pointf p2;
        Pointf p3;
    };

    Rect GetNodeRect(const VisualNodeSpec& n) const;
    Rect GetNodeRectById(const String& id) const;
    Pointf PortPoint(const Rect& r, EdgePort port) const;
    Pointf CubicPoint(Pointf p0, Pointf p1, Pointf p2, Pointf p3, double t) const;
    void DrawBackground(Draw& w);
    void DrawEdge(Draw& w, const VisualEdgeSpec& e);
    void DrawToken(Draw& w, const VisualToken& t);
    EdgePath MakePath(const VisualEdgeSpec& e) const;
    void DrawArrowhead(Draw& w, const EdgePath& path, Color c) const;
    bool EdgeIsActive(const String& id) const;
    Color EdgeColor(const VisualEdgeSpec& e) const;

private:
    VisualizerModel* model_ = nullptr;
    Array<StateNodeCard> cards_;
};

}

#endif
