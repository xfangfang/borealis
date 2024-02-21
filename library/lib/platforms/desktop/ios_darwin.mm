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
};
