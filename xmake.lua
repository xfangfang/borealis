add_rules("mode.debug", "mode.release")

set_languages("c++20")

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

-- https://github.com/zeromake/nanovg
package("zeromake_nanovg")
    if os.exists("../nanovg") then
        set_sourcedir("../nanovg")
    else
        set_sourcedir(os.getenv("NANOVG_PATH"))
    end
    add_links("nanovg")
    on_install(function (package)
        local configs = {}
        import("package.tools.xmake").install(package, configs)
    end)
package_end()

add_requires("tinyxml2")
add_requires("yoga")
add_requires("stb")
add_requires("zeromake_nanovg")
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

target("winrt")
    set_kind("static")
    add_includedirs("library/include")
    add_defines("SDL_VIDEO_DRIVER_WINRT")
    add_defines("__SDL2__")
    add_defines("__WINRT__")
    add_files("library/lib/platforms/driver/winrt.cpp")
    add_packages("sdl2", "cppwinrt")

target("borealis")
    set_kind("static")
    -- set_kind("shared")
    add_includedirs("library/include")
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
            add_deps("winrt")
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
        -- Todo: winrt 的 cpp 需要单独走 vs 的 winrt 编译，考虑把 winrt 的代码单独提取
        add_files("library/lib/platforms/driver/d3d11.cpp")
        if get_config("winrt") then
            add_defines("__WINRT__=1")
        end
        add_defines("BOREALIS_USE_D3D11")
        add_syslinks("d3d11")
        -- d3d11 可以开启可变帧率
        -- add_defines("__ALLOW_TEARING__=1")
    end
    add_packages("tinyxml2", "nlohmann_json", "zeromake_nanovg", "fmt", "tweeny", "yoga", "stb")
    add_defines("BOREALIS_USE_STD_THREAD")

target("demo")
    add_includedirs("library/include")
    add_files("demo/*.cpp")
    add_packages("tinyxml2", "zeromake_nanovg", "fmt", "tweeny", "yoga", "sdl2")
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
        add_files("demo/resource.rc")
    elseif is_plat("windows") then 
        add_files("demo/resource.manifest")
    end
