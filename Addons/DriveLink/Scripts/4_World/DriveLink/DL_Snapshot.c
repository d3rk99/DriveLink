// DriveLink - a player's gear + state <-> DL_PlayerData. Server only.
// Item walk/restore is the PersistentBodies PB_Snapshot design (same author), checked against DayZ 1.29:
//   GameInventory.AttachmentCount/GetAttachmentFromIndex/GetCargo/CreateAttachmentEx/CreateEntityInCargo/CreateInInventory
//   HumanInventory.GetEntityInHands/CreateInHands, InventoryLocation.GetType/GetSlot
//   ItemBase.HasQuantity/GetQuantity/SetQuantity/GetWet/SetWet/GetLiquidType/SetLiquidType, Magazine.GetAmmoCount/ServerSetAmmoCount
// Player stats: GetStatWater/GetStatEnergy/GetStatHeatComfort .Get/.Set (playerbase.c:1331-1342),
//   health zones "Health"/"Blood"/"Shock" (playerbase.c:1248-1301), IsRestrained/SetRestrained (playerbase.c:2049-2058),
//   restraint = the locked tool in the hands + SetRestrained(true) (RestrainTargetPlayerLambda, actionrestraintarget.c:208-227).

class DL_RestoreJob
{
	EntityAI parent;
	DL_ItemData data;
	bool onPlayer;
	void DL_RestoreJob(EntityAI p, DL_ItemData d, bool isPlayer)
	{
		parent = p;
		data = d;
		onPlayer = isPlayer;
	}
}

class DL_Snapshot
{
	protected int m_Count;

	DL_PlayerData Capture(PlayerBase p)
	{
		DL_PlayerData d = new DL_PlayerData();
		m_Count = 0;
		d.charType = p.GetType();
		d.health = p.GetHealth("", "Health");
		d.blood = p.GetHealth("", "Blood");
		d.shock = p.GetHealth("", "Shock");
		d.water = p.GetStatWater().Get();
		d.energy = p.GetStatEnergy().Get();
		d.heatComfort = p.GetStatHeatComfort().Get();
		d.restrained = p.IsRestrained();
		d.unconscious = p.IsUnconscious();

		CaptureChildren(p, d.items, 0);
		EntityAI inHands = p.GetHumanInventory().GetEntityInHands();
		if (inHands)
		{
			if (d.restrained)
				d.restrainItem = inHands.GetType(); // the locked cuffs/rope: rebuilt by the restraint, not as an item
			else
			{
				DL_ItemData h = CaptureItem(inHands, DL_Const.SLOT_HANDS, 0);
				if (h)
					d.items.Insert(h);
			}
		}
		return d;
	}

	protected void CaptureChildren(EntityAI parent, array<ref DL_ItemData> into, int depth)
	{
		if (!parent || !parent.GetInventory() || depth > 10)
			return;
		GameInventory inv = parent.GetInventory();
		int i;
		int n = inv.AttachmentCount();
		for (i = 0; i < n; i++)
		{
			EntityAI att = inv.GetAttachmentFromIndex(i);
			if (!att)
				continue;
			DL_ItemData a = CaptureItem(att, SlotNameOf(att), depth);
			if (a)
				into.Insert(a);
		}
		CargoBase cargo = inv.GetCargo();
		if (cargo)
		{
			int c = cargo.GetItemCount();
			for (i = 0; i < c; i++)
			{
				EntityAI it = cargo.GetItem(i);
				if (!it)
					continue;
				DL_ItemData ci = CaptureItem(it, DL_Const.SLOT_CARGO, depth);
				if (ci)
					into.Insert(ci);
			}
		}
	}

	protected DL_ItemData CaptureItem(EntityAI e, string slotName, int depth)
	{
		if (!e)
			return null;
		m_Count++;
		if (m_Count > DL_Const.MAX_ITEMS)
			return null;
		DL_ItemData d = new DL_ItemData();
		d.type = e.GetType();
		d.slot = slotName;
		d.health01 = e.GetHealth01("", "");
		ItemBase ib = ItemBase.Cast(e);
		if (ib)
		{
			if (ib.HasQuantity())
				d.quantity = ib.GetQuantity();
			d.wet = ib.GetWet();
			d.liquid = ib.GetLiquidType();
		}
		Magazine mag = Magazine.Cast(e);
		if (mag)
			d.ammo = mag.GetAmmoCount();
		CaptureChildren(e, d.children, depth + 1);
		return d;
	}

