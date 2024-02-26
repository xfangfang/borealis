#pragma once

#include <borealis/core/view.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/progress_spinner.hpp>

namespace brls {

// A progress bar with an optional spinner and percentage text.
class ProgressDisplay : public View {
    public:
        ProgressDisplay();

        void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
        void willAppear(bool resetState = false) override;
        void willDisappear(bool resetState = false) override;    

        void setProgress(int current, int max);
        void setProgress(float current, float max);

        static brls::View* create();
    private:
        float progressPercentage = 0.0f;
};

} // namespace brls