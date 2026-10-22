# Transition film experiment

Branch: experiment/transition-film, based on early 0.4 release build 97da69a.
Main and Workshop builds have not been modified by this experiment.

The plain connecting scene uses the departing party's cast and vehicle sprites.
It starts only after a successful Hub handover and contains no predicted road
events. The existing authoritative film remains on the destination server.

## First user transfer test

Observed: no film in menu/login countdown, and arrival showed text on black.
Client log: script_2026-10-22_03-32-54.log, prepared/started transition at 03:37:23,
but no transition update or destination lifecycle messages followed.
Server log: 03:38:02 CallLater failed to invoke DL_SendFilm because its new third
argument was omitted by the delayed calls. Default arguments do not apply to
this dynamic dispatch. This explains the missing arrival film.

## Second prototype

- Restore two-argument DL_SendFilm callback; route typed RPC sends through a
  separate three-argument DL_SendFilmRpc helper.
- Retain an explicit active session flag independently of native widget lifetime.
- Drive transition updates through the system update queue, which vanilla calls
  even when loading suspends the GUI queue.
- Detach native views on mission finish and rebuild on mission start or workspace
  changes. Rebuild the cosmetic loop instead of moving native widgets between
  workspaces. Continuous scene position across rebuilds remains future work.
- Use solid widget colors directly instead of trying to load procedural material
  expressions as image files.

Packed/signed to C:\DayZMods\FilmPrototypeRuntime\@DriveLinkFilmPrototype.
Both test servers compiled the callback/lifecycle fixes. The user's next transfer
confirmed main-menu and loading-screen animation, with the timer gap noted below.

## Manual test

## Accepted behavior and cleanup

The user confirmed a successful full transfer with menu/loading/timer coverage.
The intended behavior is a cosmetic loading loop with the right vehicles,
characters, and convoy while disconnected/loading; actual journey events begin
only after the local client has loaded, during the destination waiting-room flow.
No story outcomes are sent in the connecting preview.

Cleanup after that test:

- Queue the completed authoritative film on the client until the game mission is
  active, world loading has ended, and the login timer is no longer visible.
  Keep the arrival black screen while a story is queued; respect skip messages.
- Consume each prepared cast once. Clear stale data and partial chunks on stop,
  reject empty chunks, and clear a preview on unusable-address/non-trip redirects.
- Start the short arrival fail-safe clock only after the local client is ready.
- Dismiss a generic overlay cleanly when the waiting room ends without a film.
- Avoid allocating a transition manager when a login menu is destroyed, and
  reduce repeated heartbeat logging to detailed logs.

These changes preserve the 3-second reconnect settle time, the server's login
timer, and the existing server-side arrival/crew logic. Client presentation is
deferred without changing authoritative travel outcomes.

### Continuous scene refinement after the successful user transfer

The 14:22 client log confirms playback through the main menu and world loading,
but also shows repeated view rebuilding at mission/workspace changes. During the
15-second login timer IsLoading stayed true while the timer itself rendered on
the normal workspace, leaving the animation on the wrong workspace.

The next patch hooks LoginTimeBase.SetTime/Show/Hide without changing the server's
timer or native cancellation. It moves the existing scene to the normal workspace
for that timer and displays the remaining seconds in the overlay. It also keeps
the existing native tree through mission changes and carries its scroll/time/speed
if native widgets genuinely need recovery. For matching ordinary road casts, the
authoritative arrival film adopts the same player/sprites rather than replaying
the entry animation. Boats, aircraft, via-biome changes, and cast changes retain
the existing arrival playback as fallback. Destination resend deduplication is
preserved for the adopted player.

The current patched package uses the ORIGINAL @DriveLinkFilmPrototype folder.
The separate -v2 package is obsolete. Future tests should use the original folder
and C:\DayZMods\FilmPrototypeRuntime\Restart-Prototype-Servers.cmd.

Deployment of the continuous scene refinement: both test servers restarted with
the original prototype folder, compiled successfully (Mission CRC 828f7e58),
and completed Hub handshakes. No client was launched. Visual coverage of the login
timer and smooth handoff must be checked in the next user-driven transfer.

1. Leave the test server and exit DayZ.
2. Run C:\DayZMods\FilmPrototypeRuntime\Restart-Prototype-Servers.cmd if needed.
3. Disable Workshop DriveLink and any separate -v2 local copy. Load the original prototype folder,
   plus CF and VPPAdminTools, and join either test server.
4. Travel once in each direction. Check logout, menu, loading screen, login timer,
   arrival film, and final reveal separately. Record any stalled overlay or error.
5. Inspect client/server logs for "transition prototype", system-queue update
   messages, and any Virtual Machine Exception. In particular verify DL_SendFilm
   no longer reports a missing third argument.

Escape, a dialog, 120-second total timeout, or a 12-second arrival timeout dismisses
the cosmetic overlay. Successful authoritative film delivery dismisses it too.

Rollback: restart servers using C:\DayZMods\DemoServers\start-demo-servers.ps1
-Restart, and re-enable Workshop DriveLink in the client launcher.

