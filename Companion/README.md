# Windows travel companion source

This directory preserves the current Windows helper, tray agent, setup, updater and uninstaller source. The previous working location was `C:\DayZMods\DriveLinkRestartPrototype`; building the current source no longer requires that project.

- `Helper/`: launch/transfer protocol, travel cover, renderer and setup support.
- `TrayTravel/`: agent, startup registration, release setup, updater and uninstaller.
- `../Addons/DriveLinkRestart/`: final release addon source, including the merged client script and 0.4.9 automatic Hub-policy handling. It is the release source rather than the old lab templates; browser activation and lab auto-travel hooks are absent.
- `Tests/verify-helper-updates.ps1`: isolated updater acceptance harness. It writes under the ignored `Build/` directory. Read the harness before running it.

## Build the helper

Use Windows with .NET Framework 4's C# compiler. Supply `Assets` from the existing distributed DriveLink mod's `Helper` directory; artwork is intentionally not duplicated in source control. Preserve that release package or obtain the assets from Workshop before deleting local artwork sources.

```powershell
.\Companion\build.ps1 -AssetsPath 'C:\path\to\@DriveLink\Helper\Assets'
```

Output defaults to `Companion/dist/Helper`. An existing output directory is rejected. This command compiles files only: it does not install the agent, register Windows startup, launch DayZ, or start a server. Place the resulting Helper folder alongside the mod's Addons directory when assembling a release, and supply the network manifests for your deployment. Compilation is not a complete installer or live-transfer test.

The helper version remains 0.4.8-rc3; the mod and Hub are 0.4.9-rc1. Version differences here are deliberate. Source files were preserved from the candidate's active implementation without changing game behavior.

## Build the PBOs

`../Tools/build-mod.ps1` packs core DriveLink, DriveLink_ZenLogin and DriveLinkRestart using DayZ Tools, then signs and verifies all PBOs with the required external Derk key. The private key is not stored in this repository. Existing local DayZ Tools paths remain configured in the build scripts. Core artwork prerequisites are separate from the Windows helper assets.

Build products, player settings, network manifests, Hub credentials, databases, certificates and private keys are not included in this source snapshot. Historical native-engine logs and prototype investigation folders remain outside the repository.
