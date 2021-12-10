/*
    Copyright 2021 CrustySeanPro

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

#include "borealis/views/widgets/account.hpp"

namespace brls
{

AccountWidget::AccountWidget()
{
    setSize(Size(44, 44));

    rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc)) {
        brls::Logger::error("accountInitialize() failed: {}\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        rc = accountGetPreselectedUser(&userID);

        if (R_FAILED(rc)) {
            brls::Logger::error("accountGetPreselectedUser() failed: {}\n", rc);
        }
    }

    acc = new Image();
    acc->setSize(Size(44, 44));
    acc->setScalingType(ImageScalingType::FIT);
    acc->detach();

    getIcon(userID);

    addView(acc);
}

void AccountWidget::getIcon(AccountUid id)
{
    AccountProfile profile;
    AccountProfileBase profilebase;
    memset(&profilebase, 0, sizeof(profilebase));

    if (R_SUCCEEDED(accountGetPreselectedUser(&id))) {
        accountGetProfile(&profile, id);

        u32 iconsize = 0;
        if (R_SUCCEEDED(accountProfileGetImageSize(&profile, &iconsize))) {
            u8* icon = new u8[iconsize];
            u32 fakesize = 0;

            if(R_SUCCEEDED(accountProfileLoadImage(&profile, icon, iconsize, &fakesize))) {
                acc->setImageFromMem(icon, iconsize);
            }

            delete[] icon;
        }
        accountProfileClose(&profile);
    }
}

void AccountWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) 
{
    Box::draw(vg, x, y, width, height, style, ctx);
}

View* AccountWidget::create()
{
    return new AccountWidget();
}

} // namespace brls