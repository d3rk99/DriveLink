# M3 - whole crew travels (driver + passengers in their seats)

Needs TWO players (a second Steam account / second PC on the LAN). Car: the debug.json test car on Chernarus.

## How it works (what to look for)
- Only the DRIVER's countdown counts; everyone in the car sees it. Leaving the zone cancels it for all.
- Source sends the passengers' trips first, the driver's (with the car) only when the hub took all of them.
  Chernarus logs: `<driver> drives OffroadHatchback to livonia with 1 passenger(s)`, then
  `<passenger> leaving ... passenger in seat 1 of <driver>'s car`, then `<driver> leaving ... driving ...`.
- Target: the car is built when the DRIVER arrives. A passenger who loads first logs
  `passenger: waiting for <driver>'s car` and waits on foot 6 m right of the arrival point; when the car comes:
  `<driver>'s car is here, bringing in 1 waiting passenger(s)` and he is moved next to it and seated.
  A passenger who loads after the car: `passenger: <driver>'s OffroadHatchback is here, seat N` -> seated.

## Steps
1. Player A drives, player B sits in the co-driver seat (seat 1). Drive into "To Livonia", stop.
2. Both arrive on Livonia in the SAME car, A at the wheel, B in seat 1. Check gear of both, car parts, fuel.
3. Drive back with B in a rear seat (seat 2/3).
4. Refusal: B leaves the car during the countdown -> countdown cancelled for B only? (B is on foot in the zone ->
   he gets his own countdown and travels on foot; A's trip goes on alone).
5. Hub stopped: A drives in with B -> "Travel failed" for B, "Car trip called off" for A, both stay, car stays.

## Report
Pass/fail per step, every SCRIPT (E) line on both servers, what each player saw.

## Waiting room solo run 2026-10-19 02:13 (120346-byte build)
Chernarus -> Livonia, driver alone (party of 1). Server: built into the room 1/1 (36/36), car built after (29/29, fluids exact),
hub confirm only after the car existed, seated, "1/1 seated, black screen lifted". Client log: black screen on 02:13:20.5
("Everyone is here. Getting in..."), off 02:13:26.0, "engine started on this client, running true"; server 4 s later: running true.
Not tested: the n/m counter with a real crew, the crewWaitSeconds timeout, passengers released when no driver comes.
