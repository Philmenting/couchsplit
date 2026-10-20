; SPDX-License-Identifier: GPL-3.0-or-later
[Setup]
AppId=PlatinumLocalCoop
AppName=CouchSplit
AppVersion=0.2.0-alpha.8
VersionInfoVersion=0.2.0.8
DefaultDirName={localappdata}\Programs\Pokemon-Platinum-Coop
DefaultGroupName=CouchSplit
UsePreviousGroup=no
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=CouchSplit-Windows-Setup
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
; Preserve AppId and DefaultDirName so Alpha 7 and earlier upgrade in place.
[InstallDelete]
Type: files; Name: "{userprograms}\Pokémon Platinum Local Co-op\Pokémon Platinum Local Co-op.lnk"
Type: files; Name: "{userdesktop}\Pokémon Platinum Local Co-op.lnk"
Type: dirifempty; Name: "{userprograms}\Pokémon Platinum Local Co-op"
[Files]
Source: "..\dist\windows\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\CouchSplit"; Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; WorkingDir: "{app}"
Name: "{userdesktop}\CouchSplit"; Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; WorkingDir: "{app}"
[Run]
Filename: "{app}\project\build\bin\platinum-coop.exe"; Parameters: "--fullscreen --first-run"; Description: "Launcher starten und ROM hinzufügen"; Flags: nowait postinstall skipifsilent
