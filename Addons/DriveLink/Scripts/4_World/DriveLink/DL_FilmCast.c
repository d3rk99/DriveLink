// DriveLink road film - dresses a traveller's film survivor from his real DayZ gear (his snapshot): the item in each
// clothing slot is matched by class-name keywords to the closest MiniDayZ layer (DL_FilmCatalog). Female characters
// (SurvivorF_*) get a female base skin; the skin is fixed per player (picked from the steam id). Clothing in a plain
// colour (Hoodie_Blue, MotoHelmet_Red...) is drawn as the layer's grey copy tinted that colour. The weapon in the hands
// is drawn in the hands, a rifle on the shoulder or a melee weapon in the melee slot is slung on the back.

class DL_FilmCast
{
	static const string FEMALE = ",aika,jasmine,kate,";

	// "keyword=layer;..." (keyword lower case, in the item's class name; "" = anything): first match wins, so the
	// specific words come first
	static string Match(string type, string table)
	{
		string t = type;
		t.ToLower();
		array<string> pairs = new array<string>;
		table.Split(";", pairs);
		foreach (string pair : pairs)
		{
			int eq = pair.IndexOf("=");
			if (eq < 0)
				continue;
			string key = pair.Substring(0, eq);
			if (key == "" || t.Contains(key))
				return pair.Substring(eq + 1, pair.Length() - eq - 1);
		}
		return "";
	}

	static const string BODY = "gorka=gorkajacket;hunter=hunterjacket;down=downjacket;puffer=downjacket;paramedic=paramedicjacket;medic=paramedicjacket;bomber=raiderjacket;ttsko=oreljacket;orel=oreljacket;m65=oreljacket;bdu=oreljacket;usmc=oreljacket;police=oreljacket;firefighter=paramedicjacket;track=sportjacket;windbreaker=sportjacket;quilted=downjacket;hoodie_red=hoodiered;hoodie=hoodiegray;raincoat=raincoatblue;sweater=sweater;ghillie=cloakbrown;dress=dress;tshirt=tshirt;shirt=shirtgray;jacket=hunterjacket;=shirtgreen";
	static const string LEGS = "gorka=gorkapants;hunter=hunterpants;camo=hunterpants;ttsko=orelpants;bdu=orelpants;usmc=orelpants;paramedic=paramedicpants;track=sportpants;sport=sportpants;jeans=jeans;cargo=workpants;=workpants";
	static const string HEAD = "nvg=helmetnvg;moto=helmetmoto;construction=helmethard;hard=helmethard;gorka=gorkahelmet;helmet=helmetarmy;cowboy=cowboyhat;police=policehat;beanie=warmhat;knit=warmhat;ushanka=ushanka;beret=beretred;bandana=bandana;balaclava=balaklava;headtorch=headlamp;cap=baseballcap;hat=cowboyhat;=baseballcap";
	static const string MASK = "gasmask=gasmask;gp5=gasmask;welding=weldingmask";
	static const string BACK = "mountain=mountainbackpack;taloon=taloonbackpack;hunt=hunterbackpack;child=schoolbackpack;school=schoolbackpack;improvised=improvisedbackpack;courier=improvisedbag;burlap=improvisedbag;leather=improvisedbackpack;=hunterbackpack";
	static const string VEST = "press=vestpress;chestholster=razgruz;chestrig=razgruz;smersh=razgruzbig;plate=razgruzbig;ukass=razgruzbig;assault=razgruzbig;=vest";
	static const string WEAPON = "aks74u=aks74urifle;akm=akmrifle;ak74=ak74rifle;ak101=ak74rifle;aug=augrifle;m4a1=m4rifle;m16=m4rifle;fal=fncalrifle;svd=svdrifle;vss=vssrifle;asval=vssrifle;sks=sksrifle;sawedoff_mosin=sawedmosin;mosin=mosinnagant;winchester=repeaterrifle;repeater=repeaterrifle;cz527=sporterrifle;cr527=sporterrifle;ruger=sporterrifle;izh18=sporterrifle;b95=sporterrifle;scout=sv98rifle;m70=sv98rifle;saiga=saigashotgun;mp133=r670shotgun;remington=r670shotgun;sawedoff_izh=sawedizh;izh43=bm16shotgun;bk43=bm16shotgun;mp5=mp5smg;ump=mp5smg;bizon=bizonsmg;pp19=bizonsmg;mac=mac10pistol;cz61=mac10pistol;glock=glockpistol;fnx=fnxpistol;cz75=fnxpistol;magnum=magnumpistol;deagle=magnumpistol;crossbow=crossbow;bow=improvisedbow;fireaxe=fireaxe;hatchet=axered;woodaxe=fireaxe;axe=axered;bat=baseballbat;crowbar=crowbar;pickaxe=pickaxe;shovel=shovel;sledge=sledgehammer;pitchfork=pitchfork;wrench=pipewrench;machete=knifeany;knife=knifeany;sword=sword";

