# Native automatic-policy assertions

`PolicyTest.c` preserves the native DayZ assertions used during 0.4.9 candidate preparation. It is a test-only MissionServer extension, not part of the distributed DriveLink addon.

To use it, put it in a separate server-only test addon declaring a Mission script module and a `requiredAddons[]={"DriveLinkRestart"}` dependency. Use a disposable mission/profile with a configured DriveLink MissionServer, no legacy `DriveLinkRestart/routes.json`, and the matching core/companion PBOs. Keep it off production servers: it deliberately applies fixture Hub replies and manipulates test policy state. It runs after mission startup and logs `[DL049 TEST PASS]` or `[DL049 TEST FAIL]`; the startup no-legacy-file/unknown-route guards are separate from the 22 policy assertions recorded in candidate validation.

The original local bootstrap depended on historical test profiles, so only the self-contained assertion source is preserved here. It is not launched by the normal mod build. Its prior native run passed during candidate preparation; this source synchronization did not launch a DayZ server or repeat a live transfer test.