### Arrival cleanup and Zen Immersive Login compatibility

The user confirmed continuous playback, but reported a dim frozen scene with an
offset vehicle after arrival. The arrival story now stops the cosmetic scene and
creates its own presentation instead of adopting the loading scene's widgets.
This supersedes the sprite-adoption behavior described above. Visual confirmation
of this fix still requires a user-driven transfer.

DriveLink_ZenLogin is always packaged and depends only on DriveLink. It detects
CfgPatches ZenImmersiveLogin at runtime and does nothing when it is absent. No Zen
class inheritance or Zen-specific method overrides are compiled into the bridge.
The build script always packs and signs both addons; -ZenCompat remains accepted
only for older commands. No PBO removal or package adjustment is needed when
switching between servers with and without Zen. Workshop Zen files are unchanged.

The bridge marks rebuilt living transfer arrivals and invokes Zen's public
completion method dynamically. On transfer logins it finds Zen's client effect
manager, removes its queued blackout/control callbacks, calls Cleanup, and deletes
the manager. A system update callback covers loading; a mission callback is also
available. Ordinary logins leave the manager untouched. A client transfer marker
covers the interval before replicated state arrives and clears on ordinary logout.

Client test mod list: CF, VPPAdminTools, Zen's Core Mod, Zen's Immersive Login,
and C:\DayZMods\FilmPrototypeRuntime\@DriveLinkFilmPrototype. Disable Workshop
DriveLink and the obsolete -v2 local copy. The bridge PBO is inside the same
original prototype folder; there is no additional local folder to select.

Compatibility checks:

1. Ordinary login: Zen's wake-up, fade, and controls behave normally.
2. DriveLink transfer on foot, by vehicle, and in a convoy: generic loading film,
   then arrival story; no frozen offset scene or Zen wake-up/blackout/control lock.
3. Ordinary logout and rejoin after a transfer: Zen's wake-up runs again.
4. Check passengers and travel-death respawns separately; ordinary fresh spawns
   are not marked as living transfer arrivals.

Two compilation issues were corrected during deployment: Zen overrides were moved
into the ordered optional addon, and the server arrival marker was moved after
successful character creation in the waiting-room path. Packaging alone does not
verify Enforce compilation; inspect both test-server startup logs after deploying.

The ordered addon approach above was subsequently replaced by runtime discovery
to eliminate the hard dependency. Several configuration-order approaches were
rejected after actual engine tests; the bridge now has no Zen compile-time types.
On 2026-10-22 at 15:13 the identical final package completed mission startup on
Chernarus without either Zen mod (Mission CRC 7a159769), and Livonia with both Zen
mods (Mission CRC afa06dc9), without script errors. It was built without -ZenCompat
and still contained both signed PBOs. Client wake-up suppression needs a new
user-driven transfer test because the integration mechanism has changed.

### Wake-up / vehicle-seating regression and first-gear convenience

The user's next vehicle transfer left them sitting beside the restored Offroad_02.
The client session script_2026-10-22_15-28-46.log shows the destination Zen manager
created at 15:31:19.833 and destroyed normally at 15:31:31.256, with no DriveLink
"skipped wake-up" message. The runtime cleanup path had not intercepted the wake-up.
The earlier integration intercepted Zen's gesture request directly; the optional
rewrite removed that protection and relied on cleanup polling instead.

The new bridge overrides the VANILLA EmoteManager.CreateEmoteCBFromMenu entry point
and rejects Zen's sit/lie requests while the client transfer marker is active.
This guard does not depend on Zen class inheritance or its compile order, and is
inactive without Zen's actual compiled config type. Cleanup now detects compiled
Zen types at mission start, registers its system callback there, reads the final
mission instance, and uses the returned manager reference directly. It also logs
whether the manager type is available. The transfer marker clears on WAIT_DONE,
after server seating is complete, so later ordinary emotes/respawns are unaffected.

Core DriveLink now shifts an arrived car up from neutral once the local driver is
seated and its engine is running. It handles an already-running engine as well.
WAIT_DONE is resent by the server, so a per-mission completion flag prevents the
resend from restarting or shifting a car after the driver has driven away. Engine
start is retried within the existing bounded attempt window if it has not succeeded.
This does not synthesize keyboard input.

At 15:37 on 2026-10-22 the identical signed package compiled and started Chernarus
without Zen (Mission CRC 10d67ef7) and Livonia with Zen (Mission CRC b1a2b1e4).
The original prototype folder is updated for another user-driven vehicle transfer.
Verify automatic vehicle seating, absence of sit/lie and blackout, first forward
gear after engine start, no second shift on the WAIT_DONE resend, and ordinary
Zen login/emotes after a normal reconnect. Server startup alone cannot verify
client gesture suppression, seating, or gearbox behavior in a live transfer.

Deployment verified 2026-10-22 14:58: both original test instances compiled World
(CRC 623bac50) and Mission (CRC a2ea9caa), started their missions, and completed
Hub handshakes. Neither fresh script log contained script errors or VM exceptions.
Both deployed PBOs and signatures matched the staged files. No client was launched.
