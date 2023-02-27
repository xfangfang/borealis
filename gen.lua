local sdk_version = "10.0.22621.0"
local vctool_version = "14.33.31629"

local VCToolsInstallDir = format("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\%s\\", vctool_version)
local WindowsSdkVerBinPath = format("C:\\Program Files (x86)\\Windows Kits\\10\\References\\%s\\", sdk_version)

local winmds = {}

for _, p in ipairs(storeWinmds) do
	table.insert(winmds, format("%s%s", VCToolsInstallDir, p))
end
for _, p in ipairs(kitWinmds) do
	local dir = WindowsSdkVerBinPath..p
	for _, filedir in ipairs(os.filedirs(dir.."/*")) do
		if os.isfile(filedir.."\\"..p..".winmd") then
			table.insert(winmds, format("%s", filedir.."\\"..p..".winmd"))
			break
		end
	end
end
os.execv("./build/cppwinrt", {"-in", "local", "-out", "build/include"})
os.execv("./build/cppwinrt", table.join2({"-in"}, winmds, {"-out", "build/include"}))
