import("detect.sdks.find_vstudio")


local storeWinmds = {
	"lib\\x86\\store\\references\\platform.winmd"
}
local kitWinmds = {
	"Windows.AI.MachineLearning.MachineLearningContract\\5.0.0.0\\Windows.AI.MachineLearning.MachineLearningContract.winmd",
	"Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract\\2.0.0.0\\Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract.winmd",
	"Windows.ApplicationModel.Calls.Background.CallsBackgroundContract\\4.0.0.0\\Windows.ApplicationModel.Calls.Background.CallsBackgroundContract.winmd",
	"Windows.ApplicationModel.Calls.CallsPhoneContract\\7.0.0.0\\Windows.ApplicationModel.Calls.CallsPhoneContract.winmd",
	"Windows.ApplicationModel.Calls.CallsVoipContract\\4.0.0.0\\Windows.ApplicationModel.Calls.CallsVoipContract.winmd",
	"Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract\\2.0.0.0\\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.winmd",
	"Windows.ApplicationModel.SocialInfo.SocialInfoContract\\2.0.0.0\\Windows.ApplicationModel.SocialInfo.SocialInfoContract.winmd",
	"Windows.ApplicationModel.StartupTaskContract\\3.0.0.0\\Windows.ApplicationModel.StartupTaskContract.winmd",
	"Windows.Devices.Custom.CustomDeviceContract\\1.0.0.0\\Windows.Devices.Custom.CustomDeviceContract.winmd",
	"Windows.Devices.DevicesLowLevelContract\\3.0.0.0\\Windows.Devices.DevicesLowLevelContract.winmd",
	"Windows.Devices.Printers.PrintersContract\\1.0.0.0\\Windows.Devices.Printers.PrintersContract.winmd",
	"Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract\\3.0.0.0\\Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract.winmd",
	"Windows.Devices.SmartCards.SmartCardEmulatorContract\\6.0.0.0\\Windows.Devices.SmartCards.SmartCardEmulatorContract.winmd",
	"Windows.Foundation.FoundationContract\\4.0.0.0\\Windows.Foundation.FoundationContract.winmd",
	"Windows.Foundation.UniversalApiContract\\15.0.0.0\\Windows.Foundation.UniversalApiContract.winmd",
	"Windows.Gaming.XboxLive.StorageApiContract\\1.0.0.0\\Windows.Gaming.XboxLive.StorageApiContract.winmd",
	"Windows.Graphics.Printing3D.Printing3DContract\\4.0.0.0\\Windows.Graphics.Printing3D.Printing3DContract.winmd",
	"Windows.Networking.Connectivity.WwanContract\\2.0.0.0\\Windows.Networking.Connectivity.WwanContract.winmd",
	"Windows.Networking.Sockets.ControlChannelTriggerContract\\3.0.0.0\\Windows.Networking.Sockets.ControlChannelTriggerContract.winmd",
	"Windows.Security.Isolation.IsolatedWindowsEnvironmentContract\\4.0.0.0\\Windows.Security.Isolation.Isolatedwindowsenvironmentcontract.winmd",
	"Windows.Services.Maps.GuidanceContract\\3.0.0.0\\Windows.Services.Maps.GuidanceContract.winmd",
	"Windows.Services.Maps.LocalSearchContract\\4.0.0.0\\Windows.Services.Maps.LocalSearchContract.winmd",
	"Windows.Services.Store.StoreContract\\4.0.0.0\\Windows.Services.Store.StoreContract.winmd",
	"Windows.Services.TargetedContent.TargetedContentContract\\1.0.0.0\\Windows.Services.TargetedContent.TargetedContentContract.winmd",
	"Windows.Storage.Provider.CloudFilesContract\\7.0.0.0\\Windows.Storage.Provider.CloudFilesContract.winmd",
	"Windows.System.Profile.ProfileHardwareTokenContract\\1.0.0.0\\Windows.System.Profile.ProfileHardwareTokenContract.winmd",
	"Windows.System.Profile.ProfileRetailInfoContract\\1.0.0.0\\Windows.System.Profile.ProfileRetailInfoContract.winmd",
	"Windows.System.Profile.ProfileSharedModeContract\\2.0.0.0\\Windows.System.Profile.ProfileSharedModeContract.winmd",
	"Windows.System.Profile.SystemManufacturers.SystemManufacturersContract\\3.0.0.0\\Windows.System.Profile.SystemManufacturers.SystemManufacturersContract.winmd",
	"Windows.System.SystemManagementContract\\7.0.0.0\\Windows.System.SystemManagementContract.winmd",
	"Windows.UI.UIAutomation.UIAutomationContract\\2.0.0.0\\Windows.UI.UIAutomation.UIAutomationContract.winmd",
	"Windows.UI.ViewManagement.ViewManagementViewScalingContract\\1.0.0.0\\Windows.UI.ViewManagement.ViewManagementViewScalingContract.winmd",
	"Windows.UI.Xaml.Core.Direct.XamlDirectContract\\5.0.0.0\\Windows.UI.Xaml.Core.Direct.XamlDirectContract.winmd"
}


local vs_sdk_versions = find_vstudio()
local vs_sdk = nil;
for _,vs_sdk_archs in pairs(vs_sdk_versions) do
	vs_sdk = vs_sdk_archs["vcvarsall"]["x64"]
	break
end

local VCToolsInstallDir = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.34.31933\\"
local WindowsSdkVerBinPath = "C:\\Program Files (x86)\\Windows Kits\\10\\References\\10.0.22621.0\\"

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
	"/I../SDL-release-2.26.3/include",
	"/D _UNICODE /D UNICODE /D WINAPI_FAMILY=WINAPI_FAMILY_APP /D __WRL_NO_DEFAULT_LIB__",
}
local EXTERNAL_INCLUDE = {
	"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.34.31933\\include",
	"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.34.31933\\ATLMFC\\include",
	"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\VS\\include",
	"C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.22621.0\\ucrt",
	"C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.22621.0\\um",
	"C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.22621.0\\shared",
	"C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.22621.0\\winrt",
	"C:\\Program Files (x86)\\Windows Kits\\10\\include\\10.0.22621.0\\cppwinrt",
	"C:\\Program Files (x86)\\Windows Kits\\NETFXSDK\\4.8\\include\\um",
}

for _, ii in ipairs(EXTERNAL_INCLUDE) do
	table.insert(args, format("/I\"%s\"", ii))
end

for _, p in ipairs(storeWinmds) do
	table.insert(args, format("/FU\"%s%s\"", VCToolsInstallDir, p))
end
for _, p in ipairs(kitWinmds) do
	table.insert(args, format("/FU\"%s%s\"", WindowsSdkVerBinPath, p))
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

os.exec(arg)
