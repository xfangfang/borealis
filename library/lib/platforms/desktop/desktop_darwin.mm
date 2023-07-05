/*
    Copyright 2021 natinusala
    Copyright 2023 xfangfang

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

#import <borealis/core/logger.hpp>
#import <borealis/platforms/desktop/desktop_platform.hpp>
#import <CoreWLAN/CoreWLAN.h>

namespace brls
{

// Interface method, fetching the current connection info.
int darwin_wlan_quality() {
    @autoreleasepool {
        CWWiFiClient* Client = CWWiFiClient.sharedWiFiClient;
        CWInterface* currentInterface = Client.interface;
        if ([currentInterface powerOn] == false) {
            return -1;
        }
        if ([currentInterface serviceActive] == false) {
            return 0;
        }
        int rssi = [currentInterface rssiValue];
        if (rssi > -50) return 3;
        if (rssi > -80) return 2;
        return 1;
    }
}

}