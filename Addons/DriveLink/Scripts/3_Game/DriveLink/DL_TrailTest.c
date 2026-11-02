// DriveLink dev test (-dlTrailTest, DL_DEV builds only): the interactive trail played in the main menu with no
// server, the client standing in for it - made-up voters, the default deck, the same rules (DL_TrailGame), chapters
// (DL_TrailChapters), film (DL_FilmPlayer) and card (DL_TrailView) as a real trip. For unattended screenshots.
#ifdef DL_DEV
class DL_TrailTest
{
	static const float VOTE_SECONDS = 12;
	ref DL_FilmPlayer film = new DL_FilmPlayer();
	ref DL_TrailView view = new DL_TrailView();
	ref DL_TrailGame game;
	ref DL_TrailState st;
	ref array<ref array<ref DL_FilmEvent>> chapters;
	int scenario;
	int chap;
	int choices;
	int phase;                     // 0 road, 1 vote, 2 outcome, 3 last road, 4 summary, 5 done
	float t;
	ref DL_TrailVoteCard card;
	ref DL_TrailTally tally;
	ref array<float> voteAt = new array<float>;
	ref array<int> voteFor = new array<int>;
	float tripKm;
	float kmCounted;               // detours already added to the road
	// -dlTrailCards=roadblock:1:0,deer:0:0,... : the cards in this order, the shown choice everyone votes for and the
	// outcome it gets (for screenshots of every card scene); used up across both trips
	static ref array<string> s_Plan;
	static int s_PlanI;
	int forcePick = -1;

	bool Done()
	{
		return phase == 5;
	}

	protected DL_TrailMember Member(DL_Film f, string name, int car, string items)
	{
		DL_TrailMember m = new DL_TrailMember();
		m.sid = name;
		m.name = name;
		m.car = car;
		m.fake = true;
		items.Split(",", m.items);
		st.members.Insert(m);
		return m;
	}

	void Start(int n)
	{
		scenario = n;
		DL_Film f = new DL_Film();
		f.skip = "trail";
		st = new DL_TrailState();
		if (n == 0)
		{
			// a sedan in the forest: Derk carries a compass and epoxy putty, Orion a rifle and matches
			f.biomeFrom = "forest";
			f.biomeTo = "field";
			f.car = "sedan@wine";
			f.cars.Insert("sedan@wine");
			f.crew.Insert(DL_FilmPlayer.DemoActor("Derk", "def1", "workpants@brown,tshirt@orange,taloonbackpack@green,baseballcap@red", "glockpistol", "crowbar", 0));
			f.crew.Insert(DL_FilmPlayer.DemoActor("Orion", "redneck", "jeans@blue,hoodiegray@green,mountainbackpack,cowboyhat", "", "mosinnagant", 0));
			st.biome = "forest";
			Member(f, "Derk", 0, "Compass,EpoxyPutty,BandageDressing,Glock19");
			Member(f, "Orion", 0, "Mosin9130,Matchbox,HuntingKnife,FishingRod,Shovel");
			DL_TrailCar c = new DL_TrailCar();
			c.idx = 0;
			c.name = "the Olga 24";
			st.cars.Insert(c);
			tripKm = 80;
			f.Add("drive", 20);
			f.Add("find", 20, "Orion", "water");
			f.Add("drive", 25);
			f.Add("flat", 45, "Derk", "spare");
			f.Add("drive", 35);
		}
		else
		{
			// two cars in the snow, three people: Mika has a fishing rod, nobody has a fire
			f.biomeFrom = "snow";
			f.biomeTo = "snow";
			f.cars.Insert("4x4@green");
			f.cars.Insert("sedan@wine");
			f.crew.Insert(DL_FilmPlayer.DemoActor("Derk", "veteran", "orelpants,oreljacket,razgruz,hunterbackpack,helmetarmy", "akmrifle", "", 0));
			f.crew.Insert(DL_FilmPlayer.DemoActor("Britney", "jasmine", "jeans@blue,raincoatblue,schoolbackpack@red,beretred", "", "baseballbat", 0));
			f.crew.Insert(DL_FilmPlayer.DemoActor("Mika", "def3", "workpants,sweater@green,improvisedbag,ushanka", "glockpistol", "", 1));
			st.biome = "snow";
			Member(f, "Derk", 0, "AKM,Rope,GPSReceiver");
			Member(f, "Britney", 0, "BaseballBat,Rag");
			Member(f, "Mika", 1, "FishingRod,Glock19,DuctTape");
			DL_TrailCar c0 = new DL_TrailCar();
			c0.idx = 0;
			c0.name = "Derk's Hunter";
			st.cars.Insert(c0);
			DL_TrailCar c1 = new DL_TrailCar();
			c1.idx = 1;
			c1.name = "Mika's Olga 24";
			st.cars.Insert(c1);
			tripKm = 60;
			f.Add("drive", 30);
			f.Add("find", 30, "Mika", "food");
			f.Add("drive", 30);
		}
		f.km = tripKm;
		st.km = tripKm;
		game = new DL_TrailGame(st, DL_TrailDeck.Default());
		choices = DL_TrailGame.Choices(tripKm, 5);
		chapters = DL_TrailChapters.Split(f, choices);
		chap = 0;
		f.events = chapters[0];
		DL_Log.Info("trail test " + n.ToString() + ": " + choices.ToString() + " choices, " + chapters.Count().ToString() + " chapters");
		film.Play(f);
		phase = 0;
		t = 0;
	}

