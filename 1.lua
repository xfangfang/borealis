import("detect.sdks.find_vstudio")


local storeWinmds = {
	"lib\\x86\\store\\references\\platform.winmd"
}
local kitWinmds = {
	"Windows.AI.MachineLearning.MachineLearningContract",
	"Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract",
	"Windows.ApplicationModel.Calls.Background.CallsBackgroundContract",
	"Windows.ApplicationModel.Calls.CallsPhoneContract",
	"Windows.ApplicationModel.Calls.CallsVoipContract",
	"Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract",
	"Windows.ApplicationModel.SocialInfo.SocialInfoContract",
	"Windows.ApplicationModel.StartupTaskContract",
	"Windows.Devices.Custom.CustomDeviceContract",
	"Windows.Devices.DevicesLowLevelContract",
	"Windows.Devices.Printers.PrintersContract",
	"Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract",
	"Windows.Devices.SmartCards.SmartCardEmulatorContract",
	"Windows.Foundation.FoundationContract",
	"Windows.Foundation.UniversalApiContract",
	"Windows.Gaming.XboxLive.StorageApiContract",
	"Windows.Graphics.Printing3D.Printing3DContract",
	"Windows.Networking.Connectivity.WwanContract",
	"Windows.Networking.Sockets.ControlChannelTriggerContract",
	"Windows.Security.Isolation.Isolatedwindowsenvironmentcontract",
	"Windows.Services.Maps.GuidanceContract",
	"Windows.Services.Maps.LocalSearchContract",
	"Windows.Services.Store.StoreContract",
	"Windows.Services.TargetedContent.TargetedContentContract",
	"Windows.Storage.Provider.CloudFilesContract",
	"Windows.System.Profile.ProfileHardwareTokenContract",
	"Windows.System.Profile.ProfileRetailInfoContract",
	"Windows.System.Profile.ProfileSharedModeContract",
	"Windows.System.Profile.SystemManufacturers.SystemManufacturersContract",
	"Windows.System.SystemManagementContract",
	"Windows.UI.UIAutomation.UIAutomationContract",
	"Windows.UI.ViewManagement.ViewManagementViewScalingContract",
	"Windows.UI.Xaml.Core.Direct.XamlDirectContract"
}


local vs_sdk_versions = find_vstudio()
local vs_sdk = nil;
for _,vs_sdk_archs in pairs(vs_sdk_versions) do
	vs_sdk = vs_sdk_archs["vcvarsall"]["x64"]
	break
end
local sdk_version = "10.0.22621.0"
local vctool_version = "14.33.31629"

local VCToolsInstallDir = format("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\%s\\", vctool_version)
local WindowsSdkVerBinPath = format("C:\\Program Files (x86)\\Windows Kits\\10\\References\\%s\\", sdk_version)

if vs_sdk ~= nil then
	VCToolsInstallDir = vs_sdk.VCToolsInstallDir
	--WindowsSdkVerBinPath = vs_sdk.WindowsSdkVerBinPath
end

local cmd = format('%s%s', VCToolsInstallDir, "bin\\HostX64\\x64\\CL.exe")
local args = {
	"/c",
	"/std:c++20",
	"/await",
	"/Zi",
	-- "/ZW",
	-- "/ZW:nostdlib",
	"/Ilibrary/include",
	"/I../sdl/include",
	"/D _UNICODE /D UNICODE /D WINAPI_FAMILY=WINAPI_FAMILY_APP /D __WRL_NO_DEFAULT_LIB__",
}

local EXTERNAL_INCLUDE = {
	format("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\%s\\include", vctool_version),
	format("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\%s\\ATLMFC\\include", vctool_version),
	"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\VS\\include",
	format("C:\\Program Files (x86)\\Windows Kits\\10\\include\\%s\\ucrt", sdk_version),
	format("C:\\Program Files (x86)\\Windows Kits\\10\\include\\%s\\um", sdk_version),
	format("C:\\Program Files (x86)\\Windows Kits\\10\\include\\%s\\shared", sdk_version),
	format("C:\\Program Files (x86)\\Windows Kits\\10\\include\\%s\\winrt", sdk_version),
	format("C:\\Program Files (x86)\\Windows Kits\\10\\include\\%s\\cppwinrt", sdk_version),
	"C:\\Program Files (x86)\\Windows Kits\\NETFXSDK\\4.8\\include\\um",
}

for _, ii in ipairs(EXTERNAL_INCLUDE) do
	table.insert(args, format("/I\"%s\"", ii))
end

for _, p in ipairs(storeWinmds) do
	table.insert(args, format("/FU\"%s%s\"", VCToolsInstallDir, p))
end
for _, p in ipairs(kitWinmds) do
	local dir = WindowsSdkVerBinPath..p
	for _, filedir in ipairs(os.filedirs(dir.."/*")) do
		if os.isfile(filedir.."\\"..p..".winmd") then
			table.insert(args, format("/FU\"%s\"", filedir.."\\"..p..".winmd"))
			break
		end
	end
end

table.join2(args, {
	"/EHsc",
	"/FS",
	"/Fo\"build/winrt.obj\"",
	"/Fd\"build/windows/x64/debug/borealis.pdb\"",
	"-Od",
	"/bigobj",
	"library/lib/platforms/driver/winrt.cpp"
})
local arg = format('"%s"', cmd)
for _, g in ipairs(args) do
	arg = arg.." "..g
end
-- print(arg)
os.exec(arg)
