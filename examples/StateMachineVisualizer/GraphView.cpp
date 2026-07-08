#include "GraphView.h"

namespace Upp {

GraphView::GraphView()
{
    BackPaint();
}

void GraphView::SetModel(VisualizerModel* model)
{
    model_ = model;
    RebuildNodeCards();
    if(!timer_running_) {
        timer_running_ = true;
        SetTimeCallback(16, THISBACK(Tick));
    }
}

void GraphView::RebuildNodeCards()
{
    for(int i = 0; i < cards_.GetCount(); i++)
        cards_[i].Remove();
    cards_.Clear();

    if(!model_)
        return;

    for(int i = 0; i < model_->nodes.GetCount(); i++) {
        StateNodeCard& card = cards_.Add();
        card.SetNode(model_->nodes[i]);
        Add(card);
    }
    Layout();
    Refresh();
}

void GraphView::SyncNodeCards()
{
    if(!model_)
        return;
    int n = min(cards_.GetCount(), model_->nodes.GetCount());
    for(int i = 0; i < n; i++)
        cards_[i].SetNode(model_->nodes[i]);
    Refresh();
}

void GraphView::AddToken(const String& edge_id, VisualTokenKind kind, const String& short_label, Color c, double speed)
{
    if(model_)
        model_->AddToken(edge_id, kind, short_label, c, speed);
    Refresh();
}

void GraphView::Layout()
{
    if(!model_)
        return;
    int n = min(cards_.GetCount(), model_->nodes.GetCount());
    for(int i = 0; i < n; i++)
        cards_[i].SetRect(GetNodeRect(model_->nodes[i]));
}

Rect GraphView::GetNodeRect(const VisualNodeSpec& n) const
{
    const int node_w = DPI(172);
    const int node_h = DPI(90);
    const int cell_w = DPI(206);
    const int cell_h = DPI(130);
    const int margin_x = DPI(36);
    const int margin_y = DPI(40);
    return RectC(margin_x + n.col * cell_w, margin_y + n.row * cell_h, node_w, node_h);
}

Rect GraphView::GetNodeRectById(const String& id) const
{
    if(!model_)
        return Rect();
    const VisualNodeSpec* n = model_->FindNode(id);
    return n ? GetNodeRect(*n) : Rect();
}

Point GraphView::EdgeAnchor(const Rect& r, const Rect& other) const
{
    Point c = r.CenterPoint();
    Point o = other.CenterPoint();
    if(o.x > c.x) return Point(r.right, c.y);
    if(o.x < c.x) return Point(r.left, c.y);
    if(o.y > c.y) return Point(c.x, r.bottom);
    return Point(c.x, r.top);
}

Pointf GraphView::CubicPoint(Pointf p0, Pointf p1, Pointf p2, Pointf p3, double t) const
{
    double u = 1.0 - t;
    double b0 = u * u * u;
    double b1 = 3.0 * u * u * t;
    double b2 = 3.0 * u * t * t;
    double b3 = t * t * t;
    return Pointf(p0.x * b0 + p1.x * b1 + p2.x * b2 + p3.x * b3,
                  p0.y * b0 + p1.y * b1 + p2.y * b2 + p3.y * b3);
}

GraphView::EdgePath GraphView::MakePath(const VisualEdgeSpec& e) const
{
    EdgePath path;
    path.from = GetNodeRectById(e.from);
    path.to = GetNodeRectById(e.to);
    path.start = EdgeAnchor(path.from, path.to);
    path.end = EdgeAnchor(path.to, path.from);
    path.p0 = Pointf(path.start.x, path.start.y);
    path.p3 = Pointf(path.end.x, path.end.y);
    double dx = abs(path.end.x - path.start.x);
    double bend = max(70.0, dx * e.curve_bias);
    if(path.start.y == path.end.y)
        bend = max(bend, (double)DPI(120));

    if(path.end.x >= path.start.x) {
        path.p1 = Pointf(path.start.x + bend, path.start.y);
        path.p2 = Pointf(path.end.x - bend, path.end.y);
    }
    else {
        path.p1 = Pointf(path.start.x - bend, path.start.y - bend * 0.35);
        path.p2 = Pointf(path.end.x + bend, path.end.y - bend * 0.25);
    }
    return path;
}

void GraphView::DrawArrowhead(Draw& w, const EdgePath& path, Color c) const
{
    Pointf p = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.82);
    Pointf q = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.88);
    Pointf r = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.94);
    w.DrawLine((int)p.x, (int)p.y, (int)q.x, (int)q.y, 2, c);
    w.DrawLine((int)q.x, (int)q.y, (int)r.x, (int)r.y, 2, c);
}

