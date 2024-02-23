#import <borealis/core/logger.hpp>
#import <borealis/platforms/desktop/desktop_platform.hpp>
#import <UIKit/UIKit.h>

namespace brls {

ThemeVariant ios_theme() {
    if (UIScreen.mainScreen.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark)
        return ThemeVariant::DARK;
    else
        return ThemeVariant::LIGHT;
}

bool darwin_runloop(const std::function<bool()>& runLoopImpl) {
    @autoreleasepool {
        return runLoopImpl();
    }
}

uint8_t ios_battery_status() {
    UIDevice.currentDevice.batteryMonitoringEnabled = true;
    return UIDevice.currentDevice.batteryState;
}

float ios_battery() {
    return UIDevice.currentDevice.batteryLevel;
}
};
