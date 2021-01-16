dep_glfw3 = dependency('glfw3', version : '>=3.3')
dep_glm   = dependency('glm', version : '>=0.9.8')

# TODO: cleanup that file
borealis_files = files(
    'lib/logger.cpp',
    'lib/animations.cpp',

    'lib/application.cpp',
    # 'lib/notification_manager.cpp',
    'lib/task_manager.cpp',
    'lib/i18n.cpp',
    'lib/swkbd.cpp',

    'lib/theme.cpp',
    'lib/style.cpp',

    'lib/activity.cpp',
    'lib/view.cpp',

    'lib/box.cpp',
    'lib/scrolling_frame.cpp',

    'lib/applet_frame.cpp',
    'lib/tab_frame.cpp',
    'lib/rectangle.cpp',
    'lib/sidebar.cpp',
    # 'lib/list.cpp', REMOVE+header?
    'lib/label.cpp',
    # 'lib/crash_frame.cpp',
    'lib/button.cpp',
    # 'lib/table.cpp',
    # 'lib/dropdown.cpp',
    # 'lib/staged_applet_frame.cpp',
    # 'lib/progress_display.cpp',
    # 'lib/progress_spinner.cpp',
    'lib/image.cpp',
    'lib/header.cpp',
    # 'lib/popup_frame.cpp',
    # 'lib/thumbnail_frame.cpp',
    # 'lib/layer_view.cpp',
    # 'lib/dialog.cpp',
    # 'lib/material_icon.cpp',
    # 'lib/hint.cpp',
    # 'lib/absolute_layout.cpp', REMOVE + header, add setPosition methods to View

    'lib/repeating_task.cpp',

    'lib/extern/glad/glad.c',
    'lib/extern/nanovg/nanovg.c',
    'lib/extern/tinyxml2/tinyxml2.cpp',

    'lib/extern/fmt/src/format.cc',
    'lib/extern/fmt/src/os.cc',

    'lib/extern/libretro-common/compat/compat_strl.c',
    'lib/extern/libretro-common/features/features_cpu.c',
    'lib/extern/libretro-common/encodings/encoding_utf.c',

    'lib/extern/yoga/src/yoga/event/event.cpp',
    'lib/extern/yoga/src/yoga/log.cpp',
    'lib/extern/yoga/src/yoga/Utils.cpp',
    'lib/extern/yoga/src/yoga/YGConfig.cpp',
    'lib/extern/yoga/src/yoga/YGEnums.cpp',
    'lib/extern/yoga/src/yoga/YGLayout.cpp',
    'lib/extern/yoga/src/yoga/YGNode.cpp',
    'lib/extern/yoga/src/yoga/YGStyle.cpp',
    'lib/extern/yoga/src/yoga/YGValue.cpp',
    'lib/extern/yoga/src/yoga/Yoga.cpp',
)

borealis_include = include_directories(
    'include',
    'lib/extern/fmt/include',
    'include/borealis/extern',
    'include/borealis/extern/tinyxml2',
    'lib/extern/yoga/src',
)

borealis_dependencies = [ dep_glfw3, dep_glm, ]
borealis_cpp_args = [ '-DYG_ENABLE_EVENTS', ]
