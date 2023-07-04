add_rules("mode.debug", "mode.release")

if is_plat("windows") then
    set_languages("c++20")
else
    set_languages("c++17")
end

option("platfrom")
    set_default("desktop")
    set_showmenu(true)
option_end()

option("window")
    set_default("glfw")
    set_showmenu(true)
option_end()

option("driver")
    set_default("opengl")
    set_showmenu(true)
option_end()

option("winrt")
    set_default(false)
    set_showmenu(true)
option_end()

option("example")
    set_default(false)
    set_showmenu(true)
option_end()

if is_plat("windows") then
    add_cxflags("/utf-8")
    add_includedirs("library/include/compat")
    if is_mode("release") then
        set_optimize("faster")
    end
end
if is_plat("mingw") then
    add_defines("WINVER=0x0605")
end


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


local windowLib = get_config("window")

if windowLib == "sdl" then
    if get_config("winrt") then
        add_requires("sdl2", {configs={shared=true,winrt=true}})
        add_requires("cppwinrt")
    else
        add_requires("sdl2")
    end
elseif windowLib == "glfw" then
    add_requires("xfangfang_glfw")
end

target("borealis")
    set_kind("static")
    -- set_kind("shared")
    add_includedirs("library/include")
    add_includedirs("library/include/borealis/extern")
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
        add_packages("xfangfang_glfw")
        add_defines("__GLFW__")
    elseif windowLib == "sdl" then
        add_files("library/lib/platforms/sdl/*.cpp")
        add_files("library/lib/platforms/desktop/*.cpp")
        add_packages("sdl2")
        if get_config("winrt") then
            add_defines("SDL_VIDEO_DRIVER_WINRT")
            add_packages("cppwinrt")
            add_files("library/lib/platforms/driver/winrt.cpp")
        end
        add_defines("__SDL2__")
    end
    local driver = get_config("driver")
    if driver == "metal" then
        add_defines("BOREALIS_USE_METAL")
        add_frameworks("Metal", "MetalKit", "QuartzCore")
        add_files("library/lib/platforms/glfw/driver/metal.mm")
        add_links("nanovg_metal")
    elseif driver == "opengl" then
        add_defines("BOREALIS_USE_OPENGL")
        add_packages("glad")
    elseif driver == "d3d11" then
        add_files("library/lib/platforms/driver/d3d11.cpp")
        if get_config("winrt") then
            add_defines("__WINRT__=1")
        end
        add_defines("BOREALIS_USE_D3D11")
        add_syslinks("d3d11")
        -- d3d11 可以开启可变帧率
        -- add_defines("__ALLOW_TEARING__=1")
    end
    add_packages("tinyxml2", "nlohmann_json", "nanovg", "fmt", "tweeny", "yoga", "stb")
    add_defines("BOREALIS_USE_STD_THREAD")


if get_config("example") then
    target("demo")
        add_includedirs("library/include")
        add_includedirs("library/include/borealis/extern")
        add_files("demo/*.cpp")
        add_packages("tinyxml2", "nanovg", "fmt", "tweeny", "yoga")
        local windowLib = get_config("window")
        if windowLib == "sdl" then
            add_packages("sdl2")
        end
        add_deps("borealis")
        if get_config("winrt") then
            add_defines("__WINRT__=1")
            add_syslinks("WindowsApp")
            after_build(function (target)
                import("uwp")(target)
            end)
        end
        if is_plat("mingw") then
            add_ldflags("-static")
        end
        if is_plat("windows", "mingw") then
            add_syslinks("Wlanapi", "iphlpapi", "Ws2_32")
            add_files("demo/resource.rc")
        end
        set_rundir("$(projectdir)")
end
