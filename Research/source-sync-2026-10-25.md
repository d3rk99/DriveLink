# Source preservation — 25 October 2026

Both DriveLink and DriveLink Hub use `https://github.com/d3rk99/DriveLink`, branch `main`. Hub 0.4.9-rc1 was already pushed in commit `ce71f5c58407c16785adb913572edc192b6994c8` and distributed as a GitHub prerelease. This synchronization preserves the previously local core arrival/staging changes, final companion addon and Windows helper implementation.

## Preserved source

- Core mod: current `Addons/DriveLink` and `Mod/mod.cpp` (0.4.9-rc1).
- Companion addon: exact source snapshot used for the 0.4.9-rc1 candidate, now at `Addons/DriveLinkRestart`.
- Windows helper/agent/setup/updater/uninstaller: copied without behavior changes into `Companion`, helper version 0.4.8-rc3.
- Native policy assertions and isolated updater harness, plus the existing arrival and Namalsk research notes.
- Build scripts now include all three PBOs, reference the repository's signing script and can compile the Windows helper without the former external prototype source tree.

## Checks

All copied release addon and Windows helper source hashes matched their original candidate implementation. All five Windows executables compiled from the new source location; the generated helper manifest contains 263 entries. The isolated updater harness passed 21 checks. All three PBOs built and their Derk signatures verified. No game, server, Hub or installed helper was launched. This does not add a new live-transfer validation claim.

## Keep separate backups

Private signing keys, runtime Hub/player data, certificates, test saves, generated build directories and artwork source archives are intentionally excluded. GitHub source alone is not a backup of those items. Helper artwork can be supplied from an existing distributed mod's Helper/Assets directory. Preserve private keys and any original artwork permissions/source archives independently before spring cleaning.

The existing 0.4.9-rc1 release/tag is not moved or republished by this source synchronization. Workshop publication remains manual.
