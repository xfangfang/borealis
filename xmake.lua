add_rules("mode.debug", "mode.release")

set_languages("c++17")

option("platfrom")
    set_default("desktop")
    set_showmenu(true)
option_end()

option("window")
    set_default("glfw")
    set_showmenu(true)
option_end()

add_requires("xfangfang_glfw")
add_requires("tinyxml2")
add_requires("yoga")
add_requires("stb")
add_requires("nanovg")
add_requires("nlohmann_json")
add_requires("glad")

target("borealis")
    -- set_kind("static")
    set_kind("shared")
    for _, dir in ipairs({
        "include",
        "include/borealis/extern",
        "lib/extern/tweeny/include",
        "lib/extern/fmt/include"
    }) do
        add_includedirs(path.join("library", dir))
    end
    for _, dir in ipairs({
        "lib/core",
        "lib/core/touch",
        "lib/views",
        "lib/views/cells",
        "lib/views/widgets"
    }) do
        add_files(path.join("library", dir, "*.cpp"))
    end
    for _, dir in ipairs({
        "lib/extern/libretro-common/compat",
        "lib/extern/libretro-common/encodings",
        "lib/extern/libretro-common/features",
    }) do
        add_files(path.join("library", dir, "*.c"))
    end
    add_files("library/lib/extern/fmt/src/*.cc")
    local windowLib = get_config("window")
    if windowLib == "glfw" then
        add_files("library/lib/platforms/glfw/*.cpp")
        add_files("library/lib/platforms/desktop/*.cpp")
    elseif windowLib == "sdl" then
        add_files("library/lib/platforms/sdl/*.cpp")
        add_files("library/lib/platforms/desktop/*.cpp")
    end
    add_packages("tinyxml2", "yoga", "nlohmann_json", "xfangfang_glfw", "glad")
    add_frameworks("CoreFoundation")
    add_defines(
        'BRLS_RESOURCES="./resources/"',
        "YG_ENABLE_EVENTS",
        "__GLFW__"
    )

target("demo")
    for _, dir in ipairs({
        "include",
        "lib/extern/fmt/include",
        "include/borealis/extern",
        "lib/extern/tweeny/include"
    }) do
        add_includedirs(path.join("library", dir))
    end
    add_files("demo/*.cpp")
    add_packages("tinyxml2", "yoga")
    add_defines(
        'BRLS_RESOURCES="./resources/"',
        "YG_ENABLE_EVENTS",
        "__GLFW__"
    )
    add_deps("borealis")