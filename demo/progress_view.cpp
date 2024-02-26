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

    this->registerAction("back", brls::ControllerButton::BUTTON_B, [this](brls::View* view) {
        this->dismiss();
        return true;
    });

    // Démarrer la tâche de chargement
    std::thread loadingThread([this]() {
        while (this->progressValue < 1000) {
            this->progressValue += 10;
            brls::sync([this]() { updateProgressOnMainThread(progressValue); }); // Mettre à jour la progression sur le thread principal
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Attendre 500 millisecondes entre chaque mise à jour
        }

        brls::sync([this]() { finishLoadingOnMainThread(); }); // Signaler la fin du chargement sur le thread principal
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
