// DriveLink v0.2 - a player's whole state <-> DL_PlayerData. Server only. NO RECURSION anywhere.
// Research/R1 (how UniversalApi does it, read only): the engine lists the whole inventory in one call and every item
// is put back at its exact place, with a ground fallback so nothing is ever lost silently.
// Engine calls (vanilla 1.29 scripts):
//   GameInventory.EnumerateInventory(InventoryTraversalType.LEVELORDER, out items)  inventory.c:127 / gameplay.c:5-11
//   EntityAI.GetHierarchyParent  entityai.c:881;  InventoryLocation GetType/GetSlot/GetIdx/GetRow/GetCol/GetFlip  inventorylocation.c:43-95
//   GameInventory.CreateAttachmentEx/CreateEntityInCargoEx(type,idx,row,col,flip)/CreateEntityInCargo/CreateInInventory  inventory.c:143-150
//   HumanInventory.CreateInHands  humaninventory.c:44
//   Magazine.GetCartridgeAtIndex / ServerSetAmmoCount / ServerStoreCartridge  magazine.c:70,88,97
//   Weapon.GetMuzzleCount/GetCurrentMuzzle/SetCurrentMuzzle/GetCurrentMode/SetCurrentMode/IsChamberEmpty/GetCartridgeInfo/
//     PushCartridgeToChamber/GetInternalMagazineCartridgeCount/GetInternalMagazineCartridgeInfo/PushCartridgeToInternalMagazine/
//     Synchronize  weapon.c:11-208;  Weapon_Base.RandomizeFSMState  weapon_base.c:675 (FillChamber does the same after pushing)
//   ItemBase quantity/wet/liquid/agents (TransferAgents, itembase.c:4014)/cleanness (SetCleanness :3813); EntityAI
//     GetTemperature/SetTemperatureDirect  entityai.c:2313,2462;  ComponentEnergyManager GetEnergy/SetEnergy/IsSwitchedOn/SwitchOn
//   Edible_Base.GetFoodStageType/ChangeFoodStage  edible_base.c:531,598
//   Player: GetStatWater/Energy/HeatComfort/BloodType, blood type visible (playerbase.c:6954-6970), lifespan (:6832-6858),
//     bloody hands (:6894), broken legs (:3710-3716), m_PersistentFlags (:152), damage zones (DamageSystem.GetDamageZoneMap,
//     damagesystem.c:27), AnalyticsManagerServer stats, modifiers (ModifiersManager m_ModifierListArray / ActivateModifier /
//     ModifierBase.GetAttachedTime/SetAttachedTime/IsPersistent/IsTrackAttachedTime), agents (PlayerAgentPool.GetSingleAgentCount/
//     SetAgentCount), bleeding (BleedingSourcesManagerServer: GetBleedingBits, Get/SetBleedingSourceActiveTime, Get/SetBleedingSourceType,
//     AttemptAddBleedingSourceBySelection not needed: the vanilla store path uses AddBleedingSource, which is protected -> see Apply),
//     stomach (PlayerStomach.m_StomachContents / AddToStomach  playerstomach.c:132,401)

class DL_Snapshot
{
	// ---------------- capture ----------------

	DL_PlayerData Capture(PlayerBase p)
	{
		DL_PlayerData d = new DL_PlayerData();
		d.charType = p.GetType();
		d.health = p.GetHealth("", "Health");
		d.blood = p.GetHealth("", "Blood");
		d.shock = p.GetHealth("", "Shock");
		CaptureZones(p, d);
		d.water = p.GetStatWater().Get();
		d.energy = p.GetStatEnergy().Get();
		d.heatComfort = p.GetStatHeatComfort().Get();
		d.bloodType = p.GetStatBloodType().Get();
		d.bloodTypeVisible = p.HasBloodTypeVisible();
		d.lifespan = p.GetLifeSpanState();
		d.lastShaved = p.GetLastShavedSeconds();
		d.bloodyHands = p.HasBloodyHandsEx();
		d.brokenLegs = p.GetBrokenLegs();
		d.persistentFlags = p.m_PersistentFlags;
		d.playtime = p.StatGet(AnalyticsManagerServer.STAT_PLAYTIME);
		d.killsPlayers = p.StatGet(AnalyticsManagerServer.STAT_PLAYERS_KILLED);
		d.killsInfected = p.StatGet(AnalyticsManagerServer.STAT_INFECTED_KILLED);
		d.distance = p.StatGet(AnalyticsManagerServer.STAT_DISTANCE);
		d.longestShot = p.StatGet(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT);
		d.restrained = p.IsRestrained();
		d.unconscious = p.IsUnconscious();
		CaptureModifiers(p, d);
		CaptureAgents(p, d);
		CaptureBleeding(p, d);
		CaptureStomach(p, d);
		CaptureItems(p, d);
		return d;
	}

