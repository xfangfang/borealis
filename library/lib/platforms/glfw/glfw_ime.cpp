/*
    Copyright 2019  WerWolv
    Copyright 2019  p-sam
    Copyright 2023  xfangfang

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <borealis/core/box.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/glfw/glfw_ime.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/label.hpp>
#include <codecvt>
#include <cstring>
#include <iostream>
#include <locale>

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

class EditTextDialog : public Box
{
  public:
    EditTextDialog()
    {
        this->inflateFromXMLString(editTextDialogXML);

        // submit text
        this->registerAction(
            "hints/submit"_i18n, BUTTON_START, [this](...)
            {
                Application::popActivity(TransitionAnimation::NONE, [this](){
                        this->summitEvent.fire();
                    });
                return true; });
        this->registerAction(
            "hints/ok"_i18n, BUTTON_A, [this](...)
            {
                Application::popActivity(TransitionAnimation::NONE, [this](){
                        this->summitEvent.fire();
                });
                return true; },
            true);

        // cancel input
        this->registerAction(
            "hints/back"_i18n, BUTTON_BACK, [this](...)
            {
                Application::popActivity(TransitionAnimation::NONE, [this](){
                        this->cancelEvent.fire();
                    });

                return true; });

        this->init = true;
    }

    void open()
    {
        Application::pushActivity(new Activity(this));
    }

    void setText(const std::string& value)
    {
        this->content = value;
        this->updateUI();
    }

    void setHeaderText(const std::string& value)
    {
        this->header->setText(value);
    }

    void setHintText(const std::string& value)
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

    void setCountText(const std::string& value)
    {
        this->count->setText(value);
    }

    bool isTranslucent() override
    {
        return true;
    }

    void onLayout() override
    {
        if (!init)
            return;
        this->layoutEvent.fire(Point { this->label->getX(),
            this->label->getY() + this->label->getHeight() });
    }

    Event<Point>* getLayoutEvent()
    {
        return &this->layoutEvent;
    }

    Event<>* getCancelEvent()
    {
        return &this->cancelEvent;
    }

    Event<>* getSubmitEvent()
    {
        return &this->summitEvent;
    }

    void updateUI()
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

  private:
    std::string content;
    std::string hint;
    Event<Point> layoutEvent;
    Event<> cancelEvent, summitEvent;
    bool init = false;

    BRLS_BIND(brls::Label, header, "brls/dialog/header");
    BRLS_BIND(brls::Label, label, "brls/dialog/label");
    BRLS_BIND(brls::Label, count, "brls/dialog/count");
    BRLS_BIND(brls::Box, container, "brls/container");
};

static int currentIMEStatus = GLFW_FALSE;
#define MAX_PREEDIT_LEN 128
static char preeditBuf[MAX_PREEDIT_LEN] = "";

static size_t encode_utf8(char* s, unsigned int ch)
{
    size_t count = 0;

    if (ch < 0x80)
        s[count++] = (char)ch;
    else if (ch < 0x800)
    {
        s[count++] = (ch >> 6) | 0xc0;
        s[count++] = (ch & 0x3f) | 0x80;
    }
    else if (ch < 0x10000)
    {
        s[count++] = (ch >> 12) | 0xe0;
        s[count++] = ((ch >> 6) & 0x3f) | 0x80;
        s[count++] = (ch & 0x3f) | 0x80;
    }
    else if (ch < 0x110000)
    {
        s[count++] = (ch >> 18) | 0xf0;
        s[count++] = ((ch >> 12) & 0x3f) | 0x80;
        s[count++] = ((ch >> 6) & 0x3f) | 0x80;
        s[count++] = (ch & 0x3f) | 0x80;
    }

    return count;
}

void GLFWImeManager::ime_callback(GLFWwindow* window)
{
    currentIMEStatus = glfwGetInputMode(window, GLFW_IME);
    brls::Logger::info("IME switched: {}", currentIMEStatus ? "ON" : "OFF");
}

void GLFWImeManager::preedit_callback(GLFWwindow* window, int preeditCount,
    unsigned int* preeditString, int blockCount,
    int* blockSizes, int focusedBlock, int caret)
{
    int blockIndex = -1, remainingBlockSize = 0;
    if (preeditCount == 0 || blockCount == 0)
    {
        strcpy(preeditBuf, "(empty)");
        preeditTextBuffer = "|";
        return;
    }

    strcpy(preeditBuf, "");

    for (int i = 0; i < preeditCount; i++)
    {
        char encoded[5]     = "";
        size_t encodedCount = 0;

        if (i == caret)
        {
            if (strlen(preeditBuf) + strlen("|") < MAX_PREEDIT_LEN)
                strcat(preeditBuf, "|");
        }
        if (remainingBlockSize == 0)
        {
            if (blockIndex == focusedBlock)
            {
                if (strlen(preeditBuf) + strlen("]") < MAX_PREEDIT_LEN)
                    strcat(preeditBuf, "]");
            }
            blockIndex++;
            remainingBlockSize = blockSizes[blockIndex];
            if (blockIndex == focusedBlock)
            {
                if (strlen(preeditBuf) + strlen("[") < MAX_PREEDIT_LEN)
                    strcat(preeditBuf, "[");
            }
        }
        encodedCount          = encode_utf8(encoded, preeditString[i]);
        encoded[encodedCount] = '\0';
        if (strlen(preeditBuf) + strlen(encoded) < MAX_PREEDIT_LEN)
            strcat(preeditBuf, encoded);
        remainingBlockSize--;
    }
    if (blockIndex == focusedBlock)
    {
        if (strlen(preeditBuf) + strlen("]") < MAX_PREEDIT_LEN)
            strcat(preeditBuf, "]");
    }
    if (caret == preeditCount)
    {
        if (strlen(preeditBuf) + strlen("|") < MAX_PREEDIT_LEN)
            strcat(preeditBuf, "|");
    }

    preeditTextBuffer = std::string { preeditBuf };
}

void GLFWImeManager::char_callback(GLFWwindow* window, unsigned int codepoint)
{
    if (!showIME)
        return;
    textBuffer += codepoint;
}

GLFWImeManager::GLFWImeManager(GLFWwindow* window)
    : window(window)
{
    showIME          = false;
    currentIMEStatus = glfwGetInputMode(window, GLFW_IME);
    glfwSetPreeditCursorRectangle(window, 0, 0, 1, 1);
    glfwSetIMEStatusCallback(window, ime_callback);
    glfwSetPreeditCallback(window, preedit_callback);
    glfwSetCharCallback(window, char_callback);
}

void GLFWImeManager::openInputDialog(std::function<void(std::string)> cb, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText)
{
    preeditTextBuffer.clear();
    showIME                = true;
    textBuffer             = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(initialText);
    EditTextDialog* dialog = new EditTextDialog();
    dialog->setHintText(subText);
    dialog->setText(initialText);
    dialog->setHeaderText(headerText);
    dialog->setCountText("0/" + std::to_string(maxStringLength));
    dialog->getLayoutEvent()->subscribe([this](Point p)
        { glfwSetPreeditCursorRectangle(window, p.x, p.y, 1, 1); });

    // update
    auto eventID = Application::getRunLoopEvent()->subscribe([dialog, maxStringLength]()
        {
            static std::wstring lastText = textBuffer;
            static std::string lastPreeditText = preeditTextBuffer;
            if(lastText != textBuffer){
                if(textBuffer.size() > maxStringLength)
                    textBuffer.erase(maxStringLength, textBuffer.size() - maxStringLength);
                lastText = textBuffer;
                if(textBuffer.empty()){
                    dialog->setText("");
                } else{
                    dialog->setText(getInputText() + "|");
                }
                dialog->setCountText(std::to_string(textBuffer.size()) + "/" + std::to_string(maxStringLength));
                lastPreeditText.clear();
                preeditTextBuffer.clear();
            } else if(lastPreeditText != preeditTextBuffer){
                dialog->setText(getInputText() + preeditTextBuffer);
            } });

    // delete text
    dialog->registerAction(
        "hints/delete"_i18n, BUTTON_B, [dialog](...)
        {
            if(textBuffer.empty()) return true;
            textBuffer.erase(textBuffer.size()-1, 1);
            dialog->setText(getInputText());
            return true; },
        true, true);

    // cancel
    dialog->getCancelEvent()->subscribe([this, eventID]()
        {
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false; });

    // submit
    dialog->getSubmitEvent()->subscribe([this, eventID, cb]()
        {
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false;
            cb(getInputText());
            return true; });

    dialog->open();
}

std::string GLFWImeManager::getInputText()
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(textBuffer);
}

bool GLFWImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    this->openInputDialog([f](const std::string& text)
        {if(!text.empty()) f(text); },
        headerText, subText, maxStringLength, initialText);
    return true;
}

bool GLFWImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton,
    int kbdDisableBitmask)
{
    this->openInputDialog([f](const std::string& text)
        {
            if(text.empty()) return ;
            try
            {
                f(stoll(text));
            }
            catch (const std::exception& e)
            {
                Logger::error("Could not parse input, did you enter a valid integer?");
            } },
        headerText, subText, maxStringLength, initialText);
    return true;
}

};