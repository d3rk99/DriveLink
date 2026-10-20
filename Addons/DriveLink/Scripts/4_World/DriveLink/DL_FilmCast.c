// DriveLink road film - dresses a traveller's film survivor from his real DayZ gear (his snapshot): the item in each
// clothing slot is matched by class-name keywords to the closest MiniDayZ layer (DL_FilmCatalog). Female characters
// (SurvivorF_*) get a female base skin; the skin is fixed per player (picked from the steam id).

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

	static DL_FilmActor From(string name, string steamId, DL_PlayerData d)
	{
		DL_FilmActor a = new DL_FilmActor();
		a.name = name;
		string ct = d.charType;
		ct.ToLower();
		array<string> skins = new array<string>;
		foreach (string s : DL_FilmCatalog.List(DL_FilmCatalog.SKIN))
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
				legs = Match(it.type, LEGS);
			else if (it.slot == "Body")
				body = Match(it.type, BODY);
			else if (it.slot == "Vest")
				vest = Match(it.type, VEST);
			else if (it.slot == "Back")
				back = Match(it.type, BACK);
			else if (it.slot == "Headgear")
				head = Match(it.type, HEAD);
			else if (it.slot == "Mask")
				mask = Match(it.type, MASK);
			else if (it.slot == DL_Const.SLOT_HANDS)
				weapon = Match(it.type, WEAPON);
			else if (it.slot == "Shoulder")
				shoulder = Match(it.type, WEAPON);
			else if (it.slot == "Melee")
				melee = Match(it.type, WEAPON);
		}
		if (weapon == "")
			weapon = shoulder;
		if (weapon == "")
			weapon = melee;
		// draw order: legs, body, vest, back, head, mask, weapon
		array<string> order = new array<string>;
		order.Insert(legs);
		order.Insert(body);
		order.Insert(vest);
		order.Insert(back);
		order.Insert(head);
		order.Insert(mask);
		order.Insert(weapon);
		foreach (string l : order)
		{
			if (l != "")
				a.layers.Insert(l);
		}
		return a;
	}

	// which car sprite: hatchbacks and sedans look like the Volga, everything else like the UAZ
	static string CarSprite(string carType)
	{
		string t = carType;
		t.ToLower();
		if (t.Contains("hatchback") || t.Contains("sedan") || t.Contains("civilian") || t.Contains("olga"))
			return "volga";
		return "uaz";
	}
}