	protected void CaptureZones(PlayerBase p, DL_PlayerData d)
	{
		DamageZoneMap zones = new DamageZoneMap;
		if (!DamageSystem.GetDamageZoneMap(p, zones))
			return;
		for (int i = 0; i < zones.Count(); i++)
		{
			DL_Zone_Health z = new DL_Zone_Health();
			z.zone = zones.GetKey(i);
			z.health = p.GetHealth(z.zone, "Health");
			d.zones.Insert(z);
		}
	}

	protected void CaptureModifiers(PlayerBase p, DL_PlayerData d)
	{
		ModifiersManager mm = p.GetModifiersManager();
		if (!mm || !mm.m_ModifierListArray)
			return;
		for (int i = 0; i < mm.m_ModifierListArray.Count(); i++)
		{
			ModifierBase m = mm.m_ModifierListArray[i];
			if (!m || !m.IsActive() || !m.IsPersistent())
				continue;
			DL_Modifier dm = new DL_Modifier();
			dm.id = m.GetModifierID();
			dm.time = -1;
			if (m.IsTrackAttachedTime())
				dm.time = m.GetAttachedTime();
			d.modifiers.Insert(dm);
		}
	}

	protected void CaptureAgents(PlayerBase p, DL_PlayerData d)
	{
		if (!p.m_AgentPool)
			return;
		PluginTransmissionAgents plugin = PluginTransmissionAgents.Cast(GetPlugin(PluginTransmissionAgents));
		if (!plugin)
			return;
		array<int> ids = plugin.GetAgentList().GetKeyArray();
		for (int i = 0; i < ids.Count(); i++)
		{
			int count = p.m_AgentPool.GetSingleAgentCount(ids[i]);
			if (count <= 0)
				continue;
			DL_Modifier a = new DL_Modifier();
			a.id = ids[i];
			a.time = count;
			d.agentPool.Insert(a);
		}
	}

	protected void CaptureBleeding(PlayerBase p, DL_PlayerData d)
	{
		BleedingSourcesManagerServer bm = p.GetBleedingManagerServer();
		if (!bm)
			return;
		int bits = p.GetBleedingBits();
		for (int i = 0; i < 32; i++)
		{
			int bit = 1 << i;
			if ((bits & bit) == 0)
				continue;
			DL_Bleeding b = new DL_Bleeding();
			b.bit = bit;
			b.time = bm.GetBleedingSourceActiveTime(bit);
			b.type = bm.GetBleedingSourceType(bit);
			d.bleeding.Insert(b);
		}
	}

	protected void CaptureStomach(PlayerBase p, DL_PlayerData d)
	{
		PlayerStomach st = p.GetStomach();
		if (!st || !st.m_StomachContents)
			return;
		for (int i = 0; i < st.m_StomachContents.Count(); i++)
		{
			StomachItem si = st.m_StomachContents[i];
			if (!si)
				continue;
			DL_StomachItem ds = new DL_StomachItem();
			ds.type = si.GetClassName();
			ds.amount = si.GetAmount();
			ds.stage = si.GetFoodStage();
			ds.agents = si.m_Agents;
			ds.temperature = si.GetTemperature();
			d.stomach.Insert(ds);
		}
	}