	void Update(float dt)
	{
		t += dt;
		film.Update(dt);
		view.Update(dt);
		if (phase == 0 && film.Waiting())
			StartVote();
		else if (phase == 1)
			Vote(dt);
		else if (phase == 2 && t > 12)
			NextChapter();
		else if (phase == 3 && !film.IsPlaying())
		{
			DL_TrailSummary s = game.Summary(false, Math.Round(tripKm + st.extraKm) * 90 + st.minutes * 60);
			view.ShowSummary(s);
			DL_Log.Info("trail test: summary " + s.title + ", " + s.lines.Count().ToString() + " lines");
			phase = 4;
			t = 0;
		}
		else if (phase == 4 && t > 6)
		{
			view.Close();
			phase = 5;
		}
	}

	protected void StartVote()
	{
		if (!s_Plan)
		{
			s_Plan = new array<string>;
			string plan;
			if (g_Game.CommandlineGetParam("dlTrailCards", plan))
				plan.Split(",", s_Plan);
		}
		forcePick = -1;
		game.force = "";
		game.forceOutcome = -1;
		if (s_PlanI < s_Plan.Count())
		{
			array<string> pp = new array<string>;
			s_Plan[s_PlanI].Split(":", pp);
			s_PlanI++;
			game.force = pp[0];
			if (pp.Count() > 1)
				forcePick = pp[1].ToInt();
			if (pp.Count() > 2)
				game.forceOutcome = pp[2].ToInt();
		}
		if (!game.Draw())
		{
			DL_Log.Info("trail test: no card fits, on to the next chapter");
			NextChapter();
			return;
		}
		card = game.Show(chap + 1, choices, VOTE_SECONDS);
		tally = new DL_TrailTally();
		voteAt.Clear();
		voteFor.Clear();
		for (int i = 0; i < card.names.Count(); i++)
		{
			tally.picks.Insert(DL_TrailGame.NONE);
			tally.locked.Insert(false);
			voteAt.Insert(Math.RandomFloat(1.5, VOTE_SECONDS * 0.6));
			int pick = Math.RandomInt(0, card.choices.Count());
			if (scenario == 1 && chap == 1 && i == 2)
				pick = DL_TrailGame.TURN_BACK;   // one of three wants to go back: short of a majority, the trip goes on
			if (forcePick >= 0 && forcePick < card.choices.Count())
				pick = forcePick;
			voteFor.Insert(pick);
		}
		tally.left = VOTE_SECONDS;
		view.ShowCard(card, 0);
		DL_FilmEvent se = game.SceneEvent(film.Km());
		if (se)
		{
			array<ref DL_FilmEvent> sevs = new array<ref DL_FilmEvent>;
			sevs.Insert(se);
			film.Append(sevs, 0);
		}
		view.UpdateTally(tally);
		DL_Log.Info("trail test: card " + game.card.id + " (" + card.choices.Count().ToString() + " choices)");
		phase = 1;
		t = 0;
	}

	protected void Vote(float dt)
	{
		tally.left = Math.Max(0, tally.left - dt);
		bool all = true;
		for (int i = 0; i < voteAt.Count(); i++)
		{
			if (t >= voteAt[i] && tally.picks[i] == DL_TrailGame.NONE)
				tally.picks[i] = voteFor[i];
			if (t >= voteAt[i] + 2.5 && tally.picks[i] != DL_TrailGame.NONE)
				tally.locked[i] = true;
			if (!tally.locked[i])
				all = false;
		}
		if (!all && tally.left > 0)
		{
			view.UpdateTally(tally);
			return;
		}
		int win = game.Winner(card, tally);
		tally.result = win;
		if (win == DL_TrailGame.TURN_BACK)
		{
			tally.outcome = "The party turns back.";
			view.UpdateTally(tally);
			DL_Log.Info("trail test: turned back");
			film.SetHold(false);
			phase = 3;
			return;
		}
		array<ref DL_FilmEvent> evs = new array<ref DL_FilmEvent>;
		tally.outcome = game.Resolve(win, film.Km(), evs);
		view.UpdateTally(tally);
		film.Append(evs, 0);
		DL_Log.Info("trail test: chose " + card.choices[win].label + ": " + tally.outcome);
		phase = 2;
		t = 0;
	}

	protected void NextChapter()
	{
		view.Close();
		chap++;
		if (chap >= chapters.Count())
		{
			film.SetHold(false);
			phase = 3;
			return;
		}
		array<ref DL_FilmEvent> next = chapters[chap];
		DL_TrailChapters.Fix(next, st, st.extraKm - kmCounted);
		kmCounted = st.extraKm;
		film.Append(next, tripKm + st.extraKm);
		if (chap == chapters.Count() - 1)
		{
			film.SetHold(false);
			phase = 3;
			return;
		}
		phase = 0;
		t = 0;
	}
}
#endif
