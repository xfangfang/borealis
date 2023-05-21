#include <borealis/views/edit_text_dialog.hpp>

namespace brls
{

    const std::string editTextDialogXML = R"xml(
    <brls:Box
        id="brls/container"
        width="auto"
        height="auto"
        axis="column"
        justifyContent="flexStart"
        alignItems="center"
        focusable="true"
        hideHighlight="true"
        backgroundColor="@theme/brls/backdrop">

        <brls:Label
            id="brls/dialog/header"
            fontSize="24"
            marginTop="50"
            marginBottom="30"
            textColor="#FFFFFF"/>

        <brls:Box
            id="brls/dialog/applet"
            width="720"
            cornerRadius="4"
            alignItems="flexEnd"
            axis="column"
            backgroundColor="@theme/brls/background">

            <brls:Label
                id="brls/dialog/label"
                grow="1"
                width="680"
                margin="20"
                autoAnimate="false"
                verticalAlign="top"/>

            <brls:Label
                id="brls/dialog/count"
                width="680"
                horizontalAlign="right"
                fontSize="18"
                textColor="@theme/brls/text_disabled"
                marginRight="30"
                marginBottom="10"/>

            <brls:Hints
                allowAButtonTouch="true"
                addBaseAction="false"
                marginBottom="20"
                marginRight="10"
                width="auto"
                height="auto"/>

        </brls:Box>

    </brls:Box>
    )xml";

    EditTextDialog::EditTextDialog()
    {
        this->inflateFromXMLString(editTextDialogXML);

        // submit text
        this->registerAction(
            "hints/submit"_i18n, BUTTON_START, [this](...)
            {
                Application::popActivity(TransitionAnimation::FADE, [this](){
                        this->summitEvent.fire();
                    });
                return true; });
        this->registerAction(
            "hints/ok"_i18n, BUTTON_A, [this](...)
            {
                Application::popActivity(TransitionAnimation::FADE, [this](){
                        this->summitEvent.fire();
                });
                return true; },
            true);

        // cancel input
        this->registerAction(
            "hints/back"_i18n, BUTTON_BACK, [this](...)
            {
                Application::popActivity(TransitionAnimation::FADE, [this](){
                        this->cancelEvent.fire();
                    });

                return true; });

        this->init = true;
    }

    void EditTextDialog::open()
    {
        Application::pushActivity(new Activity(this));
    }

    void EditTextDialog::setText(const std::string& value)
    {
        this->content = value;
        this->updateUI();
    }

    void EditTextDialog::setHeaderText(const std::string& value)
    {
        this->header->setText(value);
    }

    void EditTextDialog::setHintText(const std::string& value)
    {
        if (value.empty())
        {
            this->hint = "hints/input"_i18n;
        }
        else
        {
            this->hint = value;
        }
        this->updateUI();
    }

    void EditTextDialog::setCountText(const std::string& value)
    {
        this->count->setText(value);
    }

    bool EditTextDialog::isTranslucent()
    {
        return true;
    }

    void EditTextDialog::onLayout()
    {
        if (!init)
            return;
        this->layoutEvent.fire(Point { this->label->getX(),
            this->label->getY() + this->label->getHeight() });
    }

    Event<Point>* EditTextDialog::getLayoutEvent()
    {
        return &this->layoutEvent;
    }

    Event<>* EditTextDialog::getCancelEvent()
    {
        return &this->cancelEvent;
    }

    Event<>* EditTextDialog::getSubmitEvent()
    {
        return &this->summitEvent;
    }

    void EditTextDialog::updateUI()
    {
        if (content.empty())
        {
            label->setTextColor(Application::getTheme().getColor("brls/text_disabled"));
            label->setText(hint);
        }
        else
        {
            label->setTextColor(Application::getTheme().getColor("brls/text"));
            label->setText(content);
        }
    }
}