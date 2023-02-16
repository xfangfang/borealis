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
add_requires("fmt")
add_requires("tweeny")

add_defines(
    'BRLS_RESOURCES="./resources/"',
    "YG_ENABLE_EVENTS"
)

target("borealis")
    set_kind("static")
    -- set_kind("shared")
    for _, dir in ipairs({
        "include"
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
        "compat",
        "encodings",
        "features",
    }) do
        add_files(path.join("library/lib/extern/libretro-common", dir, "*.c"))
    end
    local windowLib = get_config("window")
    if windowLib == "glfw" then
        add_files("library/lib/platforms/glfw/*.cpp")
        add_files("library/lib/platforms/desktop/*.cpp")
        add_packages("xfangfang_glfw", "glad")
    elseif windowLib == "sdl" then
        add_files("library/lib/platforms/sdl/*.cpp")
        add_files("library/lib/platforms/desktop/*.cpp")
        add_packages("sdl")
    end
    add_packages("tinyxml2", "nlohmann_json", "nanovg", "fmt", "tweeny", "yoga")

target("demo")
    for _, dir in ipairs({
        "include"
    }) do
        add_includedirs(path.join("library", dir))
    end
    add_files("demo/*.cpp")
    add_packages("tinyxml2", "nanovg", "fmt", "tweeny", "yoga")
    add_deps("borealis")