	// colour words in DayZ class names (a whole "_" part of the name, lower case) -> DL_FilmPlayer.Tint names
	static const string COLOURS = ",black,white,gray,grey,red,blue,green,yellow,orange,pink,brown,beige,olive,lime,purple,violet,khaki,tan,navy,lightblue,darkblue,bluedark,darkgreen,greendark,maroon,wine,camo,";

	static string ColourOf(string type)
	{
		string t = type;
		t.ToLower();
		array<string> parts = new array<string>;
		t.Split("_", parts);
		for (int i = parts.Count() - 1; i > 0; i--)
		{
			if (COLOURS.Contains("," + parts[i] + ","))
				return parts[i];
		}
		return "";
	}

	// the layer for a clothing item: "layer" or "layer@colour" when it has a plain colour and a grey copy
	static string Dress(string type, string table)
	{
		string l = Match(type, table);
		if (l == "")
			return "";
		string c = ColourOf(type);
		if (c != "" && ("," + DL_FilmCatalog.TINT + ",").Contains("," + l + ","))
			return l + "@" + c;
		return l;
	}

	static DL_FilmActor From(string name, string steamId, DL_PlayerData d)
	{
		DL_FilmActor a = new DL_FilmActor();
		a.name = name;
		string ct = d.charType;
		ct.ToLower();
		array<string> skins = new array<string>;
		array<string> all = DL_FilmCatalog.List(DL_FilmCatalog.SKIN);   // held in a local: a foreach over a return value is freed under it
		foreach (string s : all)
		{
			bool f = FEMALE.Contains("," + s + ",");
			if (f == ct.Contains("survivorf"))
				skins.Insert(s);
		}
		if (skins.Count() == 0)
			skins.Insert("def1");
		int h = steamId.Hash();
		if (h < 0)
			h = -h;
		a.skin = skins[h % skins.Count()];
		string legs;
		string body;
		string vest;
		string back;
		string head;
		string mask;
		string weapon;
		string shoulder;
		string melee;
		foreach (DL_ItemData it : d.items)
		{
			if (!it || it.parent != 0)
				continue;
			if (it.slot == "Legs")
				legs = Dress(it.type, LEGS);
			else if (it.slot == "Body")
				body = Dress(it.type, BODY);
			else if (it.slot == "Vest")
				vest = Dress(it.type, VEST);
			else if (it.slot == "Back")
				back = Dress(it.type, BACK);
			else if (it.slot == "Headgear")
				head = Dress(it.type, HEAD);
			else if (it.slot == "Mask")
				mask = Match(it.type, MASK);
			else if (it.slot == DL_Const.SLOT_HANDS)
				weapon = Match(it.type, WEAPON);
			else if (it.slot == "Shoulder")
				shoulder = Match(it.type, WEAPON);
			else if (it.slot == "Melee")
				melee = Match(it.type, WEAPON);
		}
		// one in the hands, one on the back (the shoulder rifle first, else the melee weapon)
		a.hand = weapon;
		a.back = shoulder;
		if (a.back == "")
			a.back = melee;
		// draw order: legs, body, vest, back, head, mask (the weapons are drawn by the player, on top)
		array<string> order = new array<string>;
		order.Insert(legs);
		order.Insert(body);
		order.Insert(vest);
		order.Insert(back);
		order.Insert(head);
		order.Insert(mask);
		foreach (string l : order)
		{
			if (l != "")
				a.layers.Insert(l);
		}
		return a;
	}

	// which car the film draws, "look@colour" (DL_FilmPlayer.BuildCar): the vanilla cars by class, their paint from the
	// class-name colour (OffroadHatchback_Blue) or the car's default paint; modded cars by the nearest kind, in grey
	static string CarSprite(string carType)
	{
		string t = carType;
		t.ToLower();
		string look = "4x4";
		string colour = "grey";
		if (t.Contains("motorbike_01"))
		{
			look = "moped";                    // Motorbike_01: the moped
			colour = "red";
		}
		else if (t.Contains("motorbike"))
		{
			look = "bike";                     // Motorbike_02: the dirt bike
			colour = "red";
		}
		else if (t.Contains("boat"))
		{
			look = "boat";                     // Boat_01
			colour = "blue";
		}
		else if (t.Contains("offroadhatchback"))
			colour = "green";                  // Ada 4x4
		else if (t.Contains("civiliansedan"))
		{
			look = "sedan";                    // Olga 24
			colour = "white";
		}
		else if (t.Contains("sedan_02"))
		{
			look = "sarka";                    // Sarka 120
			colour = "yellow";
		}
		else if (t.Contains("hatchback_02"))
		{
			look = "gunter";                   // Gunter 2
			colour = "red";
		}
		else if (t.Contains("truck_01"))
		{
			look = "truck";                    // M3S
			colour = "green";
		}
		else if (t.Contains("offroad_02"))
		{
			look = "humvee";                   // M1025: Offroad_02 is the woodland camo one, _Beige the desert one
			colour = "camo";
		}
		else if (t.Contains("truck"))
			look = "truck";
		else if (t.Contains("hatch"))
			look = "gunter";
		else if (t.Contains("sedan"))
			look = "sedan";
		string c = ColourOf(carType);
		if (c != "")
			colour = c;
		return look + "@" + colour;
	}
}
