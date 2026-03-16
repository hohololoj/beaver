[Setup]
AppName=beaver
AppVersion=1.0
ArchitecturesInstallIn64BitMode=x64
DefaultDirName={autopf}\beaver
OutputBaseFilename=beaver-setup
SetupIconFile=logo.ico
Compression=zip
SolidCompression=yes

[Files]
Source: "build/*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Registry]
Root: HKCR; Subkey: "Directory\Background\shell\Поиск в beaver"; Flags: uninsdeletekey
Root: HKCR; Subkey: "Directory\Background\shell\Поиск в beaver"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\beaver-app.exe,0"
Root: HKCR; Subkey: "Directory\Background\shell\Поиск в beaver\command"; ValueType: string; ValueName: ""; ValueData: """{app}\beaver-app.exe"" --startDir ""%V"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: "Directory\Background\shell\Поиск в beaver"; ValueType: string; ValueName: "HasLUAShield"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCR; Subkey: "Directory\Background\shell\Поиск в beaver\command"; ValueType: string; ValueName: "IsolatedCommand"; ValueData: """{app}\beaver-app.exe"" --startDir ""%V"""; Flags: uninsdeletevalue