	protected string SlotNameOf(EntityAI e)
	{
		InventoryLocation loc = new InventoryLocation();
		if (e.GetInventory() && e.GetInventory().GetCurrentInventoryLocation(loc) && loc.GetType() == InventoryLocationType.ATTACHMENT)
			return InventorySlots.GetSlotName(loc.GetSlot());
		return DL_Const.SLOT_CARGO;
	}

	// ---------------- rebuild on the target server ----------------

	// Character already created (empty) by DL_MissionServer. Returns how many items were made.
	int Apply(PlayerBase p, DL_PlayerData d)
	{
		int made = RestoreList(p, d.items, true);
		p.SetHealth("", "Health", d.health);
		p.SetHealth("", "Blood", d.blood);
		p.SetHealth("", "Shock", d.shock);
		p.GetStatWater().Set(d.water);
		p.GetStatEnergy().Set(d.energy);
		p.GetStatHeatComfort().Set(d.heatComfort);
		if (d.restrained && d.restrainItem != "")
		{
			// same end state as RestrainTargetPlayerLambda.OnSuccess: locked tool in the hands, restrained flag
			EntityAI cuffs = p.GetHumanInventory().CreateInHands(d.restrainItem);
			if (cuffs)
			{
				p.SetRestrained(true);
				p.OnItemInHandsChanged();
			}
			else
				Print(DL_Const.LOG_TAG + "could not put " + d.restrainItem + " back in the hands");
		}
		return made;
	}

	// NO recursion (see DL_Hub.CountItems): a work list of (parent, item). Parents are always handled before their
	// children because children are only added after their parent was created.
	protected int RestoreList(EntityAI root, array<ref DL_ItemData> items, bool isPlayer)
	{
		int made = 0;
		if (!root || !items)
			return 0;
		array<ref DL_RestoreJob> todo = new array<ref DL_RestoreJob>;
		int k;
		for (k = 0; k < items.Count(); k++)
			todo.Insert(new DL_RestoreJob(root, items[k], isPlayer));
		int pos = 0;
		while (pos < todo.Count())
		{
			DL_RestoreJob job = todo[pos];
			pos++;
			DL_ItemData d = job.data;
			EntityAI parent = job.parent;
			if (!d || !parent)
				continue;
			EntityAI e;
			if (job.onPlayer && d.slot == DL_Const.SLOT_HANDS)
				e = PlayerBase.Cast(parent).GetHumanInventory().CreateInHands(d.type);
			else
				e = CreateOn(parent, d);
			if (!e)
			{
				Print(DL_Const.LOG_TAG + "could not restore " + d.type + " (" + d.slot + ") in " + parent.GetType() + ", " + DL_Hub.CountItems(d.children).ToString() + " items inside it lost too");
				continue;
			}
			made++;
			ApplyState(e, d);
			if (d.children)
			{
				for (k = 0; k < d.children.Count(); k++)
					todo.Insert(new DL_RestoreJob(e, d.children[k], false));
			}
		}
		return made;
	}

	protected EntityAI CreateOn(EntityAI parent, DL_ItemData d)
	{
		GameInventory inv = parent.GetInventory();
		if (!inv)
			return null;
		EntityAI e;
		if (d.slot != DL_Const.SLOT_CARGO)
		{
			int slotId = InventorySlots.GetSlotIdFromString(d.slot);
			if (slotId != InventorySlots.INVALID)
				e = inv.CreateAttachmentEx(d.type, slotId);
			if (!e)
				e = inv.CreateAttachment(d.type);
		}
		if (!e)
			e = inv.CreateEntityInCargo(d.type);
		if (!e)
			e = inv.CreateInInventory(d.type);
		return e;
	}

	protected void ApplyState(EntityAI e, DL_ItemData d)
	{
		e.SetHealth01("", "", d.health01);
		ItemBase ib = ItemBase.Cast(e);
		if (ib)
		{
			if (d.quantity >= 0 && ib.HasQuantity())
				ib.SetQuantity(d.quantity);
			ib.SetWet(d.wet);
			if (d.liquid != 0)
				ib.SetLiquidType(d.liquid);
		}
		Magazine mag = Magazine.Cast(e);
		if (mag && d.ammo >= 0)
			mag.ServerSetAmmoCount(d.ammo);
	}
}
