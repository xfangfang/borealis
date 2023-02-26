os.exec('"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.22621.0\\x86\\makepri.exe" createconfig -Overwrite /cf build/priconfig.xml /dq en-US')
os.exec('"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.22621.0\\x86\\makepri.exe" new -Overwrite /pr winrt /cf build/priconfig.xml -OutputFile build')
os.exec('"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.22621.0\\x86\\makeappx.exe" pack /l /h SHA256 /f winrt/main.map.txt /m winrt/AppxManifest.xml /o /p build/demo.msix')
os.exec('"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.22621.0\\x86\\signtool.exe" sign /fd SHA256 /a /f .\\winrt\\TemporaryKey.pfx build\\demo.msix')
