/*
    Author
    - C Edwards (dodobar)

    License
    - Apache License 2.0, matching this repository's LICENSE file.

    StateNodeCard
    =============

    Purpose
    - UiTitleCard-derived visual node used by StateMachineVisualizer.

    Intent
    - Reuse the existing themed title-card surface instead of custom-painting
      every node from scratch.
    - Keep state-machine ownership outside the card; this class is a projection
      of VisualNodeSpec only.

    Thread context
    - GUI thread only.
*/

#ifndef _StateMachineVisualizer_StateNodeCard_h_
#define _StateMachineVisualizer_StateNodeCard_h_

#include <Ui/Ui.h>
#include "VisualizerModel.h"

namespace Upp {

class StateNodeCard : public UiTitleCard {
public:
    typedef StateNodeCard CLASSNAME;

    StateNodeCard();

    void SetNode(const VisualNodeSpec& spec);
    const String& GetNodeId() const { return node_id_; }

    virtual void Paint(Draw& w) override;

private:
    void ApplyNodeLook();
    Color AccentColor() const;
    String StatusText() const;

private:
    String node_id_;
    bool active_ = false;
    bool error_ = false;
    bool complete_ = false;
    int active_count_ = 0;
    int queued_count_ = 0;
    int processed_count_ = 0;
};

}

#endif