void GraphView::DrawBackground(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(7, 11, 19));

    Color grid = Color(24, 35, 54);
    for(int x = 0; x < sz.cx; x += DPI(24))
        w.DrawLine(x, 0, x, sz.cy, 1, grid);
    for(int y = 0; y < sz.cy; y += DPI(24))
        w.DrawLine(0, y, sz.cx, y, 1, grid);
}

void GraphView::DrawEdge(Draw& w, const VisualEdgeSpec& e)
{
    EdgePath path = MakePath(e);
    if(path.from.IsEmpty() || path.to.IsEmpty())
        return;

    Color c = e.color;
    if(e.dashed)
        c = Blend(c, White(), 60);

    Point last = path.start;
    for(int i = 1; i <= 32; i++) {
        double t = i / 32.0;
        Pointf p = CubicPoint(path.p0, path.p1, path.p2, path.p3, t);
        Point now((int)p.x, (int)p.y);
        if(!e.dashed || (i % 2))
            w.DrawLine(last.x, last.y, now.x, now.y, 2, c);
        last = now;
    }

    DrawArrowhead(w, path, c);
    Pointf mid = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.55);
    w.DrawEllipse(RectC((int)mid.x - DPI(2), (int)mid.y - DPI(2), DPI(4), DPI(4)), c);
    w.DrawText((int)mid.x + DPI(4), (int)mid.y - DPI(12), e.label, SansSerifZ(8).Bold(), c);
}

void GraphView::DrawToken(Draw& w, const VisualToken& t)
{
    if(!model_)
        return;
    const VisualEdgeSpec* e = model_->FindEdge(t.edge_id);
    if(!e)
        return;

    EdgePath path = MakePath(*e);
    if(path.from.IsEmpty() || path.to.IsEmpty())
        return;

    double tt = min(1.0, max(0.0, t.progress));
    Pointf p = CubicPoint(path.p0, path.p1, path.p2, path.p3, tt);
    int r = t.kind == VisualTokenKind::ShipmentBatch ? DPI(9) : DPI(7);
    if(t.kind == VisualTokenKind::PartA || t.kind == VisualTokenKind::PartB)
        r = DPI(6);

    Color body = t.color;
    Rect box((int)p.x - r, (int)p.y - r, (int)p.x + r, (int)p.y + r);
    w.DrawEllipse(box, body);
    Rect inner(box.left + DPI(2), box.top + DPI(2), box.right - DPI(2), box.bottom - DPI(2));
    w.DrawEllipse(inner, Blend(body, White(), 35));
    w.DrawText(box.left + DPI(1), box.top + DPI(1), t.short_label, SansSerifZ(8).Bold(), White());
}

void GraphView::Paint(Draw& w)
{
    DrawBackground(w);
    if(!model_)
        return;

    for(int i = 0; i < model_->edges.GetCount(); i++)
        DrawEdge(w, model_->edges[i]);
    for(int i = 0; i < model_->tokens.GetCount(); i++)
        DrawToken(w, model_->tokens[i]);
}

void GraphView::Tick()
{
    if(model_) {
        for(int i = model_->tokens.GetCount() - 1; i >= 0; i--) {
            VisualToken& t = model_->tokens[i];
            t.progress += 0.018 * t.speed;
            if(t.progress >= 1.0)
                model_->tokens.Remove(i);
        }
        SyncNodeCards();
    }
    SetTimeCallback(16, THISBACK(Tick));
}

}
