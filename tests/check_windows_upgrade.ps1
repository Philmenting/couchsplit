# SPDX-License-Identifier: GPL-3.0-or-later
param([Parameter(Mandatory=$true)][string]$SetupPath)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true' -or !$IsWindows) { throw 'Run this installation check only on the disposable Windows CI runner.' }
$testRoot = Join-Path $env:RUNNER_TEMP 'CouchSplit Upgrade Test'
$installDir = Join-Path $testRoot 'Application'
$profileDir = Join-Path $testRoot 'User Data'
New-Item -ItemType Directory -Force -Path $testRoot | Out-Null
$legacy = Join-Path $testRoot 'Alpha7-Setup.exe'
Invoke-WebRequest -Uri 'https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-Platinum-Coop-Windows-Setup.exe' -OutFile $legacy
if ((Get-FileHash -Algorithm SHA256 $legacy).Hash -ne '82ffc66eb0efa963b2ef10354303e615c419c3bd2fe8c1f2371ecd8cab0fff48') { throw 'Unexpected Alpha 7 installer hash' }
function Install-CheckedSetup([string]$Path) {
    $setup = Start-Process -FilePath $Path -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/DIR=`"$installDir`"") -Wait -PassThru
    if ($setup.ExitCode -ne 0) { throw "Setup failed: $($setup.ExitCode)" }
}
Install-CheckedSetup $legacy
$desktop = [Environment]::GetFolderPath([Environment+SpecialFolder]::Desktop)
$programs = [Environment]::GetFolderPath([Environment+SpecialFolder]::Programs)
$oldDesktopLink = Join-Path $desktop 'Pokémon Platinum Local Co-op.lnk'
$oldMenuLink = Join-Path $programs 'Pokémon Platinum Local Co-op/Pokémon Platinum Local Co-op.lnk'
if (!(Test-Path $oldDesktopLink) -or !(Test-Path $oldMenuLink)) { throw 'Missing old application shortcuts before upgrade' }
$save = Join-Path $profileDir 'players/1/TEST_ONLY/saves/game.sav'
New-Item -ItemType Directory -Force -Path (Split-Path $save) | Out-Null
[IO.File]::WriteAllText($save, "TEST_ONLY owned upgrade sentinel`n")
$settings = Join-Path $profileDir 'settings.json'
[IO.File]::WriteAllText($settings, '{"language":"de","layout":2,"futureSetting":{"preserve":true}}')
$saveHash = (Get-FileHash $save).Hash
$settingsHash = (Get-FileHash $settings).Hash
Install-CheckedSetup (Resolve-Path $SetupPath).Path
$entry = Get-ItemProperty 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/PlatinumLocalCoop_is1'
if ($entry.DisplayName -ne 'CouchSplit' -or $entry.DisplayVersion -ne '0.2.0-alpha.8') { throw 'Upgrade did not reuse the existing installer identity' }
$newDesktopLink = Join-Path $desktop 'CouchSplit.lnk'
$newMenuLink = Join-Path $programs 'CouchSplit/CouchSplit.lnk'
if (!(Test-Path $newDesktopLink) -or !(Test-Path $newMenuLink)) { throw 'Missing CouchSplit shortcuts' }
if ((Test-Path $oldDesktopLink) -or (Test-Path $oldMenuLink)) { throw 'Obsolete known application shortcuts remain' }
$env:QT_QPA_PLATFORM = 'offscreen'
$env:SDL_AUDIODRIVER = 'dummy'
$env:QT_LOGGING_TO_CONSOLE = '1'
$capture = Join-Path $testRoot 'launcher.png'
$smoke = Start-Process -FilePath "$installDir/project/build/bin/platinum-coop.exe" -ArgumentList @('--data-dir', "`"$profileDir`"", '--smoke-test', "`"$capture`"") -Wait -PassThru
if ($smoke.ExitCode -ne 0 -or !(Test-Path $capture)) { throw "Installed launcher failed: $($smoke.ExitCode)" }
if ((Get-FileHash $save).Hash -ne $saveHash -or (Get-FileHash $settings).Hash -ne $settingsHash) { throw 'Upgrade or launch changed existing profile data' }
$uninstall = Start-Process -FilePath "$installDir/unins000.exe" -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait -PassThru
if ($uninstall.ExitCode -ne 0 -or (Test-Path "$installDir/project/build/bin/platinum-coop.exe")) { throw 'Uninstall failed' }
if ((Get-FileHash $save).Hash -ne $saveHash -or (Get-FileHash $settings).Hash -ne $settingsHash) { throw 'Uninstall changed personal data' }
if ((Test-Path $newDesktopLink) -or (Test-Path $newMenuLink)) { throw 'Uninstall left owned shortcuts behind' }
Write-Output 'PASS: real Alpha 7 -> CouchSplit Windows upgrade, same AppId, native launcher, new shortcuts, retained settings/save, uninstall preserves data'
