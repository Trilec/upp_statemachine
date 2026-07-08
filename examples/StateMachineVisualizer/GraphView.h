/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    GraphView
    =========

    Purpose
    - Graph surface for the optional animated StateMachine visualizer.

    Intent
    - Place StateNodeCard controls on a stable row/column grid.
    - Draw subtle grid lines and curved edges behind the node controls.
    - Animate lightweight tokens along edges so state-machine activity is
      visible without turning every graph element into a child control.

    Thread context
    - GUI thread only.
*/

#ifndef _StateMachineVisualizer_GraphView_h_
#define _StateMachineVisualizer_GraphView_h_

#include "StateNodeCard.h"

namespace Upp {

class GraphView : public Ctrl {
public:
    typedef GraphView CLASSNAME;

    GraphView();

    void SetModel(VisualizerModel* model);
    void RebuildNodeCards();
    void SyncNodeCards();
    void AddToken(const String& from, const String& to, Color c, bool interrupt = false, bool batch = false, bool reverse = false);

    virtual void Paint(Draw& w) override;
    virtual void Layout() override;

private:
    void Tick();
    Rect GetNodeRect(const VisualNodeSpec& n) const;
    Rect GetNodeRectById(const String& id) const;
    Point EdgeAnchor(const Rect& r, const Rect& other) const;
    Pointf CubicPoint(Pointf p0, Pointf p1, Pointf p2, Pointf p3, double t) const;
    void DrawBackground(Draw& w);
    void DrawEdge(Draw& w, const VisualEdgeSpec& e);
    void DrawToken(Draw& w, const VisualToken& t);

private:
    VisualizerModel* model_ = nullptr;
    Array<StateNodeCard> cards_;
    bool timer_running_ = false;
};

}

#endif
