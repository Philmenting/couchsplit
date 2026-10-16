; SPDX-License-Identifier: GPL-3.0-or-later
[Setup]
AppId=PlatinumLocalCoop
AppName=Pokémon Platinum Local Co-op
AppVersion=0.2.0-alpha.5
VersionInfoVersion=0.2.0.4
DefaultDirName={localappdata}\Programs\Pokemon-Platinum-Coop
DefaultGroupName=Pokémon Platinum Local Co-op
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=Pokemon-Platinum-Coop-Windows-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
LicenseFile=..\LICENSE
UninstallDisplayIcon={app}\project\build\bin\platinum-coop.exe
[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Files]
Source: "..\dist\windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Pokémon Platinum Local Co-op"; Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; WorkingDir: "{app}"
Name: "{userdesktop}\Pokémon Platinum Local Co-op"; Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; WorkingDir: "{app}"
[Run]
Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; Description: "Launcher starten und ROM hinzufügen"; Flags: nowait postinstall skipifsilent
