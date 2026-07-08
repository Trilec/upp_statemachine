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
    const int node_h = DPI(92);
    const int cell_w = DPI(228);
    const int cell_h = DPI(126);
    const int margin_x = DPI(42);
    const int margin_y = DPI(30);
    return RectC(margin_x + n.col * cell_w, margin_y + n.row * cell_h, node_w, node_h);
}

Rect GraphView::GetNodeRectById(const String& id) const
{
    if(!model_)
        return Rect();
    const VisualNodeSpec* n = model_->FindNode(id);
    return n ? GetNodeRect(*n) : Rect();
}

Pointf GraphView::PortPoint(const Rect& r, EdgePort port) const
{
    switch(port) {
    case EdgePort::LeftTop:     return Pointf(r.left, r.top + (r.bottom - r.top) * 0.22);
    case EdgePort::LeftCenter:  return Pointf(r.left, r.CenterPoint().y);
    case EdgePort::LeftBottom:  return Pointf(r.left, r.bottom - (r.bottom - r.top) * 0.22);
    case EdgePort::RightTop:    return Pointf(r.right, r.top + (r.bottom - r.top) * 0.22);
    case EdgePort::RightCenter: return Pointf(r.right, r.CenterPoint().y);
    case EdgePort::RightBottom: return Pointf(r.right, r.bottom - (r.bottom - r.top) * 0.22);
    case EdgePort::TopLeft:     return Pointf(r.left + (r.right - r.left) * 0.22, r.top);
    case EdgePort::TopCenter:   return Pointf(r.CenterPoint().x, r.top);
    case EdgePort::TopRight:    return Pointf(r.right - (r.right - r.left) * 0.22, r.top);
    case EdgePort::BottomLeft:  return Pointf(r.left + (r.right - r.left) * 0.22, r.bottom);
    case EdgePort::BottomCenter:return Pointf(r.CenterPoint().x, r.bottom);
    case EdgePort::BottomRight: return Pointf(r.right - (r.right - r.left) * 0.22, r.bottom);
    }
    return Pointf(r.CenterPoint().x, r.CenterPoint().y);
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
    if(path.from.IsEmpty() || path.to.IsEmpty())
        return path;

    Pointf start = PortPoint(path.from, e.from_port);
    Pointf end = PortPoint(path.to, e.to_port);
    path.p0 = start;
    path.p3 = end;

    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double bend = max(70.0, abs(dx) * e.curve_bias);
    double lane = e.lane_offset;

    if(abs(dx) >= abs(dy)) {
        path.p1 = Pointf(start.x + bend, start.y + lane);
        path.p2 = Pointf(end.x - bend, end.y + lane);
    }
    else {
        path.p1 = Pointf(start.x + lane, start.y + bend);
        path.p2 = Pointf(end.x + lane, end.y - bend);
    }
    return path;
}

void GraphView::DrawArrowhead(Draw& w, const EdgePath& path, Color c) const
{
    Pointf tip = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.98);
    Pointf tail = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.92);
    double dx = tip.x - tail.x;
    double dy = tip.y - tail.y;
    double len = sqrt(dx * dx + dy * dy);
    if(len <= 0.01)
        return;
    dx /= len;
    dy /= len;
    double px = -dy;
    double py = dx;
    Point p1((int)(tip.x - dx * 12 + px * 5), (int)(tip.y - dy * 12 + py * 5));
    Point p2((int)tip.x, (int)tip.y);
    Point p3((int)(tip.x - dx * 12 - px * 5), (int)(tip.y - dy * 12 - py * 5));
    w.DrawLine(p1.x, p1.y, p2.x, p2.y, 2, c);
    w.DrawLine(p3.x, p3.y, p2.x, p2.y, 2, c);
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

Color GraphView::EdgeColor(const VisualEdgeSpec& e) const
{
    Color c = e.color;
    if(e.dashed)
        c = Blend(c, White(), 64);
    if(model_ && EdgeIsActive(e.id))
        c = Blend(c, White(), 22);
    return c;
}

bool GraphView::EdgeIsActive(const String& id) const
{
    if(!model_)
        return false;
    for(int i = 0; i < model_->tokens.GetCount(); i++)
        if(model_->tokens[i].edge_id == id)
            return true;
    return false;
}

void GraphView::DrawEdge(Draw& w, const VisualEdgeSpec& e)
{
    EdgePath path = MakePath(e);
    if(path.from.IsEmpty() || path.to.IsEmpty())
        return;

    Color c = EdgeColor(e);
    int width = EdgeIsActive(e.id) ? 3 : 2;
    Point last((int)path.p0.x, (int)path.p0.y);
    for(int i = 1; i <= 32; i++) {
        double t = i / 32.0;
        Pointf p = CubicPoint(path.p0, path.p1, path.p2, path.p3, t);
        Point now((int)p.x, (int)p.y);
        if(!e.dashed || (i % 2))
            w.DrawLine(last.x, last.y, now.x, now.y, width, c);
        last = now;
    }

    DrawArrowhead(w, path, c);
    Pointf mid = CubicPoint(path.p0, path.p1, path.p2, path.p3, 0.54);
    Point label((int)mid.x + e.label_offset.x, (int)mid.y + e.label_offset.y);
    w.DrawText(label.x, label.y, e.label, SansSerifZ(8).Bold(), c);
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

    Rect box((int)p.x - r, (int)p.y - r, (int)p.x + r, (int)p.y + r);
    w.DrawEllipse(box, t.color);
    Rect inner(box.left + DPI(2), box.top + DPI(2), box.right - DPI(2), box.bottom - DPI(2));
    w.DrawEllipse(inner, Blend(t.color, White(), 35));
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

}
