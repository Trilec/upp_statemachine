#include "StateNodeCard.h"

namespace Upp {

StateNodeCard::StateNodeCard()
{
    SetSizeMin(DPI(168), DPI(84));
    SetMediaReserve(DPI(0));
    SetContentInset(DPI(10));
    ShowTitleLine(false);
    ShowCardLine(true);
    EnableHover(true);
}

void StateNodeCard::SetNode(const VisualNodeSpec& spec)
{
    node_id_ = spec.id;
    active_ = spec.active;
    error_ = spec.error;
    complete_ = spec.complete;
    active_count_ = spec.active_count;
    queued_count_ = spec.queued_count;
    processed_count_ = spec.processed_count;

    SetTitle(spec.title);
    SetSubTitle(spec.subtitle);
    SetCopyText(spec.copy);
    ApplyNodeLook();
    Refresh();
}

Color StateNodeCard::AccentColor() const
{
    if(error_)
        return Color(245, 158, 11);
    if(complete_)
        return Color(16, 185, 129);
    if(queued_count_ > 0)
        return Color(56, 189, 248);
    if(active_)
        return Color(14, 165, 233);
    return Color(100, 116, 139);
}

String StateNodeCard::StatusText() const
{
    if(error_)
        return "ERROR";
    if(complete_)
        return "DONE";
    if(queued_count_ > 0)
        return Format("Q:%d", queued_count_);
    if(active_)
        return "ACTIVE";
    return "IDLE";
}

void StateNodeCard::ApplyNodeLook()
{
    UiTitleCard::Style s = UiTheme::ResolveTitleCard();
    const Color accent = AccentColor();

    s.metrics.radius = DPI(10);
    s.metrics.frame_enabled = true;
    s.metrics.frame_width = active_ ? DPI(2) : DPI(1);
    s.metrics.face_enabled = true;
    s.title_font = SansSerifZ(14).Bold();
    s.subtitle_font = SansSerifZ(10).Bold();
    s.copy_font = SansSerifZ(10);
    s.title_color = Color(241, 245, 249);
    s.subtitle_color = accent;
    s.copy_color = Color(148, 163, 184);
    s.card_line = true;
    s.card_line_color_enabled = true;
    s.card_line_color = accent;
    s.card_line_thickness = active_ ? DPI(2) : DPI(1);
    s.card_line_side = UiAlign::BOTTOM;

    for(int i = 0; i < 4; i++) {
        s.palette.face[i] = UiFill::Solid(active_ ? Color(15, 23, 42) : Color(12, 18, 31));
        s.palette.frame[i] = active_ ? accent : Color(51, 65, 85);
        s.palette.ink[i] = Color(226, 232, 240);
    }

    SetCustomStyle(s);
}

void StateNodeCard::Paint(Draw& w)
{
    UiTitleCard::Paint(w);

    Rect r = GetSize();
    const Color accent = AccentColor();

    Rect pill(r.right - DPI(62), r.top + DPI(7), r.right - DPI(8), r.top + DPI(23));
    w.DrawRect(pill, Blend(accent, Black(), 70));
    w.DrawText(pill.left + DPI(6), pill.top + DPI(3), StatusText(), SansSerifZ(8).Bold(), accent);

    String count = Format("A:%d  Q:%d  P:%d", active_count_, queued_count_, processed_count_);
    w.DrawText(r.left + DPI(10), r.bottom - DPI(18), count, MonospaceZ(9), Color(148, 163, 184));
}

}
