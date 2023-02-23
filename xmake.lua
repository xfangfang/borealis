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
end
if is_plat("mingw") then
    add_defines("WINVER=0x0605")
end

package("yoga")
    set_homepage("https://yogalayout.com")
    set_description("Yoga is a cross-platform layout engine which implements Flexbox.")
    set_license("MIT")
    if os.exists("../yoga") then
        set_sourcedir("../yoga")
    else
        -- set_sourcedir(os.getenv("YOGA_PATH"))
        set_urls(
            "https://github.com/facebook/yoga/archive/220d2582c94517b59d1c36f1c2faf5e3f88306f1.zip"
        )
        add_versions("latest", "a4477216404e803ba91adfdf672339e2e0ca803ec104a0b31354fed066e5ed46")
    end
    on_install(function (package)
        io.writefile("xmake.lua", [[
add_rules("mode.debug", "mode.release")
local sourceFiles = {
    "yoga/**.cpp"
}

target("yoga")
    set_kind("$(kind)")
    set_languages("c++17")
    add_includedirs(".")
    add_headerfiles("yoga/*.h", {prefixdir = "yoga"})
    add_headerfiles("yoga/event/*.h", {prefixdir = "yoga/event"})
    for _, f in ipairs(sourceFiles) do
        add_files(f)
    end
    if is_plat("windows") then
        add_cxflags("/utf-8")
    end
]])
        local configs = {}
        import("package.tools.xmake").install(package, configs)
    end)
package_end()
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
    "YG_ENABLE_EVENTS",
    "FMT_CONSTEVAL="
)


local windowLib = get_config("window")

if windowLib == "sdl" then
    add_requires("sdl2")
elseif windowLib == "glfw" then
    add_requires("xfangfang_glfw")
end

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
    add_packages("tinyxml2", "zeromake_nanovg", "fmt", "tweeny", "yoga")
    add_deps("borealis")
    if is_plat("mingw") then
        add_ldflags("-static")
        add_files("demo/resource.rc")
    elseif is_plat("windows") then 
        add_files("demo/resource.manifest")
    end
