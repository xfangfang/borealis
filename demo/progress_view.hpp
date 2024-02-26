#pragma once

#include <borealis.hpp>

class ProgressBarView : public brls::Box {
    public:
        ProgressBarView();

        static brls::View* create();
    private:
        BRLS_BIND(brls::ProgressDisplay, progress, "progress");
        BRLS_BIND(brls::Label, label, "label");
        int progressValue = 0;

        void updateProgressOnMainThread(int value);
        void finishLoadingOnMainThread();

        bool stopThread = false;
};