	// LEVELORDER = the player's direct items first, then their children, then grandchildren: every parent is in the list
	// before its children, so the rebuild can go straight down the list.
	protected void CaptureItems(PlayerBase p, DL_PlayerData d)
	{
		array<EntityAI> all = new array<EntityAI>;
		p.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, all);
		map<EntityAI, int> ids = new map<EntityAI, int>;
		ids.Set(p, 0);
		EntityAI inHands = p.GetHumanInventory().GetEntityInHands();
		int next = 1;
		for (int i = 0; i < all.Count() && next <= DL_Const.MAX_ITEMS; i++)
		{
			EntityAI e = all[i];
			if (!e || e == p)
				continue;
			EntityAI par = e.GetHierarchyParent();
			int parentId;
			if (!par || !ids.Find(par, parentId))
			{
				Print(DL_Const.LOG_TAG + "capture: skipped " + e.GetType() + " (parent not in list)");
				continue;
			}
			if (e == inHands && d.restrained)
			{
				d.restrainItem = e.GetType(); // the locked cuffs: rebuilt by the restraint itself
				continue;
			}
			DL_ItemData it = CaptureOne(p, e, inHands);
			it.id = next;
			it.parent = parentId;
			ids.Set(e, next);
			next++;
			d.items.Insert(it);
		}
	}

	protected DL_ItemData CaptureOne(PlayerBase p, EntityAI e, EntityAI inHands)
	{
		DL_ItemData d = new DL_ItemData();
		d.type = e.GetType();
		d.health = e.GetHealth("", "");
		d.slot = DL_Const.SLOT_CARGO;
		InventoryLocation loc = new InventoryLocation();
		if (e == inHands)
			d.slot = DL_Const.SLOT_HANDS;
		else if (e.GetInventory().GetCurrentInventoryLocation(loc))
		{
			if (loc.GetType() == InventoryLocationType.ATTACHMENT)
				d.slot = InventorySlots.GetSlotName(loc.GetSlot());
			else if (loc.GetType() == InventoryLocationType.CARGO)
			{
				d.cIdx = loc.GetIdx();
				d.cRow = loc.GetRow();
				d.cCol = loc.GetCol();
				d.cFlip = loc.GetFlip();
			}
		}
		d.quickbar = p.FindQuickBarEntityIndex(e);
		d.temperature = e.GetTemperature();

		ItemBase ib = ItemBase.Cast(e);
		if (ib)
		{
			if (ib.HasQuantity() && !ib.IsMagazine())
				d.quantity = ib.GetQuantity();
			d.wet = ib.GetWet();
			d.liquid = ib.GetLiquidType();
			d.agents = ib.GetAgents();
			d.cleanness = ib.GetCleanness();
			if (ib.GetCompEM())
			{
				d.energy = ib.GetCompEM().GetEnergy();
				d.isOn = ib.GetCompEM().IsSwitchedOn();
			}
		}
		Edible_Base food = Edible_Base.Cast(e);
		if (food && food.HasFoodStage())
			d.foodStage = food.GetFoodStageType();

		Magazine mag = Magazine.Cast(e);
		if (mag && !mag.IsAmmoPile())
		{
			d.rounds = new array<ref DL_Round>;
			for (int r = 0; r < mag.GetAmmoCount(); r++)
			{
				float dmg;
				string ctype;
				if (mag.GetCartridgeAtIndex(r, dmg, ctype))
				{
					DL_Round rd = new DL_Round();
					rd.type = ctype;
					rd.dmg = dmg;
					d.rounds.Insert(rd);
				}
			}
		}
		else if (mag)
			d.quantity = mag.GetAmmoCount(); // loose ammo pile: just the count

		Weapon_Base w = Weapon_Base.Cast(e);
		if (w)
		{
			d.muzzle = w.GetCurrentMuzzle();
			d.chambers = new array<ref DL_Round>;
			d.fireModes = new array<int>;
			for (int mi = 0; mi < w.GetMuzzleCount(); mi++)
			{
				DL_Round ch = new DL_Round();
				ch.type = "";
				if (!w.IsChamberEmpty(mi))
				{
					float cd;
					string ct;
					if (w.GetCartridgeInfo(mi, cd, ct))
					{
						ch.type = ct;
						ch.dmg = cd;
					}
				}
				d.chambers.Insert(ch);
				d.fireModes.Insert(w.GetCurrentMode(mi));
			}
			d.internalMag = new array<ref DL_Round>;
			for (int ci = 0; ci < w.GetInternalMagazineCartridgeCount(0); ci++)
			{
				float idmg;
				string itype;
				if (w.GetInternalMagazineCartridgeInfo(0, ci, idmg, itype))
				{
					DL_Round ir = new DL_Round();
					ir.type = itype;
					ir.dmg = idmg;
					d.internalMag.Insert(ir);
				}
			}
		}
		return d;
	}

	// ---------------- rebuild on the target server ----------------

	// The character already exists (empty). Returns how many items were made; "dropped" counts the ones that had to
	// go on the ground because their place was taken (never lost).
	int Apply(PlayerBase p, DL_PlayerData d, out int dropped)
	{
		int made = ApplyItems(p, d, dropped);
		p.SetHealth("", "Health", d.health);
		p.SetHealth("", "Blood", d.blood);
		p.SetHealth("", "Shock", d.shock);
		for (int z = 0; z < d.zones.Count(); z++)
		{
			if (d.zones[z])
				p.SetHealth(d.zones[z].zone, "Health", d.zones[z].health);
		}
		p.GetStatWater().Set(d.water);
		p.GetStatEnergy().Set(d.energy);
		p.GetStatHeatComfort().Set(d.heatComfort);
		if (d.bloodType >= 0)
		{
			p.GetStatBloodType().Set(d.bloodType);
			p.SetBloodType(d.bloodType);
		}
		p.SetBloodTypeVisible(d.bloodTypeVisible);
		p.SetLifeSpanStateVisible(d.lifespan);
		p.SetLastShavedSeconds(d.lastShaved);
		p.SetBloodyHandsEx(d.bloodyHands);
		p.SetBrokenLegs(d.brokenLegs);
		p.m_PersistentFlags = d.persistentFlags;
		// stats are counters: StatUpdate adds, a new character starts at 0 (MapLink PlayerDataStore.c:446-450 does the same)
		p.StatUpdate(AnalyticsManagerServer.STAT_PLAYTIME, d.playtime);
		p.StatUpdate(AnalyticsManagerServer.STAT_PLAYERS_KILLED, d.killsPlayers);
		p.StatUpdate(AnalyticsManagerServer.STAT_INFECTED_KILLED, d.killsInfected);
		p.StatUpdate(AnalyticsManagerServer.STAT_DISTANCE, d.distance);
		p.StatUpdate(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT, d.longestShot);
		ApplyModifiers(p, d);
		ApplyAgents(p, d);
		ApplyBleeding(p, d);
		ApplyStomach(p, d);
		if (d.restrained && d.restrainItem != "")
		{
			// same end state as RestrainTargetPlayerLambda.OnSuccess (actionrestraintarget.c:221-227)
			EntityAI cuffs = p.GetHumanInventory().CreateInHands(d.restrainItem);
			if (cuffs)
			{
				p.SetRestrained(true);
				p.OnItemInHandsChanged();
			}
			else
				Print(DL_Const.LOG_TAG + "could not put " + d.restrainItem + " back in the hands");
		}
		p.SetSynchDirty();
		return made;
	}

	protected int ApplyItems(PlayerBase p, DL_PlayerData d, out int dropped)
	{
		map<int, EntityAI> made = new map<int, EntityAI>;
		made.Set(0, p);
		array<ref DL_ItemData> toQuickbar = new array<ref DL_ItemData>;
		dropped = 0;
		int count = 0;
		for (int i = 0; i < d.items.Count(); i++)
		{
			DL_ItemData it = d.items[i];
			if (!it)
				continue;
			EntityAI parent;
			bool parentOk = made.Find(it.parent, parent) && parent;
			EntityAI e;
			if (parentOk)
				e = CreateAt(p, parent, it);
			if (!e)
			{
				// never lose it: on the ground at the player (same idea as UAPI's CreateObject fallback, ItemStore.c:133)
				e = EntityAI.Cast(g_Game.CreateObjectEx(it.type, p.GetPosition(), ECE_PLACE_ON_SURFACE));
				if (e)
				{
					dropped++;
					Print(DL_Const.LOG_TAG + "no room for " + it.type + " (" + it.slot + "), put on the ground");
				}
				else
				{
					Print(DL_Const.LOG_TAG + "could not create " + it.type + " at all");
					continue;
				}
			}
			made.Set(it.id, e);
			count++;
			ApplyState(e, it);
			if (it.quickbar >= 0)
				toQuickbar.Insert(it);
		}
		for (int q = 0; q < toQuickbar.Count(); q++)
		{
			EntityAI qe;
			if (made.Find(toQuickbar[q].id, qe) && qe)
				p.SetQuickBarEntityShortcut(qe, toQuickbar[q].quickbar, true);
		}
		return count;
	}

	protected EntityAI CreateAt(PlayerBase p, EntityAI parent, DL_ItemData it)
	{
		GameInventory inv = parent.GetInventory();
		EntityAI e;
		if (it.slot == DL_Const.SLOT_HANDS)
			return p.GetHumanInventory().CreateInHands(it.type);
		if (it.slot != DL_Const.SLOT_CARGO)
		{
			int slotId = InventorySlots.GetSlotIdFromString(it.slot);
			if (slotId != InventorySlots.INVALID)
				e = inv.CreateAttachmentEx(it.type, slotId);
			if (!e)
				e = inv.CreateAttachment(it.type);
			return e;
		}
		if (it.cIdx >= 0)
			e = inv.CreateEntityInCargoEx(it.type, it.cIdx, it.cRow, it.cCol, it.cFlip);
		if (!e)
			e = inv.CreateEntityInCargo(it.type);
		return e;
	}

	protected void ApplyState(EntityAI e, DL_ItemData d)
	{
		if (d.health >= 0)
			e.SetHealth("", "", d.health);
		if (d.temperature > -9999)
			e.SetTemperatureDirect(d.temperature);
		ItemBase ib = ItemBase.Cast(e);
		if (ib)
		{
			if (d.quantity >= 0 && ib.HasQuantity() && !ib.IsMagazine())
				ib.SetQuantity(d.quantity);
			ib.SetWet(d.wet);
			if (d.liquid != 0)
				ib.SetLiquidType(d.liquid);
			ib.RemoveAllAgents();
			ib.TransferAgents(d.agents);
			ib.SetCleanness(d.cleanness);
			if (ib.GetCompEM() && d.energy >= 0)
			{
				ib.GetCompEM().SetEnergy(d.energy);
				if (d.isOn)
					ib.GetCompEM().SwitchOn();
			}
		}
		Edible_Base food = Edible_Base.Cast(e);
		if (food && food.HasFoodStage() && d.foodStage >= 0 && food.GetFoodStageType() != d.foodStage)
			food.ChangeFoodStage(d.foodStage);

		Magazine mag = Magazine.Cast(e);
		if (mag && d.rounds)
		{
			mag.ServerSetAmmoCount(0);
			for (int r = 0; r < d.rounds.Count(); r++)
			{
				if (d.rounds[r])
					mag.ServerStoreCartridge(d.rounds[r].dmg, d.rounds[r].type);
			}
			mag.SetSynchDirty();
		}
		else if (mag && d.quantity >= 0)
			mag.ServerSetAmmoCount(d.quantity);

		Weapon_Base w = Weapon_Base.Cast(e);
		if (w)
		{
			int mi;
			if (d.chambers)
			{
				for (mi = 0; mi < d.chambers.Count() && mi < w.GetMuzzleCount(); mi++)
				{
					if (d.chambers[mi] && d.chambers[mi].type != "" && w.IsChamberEmpty(mi))
						w.PushCartridgeToChamber(mi, d.chambers[mi].dmg, d.chambers[mi].type);
				}
			}
			if (d.internalMag)
			{
				for (int ci = 0; ci < d.internalMag.Count(); ci++)
				{
					if (d.internalMag[ci])
						w.PushCartridgeToInternalMagazine(0, d.internalMag[ci].dmg, d.internalMag[ci].type);
				}
			}
			if (d.fireModes)
			{
				for (mi = 0; mi < d.fireModes.Count() && mi < w.GetMuzzleCount(); mi++)
					w.SetCurrentMode(mi, d.fireModes[mi]);
			}
			if (d.muzzle >= 0 && d.muzzle < w.GetMuzzleCount())
				w.SetCurrentMuzzle(d.muzzle);
			// FSM must match the chamber/mag state, like FillChamber does after pushing (weapon_base.c:978-980).
			// Called again after the attached magazine is made (DL_FixWeapons).
			w.RandomizeFSMState();
			w.Synchronize();
		}
	}

	// After all items exist: weapons whose magazine was attached after them need their FSM refreshed.
	void FixWeapons(PlayerBase p)
	{
		array<EntityAI> all = new array<EntityAI>;
		p.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, all);
		for (int i = 0; i < all.Count(); i++)
		{
			Weapon_Base w = Weapon_Base.Cast(all[i]);
			if (w)
			{
				w.RandomizeFSMState();
				w.Synchronize();
			}
		}
	}

	protected void ApplyModifiers(PlayerBase p, DL_PlayerData d)
	{
		ModifiersManager mm = p.GetModifiersManager();
		if (!mm)
			return;
		for (int i = 0; i < d.modifiers.Count(); i++)
		{
			DL_Modifier dm = d.modifiers[i];
			if (!dm)
				continue;
			ModifierBase m = mm.GetModifier(dm.id);
			if (!m)
				continue;
			if (dm.time >= 0 && m.IsTrackAttachedTime())
				m.SetAttachedTime(dm.time);
			// same activation the game uses when loading a character from its database (modifiersmanager.c:289)
			mm.ActivateModifier(dm.id, EActivationType.TRIGGER_EVENT_ON_CONNECT);
		}
	}

	protected void ApplyAgents(PlayerBase p, DL_PlayerData d)
	{
		if (!p.m_AgentPool)
			return;
		for (int i = 0; i < d.agentPool.Count(); i++)
		{
			if (d.agentPool[i])
				p.m_AgentPool.SetAgentCount(d.agentPool[i].id, d.agentPool[i].time);
		}
	}

	protected void ApplyBleeding(PlayerBase p, DL_PlayerData d)
	{
		BleedingSourcesManagerServer bm = p.GetBleedingManagerServer();
		if (!bm || d.bleeding.Count() == 0)
			return;
		// The vanilla database load path is BleedingSourcesManagerServer.OnStoreLoad (bleedingsourcesmanagerserver.c:255):
		// it reads (bits, then per bit time + type). Feed it exactly that stream instead of calling its protected helpers.
		ScriptReadWriteContext rw = new ScriptReadWriteContext;
		ParamsWriteContext wctx = rw.GetWriteContext();
		int bits = 0;
		int i;
		for (i = 0; i < d.bleeding.Count(); i++)
		{
			if (d.bleeding[i])
				bits = bits | d.bleeding[i].bit;
		}
		wctx.Write(bits);
		// OnStoreLoad walks bits low -> high, so write the entries in that order
		for (int b = 0; b < 32; b++)
		{
			int bit = 1 << b;
			for (i = 0; i < d.bleeding.Count(); i++)
			{
				if (d.bleeding[i] && d.bleeding[i].bit == bit)
				{
					wctx.Write(d.bleeding[i].time);
					wctx.Write(d.bleeding[i].type);
				}
			}
		}
		if (!bm.OnStoreLoad(rw.GetReadContext(), g_Game.SaveVersion()))
			Print(DL_Const.LOG_TAG + "bleeding sources could not be restored");
	}

	protected void ApplyStomach(PlayerBase p, DL_PlayerData d)
	{
		PlayerStomach st = p.GetStomach();
		if (!st)
			return;
		st.ClearContents();
		for (int i = 0; i < d.stomach.Count(); i++)
		{
			DL_StomachItem s = d.stomach[i];
			if (s)
				st.AddToStomach(s.type, s.amount, s.stage, s.agents, s.temperature);
		}
	}
}
