#include "progress_view.hpp"
#include <chrono>

ProgressBarView::ProgressBarView() {
    this->inflateFromXMLRes("xml/views/progress.xml");

    this->progress->setProgress(this->progressValue, 1000);
    this->progress->setParent(this);
    this->label->setText("Loading...");
    this->label->setParent(this);

    this->setFocusable(true);
    this->setHideHighlightBackground(true);
    this->setHideHighlightBorder(true);

    std::thread loadingThread([this]() {
        while (this->progressValue < 1000) {
            if (this->stopThread) break;
            this->progressValue += 10;
            ASYNC_RETAIN
            brls::sync([ASYNC_TOKEN]() { 
                ASYNC_RELEASE
                updateProgressOnMainThread(progressValue); 
            }); 
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
        }
        ASYNC_RETAIN
        brls::sync([ASYNC_TOKEN]() { 
            ASYNC_RELEASE
            finishLoadingOnMainThread(); 
        });
    });

    this->registerAction("back", brls::ControllerButton::BUTTON_B, [this](brls::View* view) {
        this->stopThread = true;
        this->dismiss();
        return true;
    });

    loadingThread.detach();
}

void ProgressBarView::updateProgressOnMainThread(int value) {
    this->progress->setProgress(value, 1000);
    this->label->setText("Loading... " + std::to_string(value / 10) + "%");
}

void ProgressBarView::finishLoadingOnMainThread() {
    this->progressValue = 1000;
    this->progress->setProgress(progressValue, 1000);
    this->label->setText("Loading... 100%");
}

brls::View* ProgressBarView::create() {
    return new ProgressBarView();
}
