#include <borealis/core/application.hpp>
#include <borealis/views/progress_display.hpp>

namespace brls
{

ProgressDisplay::ProgressDisplay()
{

}

void ProgressDisplay::setProgress(int current, int max) {
    if(current > max) return;

    this->progressPercentage = ((current * 100) / max);
}


void ProgressDisplay::setProgress(float current, float max)
{
    if (current > max) return;

    this->progressPercentage = ((current * 100) / max);
}

void ProgressDisplay::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    height = 50.f;

    Theme theme       = Application::getTheme();
    NVGcolor emptyBarColor = a(theme["brls/progress_bar/empty_bar_color"]);
    NVGcolor barColor = a(theme["brls/progress_bar/bar_color"]);

    // progressBarX = 600;
    // progressBarWidth = 400;

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, y + height / 2);
    nvgLineTo(vg, x + width, y + height / 2);
    nvgStrokeColor(vg, emptyBarColor);
    nvgStrokeWidth(vg, height / 3);
    nvgLineCap(vg, NVG_ROUND);
    nvgStroke(vg);

    if(this->progressPercentage > 0.0f) {
        nvgBeginPath(vg);
        nvgMoveTo(vg, x, y + height / 2);
        nvgLineTo(vg, x + ((float)width * this->progressPercentage) / 100, y + height / 2);
        nvgStrokeColor(vg, barColor);
        nvgStrokeWidth(vg, height / 3);
        nvgLineCap(vg, NVG_ROUND);
        nvgStroke(vg);
    }
}

void ProgressDisplay::willAppear(bool resetState) {

}

void ProgressDisplay::willDisappear(bool resetState) {

}

brls::View* ProgressDisplay::create() {
    return new ProgressDisplay();

}

} // namespace brls