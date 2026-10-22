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
