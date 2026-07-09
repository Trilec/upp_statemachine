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
    part_a_ = spec.part_a;
    part_b_ = spec.part_b;
    assembled_ = spec.assembled;
    review_ = spec.under_review;
    rejected_ = spec.rejected;
    recycled_ = spec.recycled;
    packaging_ = spec.packaging_buffer;
    shipping_ = spec.shipping;

    SetTitle(spec.title);
    SetSubTitle(spec.subtitle);
    SetCopyText(spec.copy);
    ApplyNodeLook();
    Refresh();
}

Color StateNodeCard::AccentColor() const
{
    if(node_id_ == "GEN_A")
        return Color(56, 189, 248);
    if(node_id_ == "GEN_B")
        return Color(45, 212, 191);
    if(node_id_ == "ASSEMBLY")
        return Color(16, 185, 129);
    if(node_id_ == "QUALITY_CHECK")
        return Color(245, 158, 11);
    if(node_id_ == "QUALITY_REVIEW")
        return Color(251, 191, 36);
    if(node_id_ == "DISASSEMBLY")
        return Color(239, 68, 68);
    if(node_id_ == "PACKAGING")
        return Color(124, 58, 237);
    if(node_id_ == "SHIPPING")
        return Color(16, 185, 129);
    return Color(100, 116, 139);
}

String StateNodeCard::StatusText() const
{
    if(node_id_ == "GEN_A")
        return Format("New:%d", part_a_);
    if(node_id_ == "GEN_B")
        return Format("New:%d", part_b_);
    if(node_id_ == "ASSEMBLY")
        return Format("Wait A:%d B:%d", part_a_, part_b_);
    if(node_id_ == "QUALITY_CHECK")
        return Format("Checking:%d", assembled_);
    if(node_id_ == "QUALITY_REVIEW")
        return Format("Review:%d", review_);
    if(node_id_ == "DISASSEMBLY")
        return Format("Process:%d", rejected_);
    if(node_id_ == "PACKAGING")
        return Format("Buffer:%d", packaging_);
    if(node_id_ == "SHIPPING")
        return Format("Ship:%d", shipping_);
    return active_ ? "ACTIVE" : "IDLE";
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

    Rect pill(r.right - DPI(72), r.top + DPI(7), r.right - DPI(8), r.top + DPI(23));
    w.DrawRect(pill, Blend(accent, Black(), 68));
    w.DrawText(pill.left + DPI(6), pill.top + DPI(3), StatusText(), SansSerifZ(8).Bold(), White());

    String count;
    if(node_id_ == "GEN_A")
        count = Format("New:%d", part_a_);
    else if(node_id_ == "GEN_B")
        count = Format("New:%d", part_b_);
    else if(node_id_ == "ASSEMBLY")
        count = Format("Waiting A:%d  Waiting B:%d", part_a_, part_b_);
    else if(node_id_ == "QUALITY_CHECK")
        count = Format("Checking:%d", assembled_);
    else if(node_id_ == "QUALITY_REVIEW")
        count = Format("Review:%d", review_);
    else if(node_id_ == "DISASSEMBLY")
        count = Format("Processing:%d", rejected_);
    else if(node_id_ == "PACKAGING")
        count = Format("Buffer:%d", packaging_);
    else if(node_id_ == "SHIPPING")
        count = Format("Shipments:%d", shipping_);
    else
        count = Format("A:%d B:%d U:%d R:%d X:%d P:%d", part_a_, part_b_, assembled_, review_, rejected_, packaging_);
    w.DrawText(r.left + DPI(10), r.bottom - DPI(18), count, MonospaceZ(8), Color(148, 163, 184));
}

}
