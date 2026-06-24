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

void GraphView::AddToken(const String& from, const String& to, Color c, bool interrupt, bool batch)
{
    if(model_)
        model_->AddToken(from, to, c, interrupt, batch);
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
    const int node_w = DPI(168);
    const int node_h = DPI(86);
    const int cell_w = DPI(220);
    const int cell_h = DPI(130);
    const int margin_x = DPI(42);
    const int margin_y = DPI(42);

    int x = margin_x + n.col * cell_w;
    int y = margin_y + n.row * cell_h;
    return RectC(x, y, node_w, node_h);
}

Rect GraphView::GetNodeRectById(const String& id) const
{
    if(!model_)
        return Rect(0, 0, 0, 0);
    for(int i = 0; i < model_->nodes.GetCount(); i++)
        if(model_->nodes[i].id == id)
            return GetNodeRect(model_->nodes[i]);
    return Rect(0, 0, 0, 0);
}

Point GraphView::EdgeAnchor(const Rect& r, const Rect& other) const
{
    Point c = r.CenterPoint();
    Point o = other.CenterPoint();
    if(o.x > c.x)
        return Point(r.right, c.y);
    if(o.x < c.x)
        return Point(r.left, c.y);
    if(o.y > c.y)
        return Point(c.x, r.bottom);
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

void GraphView::DrawBackground(Draw& w)
{
    Size sz = GetSize();
    w.DrawRect(sz, Color(7, 11, 19));

    Color grid = Color(24, 35, 54);
    const int step = DPI(24);
    for(int x = 0; x < sz.cx; x += step)
        w.DrawLine(x, 0, x, sz.cy, 1, grid);
    for(int y = 0; y < sz.cy; y += step)
        w.DrawLine(0, y, sz.cx, y, 1, grid);

    // A few stronger guide lines echo the row/column placement without turning
    // the canvas into a noisy engineering grid.
    Color guide = Color(35, 50, 74);
    for(int x = DPI(42); x < sz.cx; x += DPI(220))
        w.DrawLine(x, 0, x, sz.cy, 1, guide);
    for(int y = DPI(42); y < sz.cy; y += DPI(130))
        w.DrawLine(0, y, sz.cx, y, 1, guide);
}

void GraphView::DrawEdge(Draw& w, const VisualEdgeSpec& e)
{
    Rect fr = GetNodeRectById(e.from);
    Rect tr = GetNodeRectById(e.to);
    if(fr.IsEmpty() || tr.IsEmpty())
        return;

    Point start = EdgeAnchor(fr, tr);
    Point end = EdgeAnchor(tr, fr);
    Pointf p0(start.x, start.y);
    Pointf p3(end.x, end.y);
    double dx = abs(end.x - start.x);
    Pointf p1(start.x + max(60.0, dx * 0.45), start.y);
    Pointf p2(end.x - max(60.0, dx * 0.45), end.y);

    Color c = e.interrupt ? Color(245, 158, 11) : Color(56, 189, 248);
    if(!e.active)
        c = e.interrupt ? Color(92, 61, 22) : Color(45, 61, 84);

    Point last = start;
    for(int i = 1; i <= 28; i++) {
        double t = i / 28.0;
        Pointf p = CubicPoint(p0, p1, p2, p3, t);
        Point now((int)p.x, (int)p.y);
        w.DrawLine(last.x, last.y, now.x, now.y, e.active ? 3 : 2, c);
        last = now;
    }

    Pointf mid = CubicPoint(p0, p1, p2, p3, 0.55);
    w.DrawEllipse(RectC((int)mid.x - DPI(3), (int)mid.y - DPI(3), DPI(6), DPI(6)), c);
}

void GraphView::DrawToken(Draw& w, const VisualToken& t)
{
    Rect fr = GetNodeRectById(t.from);
    Rect tr = GetNodeRectById(t.to);
    if(fr.IsEmpty() || tr.IsEmpty())
        return;

    Point start = EdgeAnchor(fr, tr);
    Point end = EdgeAnchor(tr, fr);
    Pointf p0(start.x, start.y);
    Pointf p3(end.x, end.y);
    double dx = abs(end.x - start.x);
    Pointf p1(start.x + max(60.0, dx * 0.45), start.y);
    Pointf p2(end.x - max(60.0, dx * 0.45), end.y);
    double tt = min(1.0, max(0.0, t.progress));
    Pointf p = CubicPoint(p0, p1, p2, p3, tt);

    int r = t.batch ? DPI(7) : DPI(5);
    w.DrawEllipse(RectC((int)p.x - r, (int)p.y - r, 2 * r, 2 * r), t.color);
    w.DrawEllipse(RectC((int)p.x - DPI(2), (int)p.y - DPI(2), DPI(4), DPI(4)), White());
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
            model_->tokens[i].progress += 0.018;
            if(model_->tokens[i].progress >= 1.0) {
                if(VisualNodeSpec* to = model_->FindNode(model_->tokens[i].to)) {
                    to->processed_count++;
                    to->complete = to->id == "DONE";
                }
                model_->tokens.Remove(i);
            }
        }
        SyncNodeCards();
    }
    SetTimeCallback(16, THISBACK(Tick));
}

}
