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

// https://github.com/glfw/glfw/pull/2130

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

    brls::Logger::debug("preeditBuf: {}", preeditBuf);
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

void GLFWImeManager::openInputDialog(std::function<void(std::string)> cb)
{
    showIME = true;
    textBuffer.clear();
    glfwSetPreeditCursorRectangle(window, 310, 270, 1, 1);
    Box* container = new Box();
    container->setHeight(200);
    Label* content = new Label();
    content->setMargins(20, 20, 20, 20);
    container->addView(content);
    Dialog* dialog = new Dialog(container);
    content->setVerticalAlign(VerticalAlign::TOP);

    auto eventID = Application::getRunLoopEvent()->subscribe([content]()
        {
            static std::wstring lastText = textBuffer;
            if(lastText != textBuffer){
                lastText = textBuffer;
                content->setText(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(textBuffer));
            } });

    // 删除文字
    dialog->registerAction(
        "Delete", BUTTON_B, [content](...)
        {
            if(textBuffer.empty()) return true;
            textBuffer.erase(textBuffer.size()-1, 1);
            content->setText(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(textBuffer));
            return true; },
        true, true);

    dialog->setCancelable(false);
    dialog->addButton("hints/cancel"_i18n, [this, eventID]
        {
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false; });
    dialog->addButton("hints/ok"_i18n, [this, cb, eventID]()
        {
            cb(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(textBuffer));
            glfwSetInputMode(window, GLFW_IME, GLFW_FALSE);
            Application::getRunLoopEvent()->unsubscribe(eventID);
            showIME = false; });
    dialog->open();
}

bool GLFWImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    int kbdDisableBitmask)
{
    this->openInputDialog([f](const std::string& text)
        {if(!text.empty()) f(text); });
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
            } });
    return true;
}

};