// DriveLink main menu "on the road" scene (Research/main-menu-scene.md). While a trip with travel time lasts
// (DL_MenuScene, saved when the server sent the player to the menu), the menu's scene shows his party instead of him
// standing alone: the vehicles in a line where the menu character stood, everyone in his seat in his clothes, the
// menu's own character in his own seat, walkers in a row beside them, the camera on the drivers' side.
// Everything is made in the menu's offline world (vanilla 5_mission/dayzintroscene.c) and deleted with the scene.
// Where the scene stands on each map: a road for land parties, open water for boats. Maps not listed keep the vanilla
// scene's spot. x, z in metres. A road spot has a route: points down the road's middle in the way the cars drive,
// read off the map (script can't see roads: the terrain under them is not reliably its own surface); the cars stand
// at pos on it and drive it. A water spot sails straight on along dir.
class DL_MenuSpots
{
	static bool Find(string world, bool water, out vector pos, out vector dir, array<vector> route, array<float> speeds)
	{
		world.ToLower();
		if (world == "chernarusplus")
		{
			if (water)
			{
				pos = Vector(14851.60, 0, 10906.85);
				dir = Vector(0, 0, 1);
			}
			else
			{
				// the coast road north of Solnichniy, south-west to north-east, the sea on its right
				pos = Vector(13240.27, 0, 10675.65);
				dir = Vector(0.38, 0, 0.925);
				// Derk's recorded drive (dev build F8 recorder, 2026-10-31): north-east up the coast road from Solnichniy,
				// around the barriers; a point every 8 m with the speed he drove there (m/s)
				DL_RouteChernarus(route, speeds);
			}
			return true;
		}
		if (world == "enoch")
		{
			if (water)
				return false;                       // no sea on Livonia: a boat party stands (DL_BuildRoad)
			// a road across the south of the map, west to east (4.3 km)
			pos = Vector(5258.35, 0, 1638.08);
			dir = Vector(0.804, 0, -0.595);
			// Derk's recorded drive (dev build F8 recorder, 2026-10-31): a point every 8 m with the speed he drove
			// there (m/s)
				DL_RouteLivonia(route, speeds);
			return true;
		}
		if (world == "nasdara")
		{
			if (water)
				return false;                       // no water spot picked yet: a boat party stands (DL_BuildRoad)
			// Badlands: a road from the south-east towards the north-west (3.5 km)
			pos = Vector(13699.66, 0, 1224.95);
			dir = Vector(-0.586, 0, 0.810);
			// Derk's recorded drive (dev build F8 recorder, 2026-10-31): a point every 8 m with the speed he drove
			// there (m/s)
				DL_RouteBadlands(route, speeds);
			return true;
		}
		if (world == "sakhal")
		{
			if (!water)
				return false;                       // Frostline is reached by water: no road scene picked
			// Frostline: Derk's recorded boat route in and out (dev build F8 recorder, 2026-10-31)
			pos = Vector(3805.37, 0, 14598.31);
			dir = Vector(0.964, 0, 0.266);
			DL_RouteFrostline(route, speeds);
			return true;
		}
		return false;
	}

	// Frostline: the recorded boat route, every 10th recorded point (about every 30 m: a boat needs no more) with the
	// speed sailed there (m/s)
	static void DL_RouteFrostline(array<vector> route, array<float> speeds)
	{
		route.Insert(Vector(3745.8, 0, 14580.7)); speeds.Insert(4.6);
		route.Insert(Vector(3774.9, 0, 14589.2)); speeds.Insert(11.3);
		route.Insert(Vector(3805.4, 0, 14598.3)); speeds.Insert(10.5);
		route.Insert(Vector(3834.7, 0, 14605.7)); speeds.Insert(8.5);
		route.Insert(Vector(3864.4, 0, 14610.9)); speeds.Insert(11.8);
		route.Insert(Vector(3894.5, 0, 14615.9)); speeds.Insert(14.3);
		route.Insert(Vector(3924.5, 0, 14620.4)); speeds.Insert(14.9);
		route.Insert(Vector(3954.4, 0, 14625.5)); speeds.Insert(13.0);
		route.Insert(Vector(3984.3, 0, 14631.0)); speeds.Insert(11.2);
		route.Insert(Vector(4014.2, 0, 14637.2)); speeds.Insert(12.6);
		route.Insert(Vector(4044.1, 0, 14643.2)); speeds.Insert(14.4);
		route.Insert(Vector(4074.0, 0, 14648.9)); speeds.Insert(15.0);
		route.Insert(Vector(4103.8, 0, 14655.7)); speeds.Insert(13.7);
		route.Insert(Vector(4133.4, 0, 14663.7)); speeds.Insert(11.4);
		route.Insert(Vector(4163.1, 0, 14670.6)); speeds.Insert(11.9);
		route.Insert(Vector(4192.8, 0, 14677.3)); speeds.Insert(14.0);
		route.Insert(Vector(4223.0, 0, 14683.5)); speeds.Insert(15.7);
		route.Insert(Vector(4252.8, 0, 14690.0)); speeds.Insert(15.7);
		route.Insert(Vector(4282.9, 0, 14696.2)); speeds.Insert(12.2);
		route.Insert(Vector(4312.9, 0, 14702.3)); speeds.Insert(11.0);
		route.Insert(Vector(4342.5, 0, 14709.2)); speeds.Insert(12.7);
		route.Insert(Vector(4372.3, 0, 14716.5)); speeds.Insert(15.1);
		route.Insert(Vector(4401.2, 0, 14725.5)); speeds.Insert(15.8);
		route.Insert(Vector(4429.8, 0, 14736.6)); speeds.Insert(12.9);
		route.Insert(Vector(4458.6, 0, 14746.5)); speeds.Insert(11.4);
		route.Insert(Vector(4488.1, 0, 14754.4)); speeds.Insert(11.8);
		route.Insert(Vector(4518.1, 0, 14760.5)); speeds.Insert(14.5);
		route.Insert(Vector(4548.4, 0, 14765.8)); speeds.Insert(15.5);
		route.Insert(Vector(4578.6, 0, 14771.3)); speeds.Insert(13.8);
		route.Insert(Vector(4608.3, 0, 14778.5)); speeds.Insert(11.7);
		route.Insert(Vector(4638.0, 0, 14785.7)); speeds.Insert(11.3);
		route.Insert(Vector(4667.7, 0, 14792.5)); speeds.Insert(13.6);
		route.Insert(Vector(4697.5, 0, 14800.1)); speeds.Insert(15.5);
		route.Insert(Vector(4727.5, 0, 14806.8)); speeds.Insert(15.1);
		route.Insert(Vector(4757.7, 0, 14812.0)); speeds.Insert(13.1);
		route.Insert(Vector(4787.8, 0, 14818.1)); speeds.Insert(11.0);
		route.Insert(Vector(4817.5, 0, 14824.0)); speeds.Insert(12.2);
		route.Insert(Vector(4847.5, 0, 14830.6)); speeds.Insert(14.8);
		route.Insert(Vector(4877.5, 0, 14837.4)); speeds.Insert(14.4);
		route.Insert(Vector(4906.9, 0, 14845.2)); speeds.Insert(12.9);
		route.Insert(Vector(4936.7, 0, 14852.2)); speeds.Insert(11.6);
		route.Insert(Vector(4966.6, 0, 14857.7)); speeds.Insert(11.8);
		route.Insert(Vector(4996.8, 0, 14862.9)); speeds.Insert(14.4);
		route.Insert(Vector(5027.2, 0, 14867.1)); speeds.Insert(15.3);
		route.Insert(Vector(5057.8, 0, 14870.6)); speeds.Insert(14.1);
		route.Insert(Vector(5088.0, 0, 14873.8)); speeds.Insert(12.3);
		route.Insert(Vector(5118.2, 0, 14877.8)); speeds.Insert(10.9);
		route.Insert(Vector(5148.4, 0, 14882.3)); speeds.Insert(13.7);
		route.Insert(Vector(5178.7, 0, 14886.4)); speeds.Insert(15.3);
		route.Insert(Vector(5209.2, 0, 14889.7)); speeds.Insert(14.3);
		route.Insert(Vector(5239.5, 0, 14892.4)); speeds.Insert(12.0);
		route.Insert(Vector(5269.9, 0, 14895.4)); speeds.Insert(11.2);
		route.Insert(Vector(5300.2, 0, 14898.4)); speeds.Insert(12.0);
		route.Insert(Vector(5330.7, 0, 14901.6)); speeds.Insert(14.5);
		route.Insert(Vector(5361.3, 0, 14905.6)); speeds.Insert(14.9);
		route.Insert(Vector(5391.7, 0, 14911.1)); speeds.Insert(12.4);
		route.Insert(Vector(5421.8, 0, 14916.8)); speeds.Insert(11.3);
		route.Insert(Vector(5451.8, 0, 14922.7)); speeds.Insert(11.6);
		route.Insert(Vector(5481.8, 0, 14928.7)); speeds.Insert(14.2);
		route.Insert(Vector(5511.7, 0, 14935.2)); speeds.Insert(15.6);
		route.Insert(Vector(5541.7, 0, 14940.8)); speeds.Insert(14.1);
		route.Insert(Vector(5572.1, 0, 14945.8)); speeds.Insert(11.7);
		route.Insert(Vector(5601.9, 0, 14950.6)); speeds.Insert(11.3);
		route.Insert(Vector(5632.0, 0, 14956.3)); speeds.Insert(13.3);
		route.Insert(Vector(5662.0, 0, 14962.3)); speeds.Insert(15.4);
		route.Insert(Vector(5692.3, 0, 14967.2)); speeds.Insert(14.7);
		route.Insert(Vector(5722.5, 0, 14971.7)); speeds.Insert(11.5);
		route.Insert(Vector(5752.5, 0, 14976.8)); speeds.Insert(11.2);
		route.Insert(Vector(5782.6, 0, 14981.5)); speeds.Insert(12.5);
		route.Insert(Vector(5813.0, 0, 14985.7)); speeds.Insert(14.7);
		route.Insert(Vector(5843.3, 0, 14990.2)); speeds.Insert(15.1);
		route.Insert(Vector(5873.3, 0, 14997.2)); speeds.Insert(12.4);
		route.Insert(Vector(5902.6, 0, 15006.1)); speeds.Insert(11.3);
		route.Insert(Vector(5932.2, 0, 15013.3)); speeds.Insert(11.5);
		route.Insert(Vector(5962.3, 0, 15019.2)); speeds.Insert(14.1);
		route.Insert(Vector(5992.4, 0, 15024.4)); speeds.Insert(15.6);
		route.Insert(Vector(6022.6, 0, 15029.3)); speeds.Insert(14.1);
		route.Insert(Vector(6052.7, 0, 15033.4)); speeds.Insert(11.2);
		route.Insert(Vector(6083.1, 0, 15037.5)); speeds.Insert(11.3);
		route.Insert(Vector(6113.5, 0, 15041.3)); speeds.Insert(13.1);
		route.Insert(Vector(6143.9, 0, 15045.4)); speeds.Insert(15.4);
		route.Insert(Vector(6174.4, 0, 15049.3)); speeds.Insert(15.0);
		route.Insert(Vector(6204.8, 0, 15052.8)); speeds.Insert(11.5);
		route.Insert(Vector(6235.1, 0, 15056.1)); speeds.Insert(10.7);
		route.Insert(Vector(6265.4, 0, 15059.9)); speeds.Insert(12.1);
		route.Insert(Vector(6295.6, 0, 15063.9)); speeds.Insert(14.4);
		route.Insert(Vector(6325.9, 0, 15068.2)); speeds.Insert(15.2);
		route.Insert(Vector(6356.2, 0, 15072.4)); speeds.Insert(13.5);
		route.Insert(Vector(6386.5, 0, 15075.9)); speeds.Insert(11.2);
		route.Insert(Vector(6416.9, 0, 15078.9)); speeds.Insert(12.1);
		route.Insert(Vector(6447.3, 0, 15082.4)); speeds.Insert(14.1);
		route.Insert(Vector(6478.2, 0, 15085.5)); speeds.Insert(15.1);
		route.Insert(Vector(6508.8, 0, 15088.0)); speeds.Insert(14.6);
		route.Insert(Vector(6539.0, 0, 15090.7)); speeds.Insert(11.3);
		route.Insert(Vector(6569.5, 0, 15093.9)); speeds.Insert(11.3);
		route.Insert(Vector(6600.0, 0, 15097.2)); speeds.Insert(13.9);
		route.Insert(Vector(6630.6, 0, 15100.3)); speeds.Insert(15.0);
		route.Insert(Vector(6661.1, 0, 15103.1)); speeds.Insert(14.5);
		route.Insert(Vector(6691.5, 0, 15106.3)); speeds.Insert(12.2);
		route.Insert(Vector(6721.7, 0, 15110.4)); speeds.Insert(10.5);
		route.Insert(Vector(6752.0, 0, 15113.3)); speeds.Insert(12.9);
		route.Insert(Vector(6782.6, 0, 15113.9)); speeds.Insert(14.8);
		route.Insert(Vector(6813.0, 0, 15111.3)); speeds.Insert(15.0);
		route.Insert(Vector(6843.1, 0, 15107.5)); speeds.Insert(13.0);
		route.Insert(Vector(6873.6, 0, 15104.4)); speeds.Insert(11.3);
		route.Insert(Vector(6904.1, 0, 15102.2)); speeds.Insert(12.5);
		route.Insert(Vector(6934.7, 0, 15101.0)); speeds.Insert(14.2);
		route.Insert(Vector(6965.2, 0, 15101.2)); speeds.Insert(14.8);
		route.Insert(Vector(6996.1, 0, 15102.6)); speeds.Insert(13.7);
		route.Insert(Vector(7026.7, 0, 15104.3)); speeds.Insert(11.3);
		route.Insert(Vector(7057.1, 0, 15106.5)); speeds.Insert(10.6);
		route.Insert(Vector(7087.5, 0, 15108.9)); speeds.Insert(13.6);
		route.Insert(Vector(7118.2, 0, 15111.3)); speeds.Insert(14.6);
		route.Insert(Vector(7149.0, 0, 15113.6)); speeds.Insert(14.2);
		route.Insert(Vector(7179.4, 0, 15116.5)); speeds.Insert(12.1);
		route.Insert(Vector(7209.9, 0, 15119.4)); speeds.Insert(11.2);
		route.Insert(Vector(7240.3, 0, 15121.9)); speeds.Insert(12.4);
		route.Insert(Vector(7270.7, 0, 15123.9)); speeds.Insert(14.4);
		route.Insert(Vector(7301.4, 0, 15126.9)); speeds.Insert(15.0);
		route.Insert(Vector(7331.8, 0, 15129.8)); speeds.Insert(12.7);
		route.Insert(Vector(7362.3, 0, 15131.9)); speeds.Insert(11.3);
		route.Insert(Vector(7393.0, 0, 15133.1)); speeds.Insert(11.7);
		route.Insert(Vector(7423.7, 0, 15135.0)); speeds.Insert(14.0);
		route.Insert(Vector(7453.8, 0, 15138.6)); speeds.Insert(14.9);
		route.Insert(Vector(7484.3, 0, 15143.1)); speeds.Insert(13.0);
		route.Insert(Vector(7514.7, 0, 15147.1)); speeds.Insert(10.6);
		route.Insert(Vector(7545.0, 0, 15150.6)); speeds.Insert(11.5);
		route.Insert(Vector(7575.5, 0, 15152.9)); speeds.Insert(13.5);
		route.Insert(Vector(7605.9, 0, 15153.2)); speeds.Insert(14.6);
		route.Insert(Vector(7636.6, 0, 15152.3)); speeds.Insert(14.6);
		route.Insert(Vector(7667.1, 0, 15151.8)); speeds.Insert(12.0);
		route.Insert(Vector(7697.6, 0, 15152.0)); speeds.Insert(10.4);
		route.Insert(Vector(7727.9, 0, 15152.9)); speeds.Insert(13.4);
		route.Insert(Vector(7758.4, 0, 15154.4)); speeds.Insert(14.7);
		route.Insert(Vector(7789.2, 0, 15155.6)); speeds.Insert(14.0);
		route.Insert(Vector(7819.9, 0, 15156.0)); speeds.Insert(12.3);
		route.Insert(Vector(7850.5, 0, 15156.2)); speeds.Insert(11.2);
		route.Insert(Vector(7881.1, 0, 15156.5)); speeds.Insert(12.5);
		route.Insert(Vector(7911.5, 0, 15156.3)); speeds.Insert(14.9);
		route.Insert(Vector(7942.2, 0, 15156.6)); speeds.Insert(15.3);
		route.Insert(Vector(7973.0, 0, 15157.9)); speeds.Insert(13.4);
		route.Insert(Vector(8003.4, 0, 15159.3)); speeds.Insert(10.8);
		route.Insert(Vector(8033.9, 0, 15159.5)); speeds.Insert(12.3);
		route.Insert(Vector(8064.5, 0, 15159.7)); speeds.Insert(14.2);
		route.Insert(Vector(8095.0, 0, 15160.4)); speeds.Insert(14.5);
		route.Insert(Vector(8125.5, 0, 15161.7)); speeds.Insert(14.0);
		route.Insert(Vector(8156.2, 0, 15162.2)); speeds.Insert(12.2);
		route.Insert(Vector(8186.6, 0, 15162.9)); speeds.Insert(11.1);
		route.Insert(Vector(8217.1, 0, 15163.7)); speeds.Insert(13.2);
		route.Insert(Vector(8247.6, 0, 15164.4)); speeds.Insert(15.0);
		route.Insert(Vector(8278.5, 0, 15163.6)); speeds.Insert(15.0);
		route.Insert(Vector(8309.1, 0, 15161.5)); speeds.Insert(11.7);
		route.Insert(Vector(8339.5, 0, 15159.9)); speeds.Insert(11.0);
		route.Insert(Vector(8370.0, 0, 15159.5)); speeds.Insert(13.0);
		route.Insert(Vector(8400.7, 0, 15160.1)); speeds.Insert(15.0);
		route.Insert(Vector(8431.4, 0, 15162.9)); speeds.Insert(14.8);
		route.Insert(Vector(8461.5, 0, 15165.7)); speeds.Insert(13.2);
		route.Insert(Vector(8492.0, 0, 15165.6)); speeds.Insert(11.0);
		route.Insert(Vector(8522.4, 0, 15164.0)); speeds.Insert(11.0);
		route.Insert(Vector(8552.9, 0, 15160.7)); speeds.Insert(14.0);
		route.Insert(Vector(8583.3, 0, 15156.2)); speeds.Insert(15.4);
		route.Insert(Vector(8613.8, 0, 15153.0)); speeds.Insert(14.4);
		route.Insert(Vector(8644.5, 0, 15149.8)); speeds.Insert(11.3);
		route.Insert(Vector(8674.9, 0, 15147.0)); speeds.Insert(11.1);
		route.Insert(Vector(8705.3, 0, 15144.1)); speeds.Insert(13.6);
		route.Insert(Vector(8735.9, 0, 15141.1)); speeds.Insert(14.9);
		route.Insert(Vector(8766.6, 0, 15137.5)); speeds.Insert(15.2);
		route.Insert(Vector(8797.2, 0, 15138.8)); speeds.Insert(12.5);
		route.Insert(Vector(8827.7, 0, 15138.7)); speeds.Insert(11.2);
		route.Insert(Vector(8858.0, 0, 15136.6)); speeds.Insert(12.9);
		route.Insert(Vector(8888.5, 0, 15134.0)); speeds.Insert(15.0);
		route.Insert(Vector(8919.4, 0, 15131.1)); speeds.Insert(15.0);
		route.Insert(Vector(8950.0, 0, 15130.3)); speeds.Insert(13.7);
		route.Insert(Vector(8980.6, 0, 15130.8)); speeds.Insert(11.4);
		route.Insert(Vector(9011.2, 0, 15128.3)); speeds.Insert(11.3);
		route.Insert(Vector(9041.3, 0, 15123.4)); speeds.Insert(14.5);
		route.Insert(Vector(9071.2, 0, 15117.2)); speeds.Insert(15.3);
		route.Insert(Vector(9100.5, 0, 15108.5)); speeds.Insert(13.9);
		route.Insert(Vector(9130.0, 0, 15101.0)); speeds.Insert(11.6);
		route.Insert(Vector(9159.9, 0, 15094.9)); speeds.Insert(11.1);
		route.Insert(Vector(9190.1, 0, 15089.9)); speeds.Insert(13.5);
		route.Insert(Vector(9220.3, 0, 15084.7)); speeds.Insert(14.9);
		route.Insert(Vector(9250.7, 0, 15080.6)); speeds.Insert(14.8);
		route.Insert(Vector(9281.1, 0, 15077.2)); speeds.Insert(13.2);
		route.Insert(Vector(9311.5, 0, 15072.5)); speeds.Insert(11.6);
		route.Insert(Vector(9341.9, 0, 15068.4)); speeds.Insert(12.4);
		route.Insert(Vector(9372.3, 0, 15063.8)); speeds.Insert(14.5);
		route.Insert(Vector(9402.2, 0, 15058.3)); speeds.Insert(14.8);
		route.Insert(Vector(9432.3, 0, 15052.4)); speeds.Insert(14.1);
		route.Insert(Vector(9462.3, 0, 15046.9)); speeds.Insert(11.5);
		route.Insert(Vector(9492.3, 0, 15041.8)); speeds.Insert(11.9);
		route.Insert(Vector(9522.6, 0, 15037.5)); speeds.Insert(13.2);
		route.Insert(Vector(9552.9, 0, 15033.0)); speeds.Insert(15.0);
		route.Insert(Vector(9583.3, 0, 15028.9)); speeds.Insert(14.5);
		route.Insert(Vector(9613.6, 0, 15024.8)); speeds.Insert(12.4);
		route.Insert(Vector(9644.0, 0, 15021.5)); speeds.Insert(11.0);
		route.Insert(Vector(9674.4, 0, 15018.5)); speeds.Insert(12.5);
		route.Insert(Vector(9704.7, 0, 15015.8)); speeds.Insert(14.4);
		route.Insert(Vector(9735.2, 0, 15013.3)); speeds.Insert(14.8);
		route.Insert(Vector(9765.4, 0, 15009.8)); speeds.Insert(12.3);
		route.Insert(Vector(9795.5, 0, 15005.1)); speeds.Insert(11.0);
		route.Insert(Vector(9825.8, 0, 15001.4)); speeds.Insert(12.6);
		route.Insert(Vector(9856.1, 0, 14998.5)); speeds.Insert(14.0);
		route.Insert(Vector(9886.4, 0, 14995.4)); speeds.Insert(15.0);
		route.Insert(Vector(9916.5, 0, 14989.6)); speeds.Insert(13.1);
		route.Insert(Vector(9946.0, 0, 14982.0)); speeds.Insert(10.8);
		route.Insert(Vector(9975.7, 0, 14975.2)); speeds.Insert(12.2);
		route.Insert(Vector(10005.7, 0, 14969.0)); speeds.Insert(14.4);
		route.Insert(Vector(10036.4, 0, 14964.4)); speeds.Insert(15.3);
		route.Insert(Vector(10067.1, 0, 14961.0)); speeds.Insert(14.6);
		route.Insert(Vector(10097.4, 0, 14957.4)); speeds.Insert(10.5);
		route.Insert(Vector(10127.5, 0, 14954.7)); speeds.Insert(11.3);
		route.Insert(Vector(10157.9, 0, 14952.1)); speeds.Insert(13.7);
		route.Insert(Vector(10188.3, 0, 14950.4)); speeds.Insert(15.4);
		route.Insert(Vector(10218.9, 0, 14950.4)); speeds.Insert(14.1);
		route.Insert(Vector(10249.4, 0, 14951.3)); speeds.Insert(12.5);
		route.Insert(Vector(10279.8, 0, 14950.6)); speeds.Insert(11.8);
		route.Insert(Vector(10310.1, 0, 14947.1)); speeds.Insert(13.0);
		route.Insert(Vector(10340.5, 0, 14942.7)); speeds.Insert(14.6);
		route.Insert(Vector(10370.7, 0, 14937.4)); speeds.Insert(15.4);
		route.Insert(Vector(10400.7, 0, 14932.1)); speeds.Insert(13.5);
		route.Insert(Vector(10430.8, 0, 14926.9)); speeds.Insert(11.9);
		route.Insert(Vector(10461.1, 0, 14922.0)); speeds.Insert(12.1);
		route.Insert(Vector(10491.0, 0, 14917.0)); speeds.Insert(13.5);
		route.Insert(Vector(10521.0, 0, 14910.7)); speeds.Insert(14.7);
		route.Insert(Vector(10550.9, 0, 14904.3)); speeds.Insert(13.9);
		route.Insert(Vector(10581.2, 0, 14899.8)); speeds.Insert(12.2);
		route.Insert(Vector(10611.5, 0, 14895.6)); speeds.Insert(11.7);
		route.Insert(Vector(10641.8, 0, 14891.8)); speeds.Insert(12.7);
		route.Insert(Vector(10672.1, 0, 14887.9)); speeds.Insert(13.9);
		route.Insert(Vector(10702.4, 0, 14883.1)); speeds.Insert(14.6);
		route.Insert(Vector(10732.7, 0, 14879.0)); speeds.Insert(13.3);
		route.Insert(Vector(10763.0, 0, 14875.3)); speeds.Insert(11.1);
		route.Insert(Vector(10793.4, 0, 14871.9)); speeds.Insert(12.2);
		route.Insert(Vector(10823.8, 0, 14868.4)); speeds.Insert(13.9);
		route.Insert(Vector(10854.3, 0, 14863.9)); speeds.Insert(15.0);
		route.Insert(Vector(10885.0, 0, 14859.4)); speeds.Insert(14.1);
		route.Insert(Vector(10915.2, 0, 14855.5)); speeds.Insert(12.3);
		route.Insert(Vector(10945.5, 0, 14851.7)); speeds.Insert(11.1);
		route.Insert(Vector(10975.9, 0, 14847.9)); speeds.Insert(13.4);
		route.Insert(Vector(11006.0, 0, 14842.8)); speeds.Insert(15.0);
		route.Insert(Vector(11035.5, 0, 14835.3)); speeds.Insert(14.1);
		route.Insert(Vector(11065.1, 0, 14827.9)); speeds.Insert(12.7);
		route.Insert(Vector(11095.0, 0, 14822.6)); speeds.Insert(11.1);
		route.Insert(Vector(11125.1, 0, 14818.1)); speeds.Insert(13.0);
		route.Insert(Vector(11155.4, 0, 14814.1)); speeds.Insert(14.7);
		route.Insert(Vector(11185.8, 0, 14809.6)); speeds.Insert(15.0);
		route.Insert(Vector(11216.2, 0, 14804.3)); speeds.Insert(13.5);
		route.Insert(Vector(11246.2, 0, 14799.8)); speeds.Insert(11.7);
		route.Insert(Vector(11276.4, 0, 14795.5)); speeds.Insert(11.9);
		route.Insert(Vector(11306.7, 0, 14791.8)); speeds.Insert(14.3);
		route.Insert(Vector(11337.3, 0, 14788.2)); speeds.Insert(15.1);
		route.Insert(Vector(11367.6, 0, 14783.9)); speeds.Insert(14.6);
		route.Insert(Vector(11397.9, 0, 14779.7)); speeds.Insert(12.3);
		route.Insert(Vector(11428.1, 0, 14775.8)); speeds.Insert(11.4);
		route.Insert(Vector(11458.3, 0, 14772.2)); speeds.Insert(13.2);
		route.Insert(Vector(11488.7, 0, 14769.1)); speeds.Insert(14.9);
		route.Insert(Vector(11519.4, 0, 14766.2)); speeds.Insert(14.9);
		route.Insert(Vector(11549.5, 0, 14760.8)); speeds.Insert(12.9);
		route.Insert(Vector(11579.3, 0, 14755.1)); speeds.Insert(11.4);
		route.Insert(Vector(11609.1, 0, 14749.3)); speeds.Insert(12.6);
		route.Insert(Vector(11639.3, 0, 14742.9)); speeds.Insert(14.7);
		route.Insert(Vector(11669.1, 0, 14736.3)); speeds.Insert(15.5);
		route.Insert(Vector(11699.0, 0, 14730.2)); speeds.Insert(13.9);
		route.Insert(Vector(11729.0, 0, 14724.8)); speeds.Insert(11.7);
		route.Insert(Vector(11759.0, 0, 14719.6)); speeds.Insert(11.5);
		route.Insert(Vector(11789.1, 0, 14714.7)); speeds.Insert(13.7);
		route.Insert(Vector(11819.4, 0, 14710.4)); speeds.Insert(15.0);
		route.Insert(Vector(11849.3, 0, 14705.2)); speeds.Insert(14.3);
		route.Insert(Vector(11879.3, 0, 14699.6)); speeds.Insert(12.8);
		route.Insert(Vector(11909.5, 0, 14694.9)); speeds.Insert(11.4);
		route.Insert(Vector(11939.5, 0, 14690.9)); speeds.Insert(12.5);
		route.Insert(Vector(11970.0, 0, 14687.8)); speeds.Insert(14.8);
		route.Insert(Vector(12000.5, 0, 14683.8)); speeds.Insert(15.2);
		route.Insert(Vector(12030.4, 0, 14677.0)); speeds.Insert(13.6);
		route.Insert(Vector(12060.1, 0, 14669.3)); speeds.Insert(11.7);
		route.Insert(Vector(12089.7, 0, 14662.1)); speeds.Insert(11.9);
		route.Insert(Vector(12119.5, 0, 14655.4)); speeds.Insert(14.8);
		route.Insert(Vector(12149.7, 0, 14650.4)); speeds.Insert(15.0);
		route.Insert(Vector(12180.2, 0, 14648.7)); speeds.Insert(14.6);
		route.Insert(Vector(12210.6, 0, 14646.2)); speeds.Insert(12.5);
		route.Insert(Vector(12240.8, 0, 14640.5)); speeds.Insert(11.2);
		route.Insert(Vector(12270.6, 0, 14634.1)); speeds.Insert(13.7);
		route.Insert(Vector(12300.4, 0, 14626.9)); speeds.Insert(14.9);
		route.Insert(Vector(12329.8, 0, 14619.1)); speeds.Insert(14.1);
		route.Insert(Vector(12359.2, 0, 14611.2)); speeds.Insert(12.5);
		route.Insert(Vector(12388.8, 0, 14604.1)); speeds.Insert(11.7);
		route.Insert(Vector(12418.6, 0, 14597.8)); speeds.Insert(12.8);
		route.Insert(Vector(12448.4, 0, 14591.8)); speeds.Insert(14.5);
		route.Insert(Vector(12478.5, 0, 14585.6)); speeds.Insert(14.5);
		route.Insert(Vector(12508.2, 0, 14578.7)); speeds.Insert(12.3);
		route.Insert(Vector(12537.8, 0, 14571.8)); speeds.Insert(10.9);
		route.Insert(Vector(12567.6, 0, 14565.8)); speeds.Insert(12.3);
		route.Insert(Vector(12597.6, 0, 14559.9)); speeds.Insert(14.2);
		route.Insert(Vector(12627.3, 0, 14552.1)); speeds.Insert(15.0);
		route.Insert(Vector(12657.2, 0, 14544.0)); speeds.Insert(14.0);
		route.Insert(Vector(12687.0, 0, 14537.4)); speeds.Insert(11.7);
		route.Insert(Vector(12716.6, 0, 14530.5)); speeds.Insert(11.0);
		route.Insert(Vector(12746.0, 0, 14522.1)); speeds.Insert(13.8);
		route.Insert(Vector(12774.6, 0, 14511.7)); speeds.Insert(15.3);
		route.Insert(Vector(12802.2, 0, 14499.0)); speeds.Insert(14.9);
		route.Insert(Vector(12829.3, 0, 14484.1)); speeds.Insert(12.5);
		route.Insert(Vector(12856.8, 0, 14471.3)); speeds.Insert(11.2);
		route.Insert(Vector(12884.6, 0, 14459.0)); speeds.Insert(12.8);
		route.Insert(Vector(12912.9, 0, 14447.3)); speeds.Insert(15.0);
		route.Insert(Vector(12941.5, 0, 14436.9)); speeds.Insert(15.7);
		route.Insert(Vector(12970.8, 0, 14427.6)); speeds.Insert(14.3);
		route.Insert(Vector(13000.0, 0, 14418.7)); speeds.Insert(12.4);
		route.Insert(Vector(13029.0, 0, 14409.9)); speeds.Insert(11.7);
		route.Insert(Vector(13058.2, 0, 14401.5)); speeds.Insert(13.9);
		route.Insert(Vector(13087.7, 0, 14394.1)); speeds.Insert(15.7);
		route.Insert(Vector(13117.6, 0, 14388.4)); speeds.Insert(14.9);
		route.Insert(Vector(13148.0, 0, 14384.4)); speeds.Insert(12.8);
		route.Insert(Vector(13178.4, 0, 14382.1)); speeds.Insert(11.5);
		route.Insert(Vector(13208.7, 0, 14381.0)); speeds.Insert(12.5);
		route.Insert(Vector(13239.2, 0, 14382.7)); speeds.Insert(14.2);
		route.Insert(Vector(13269.3, 0, 14388.3)); speeds.Insert(14.8);
		route.Insert(Vector(13297.7, 0, 14399.4)); speeds.Insert(13.6);
		route.Insert(Vector(13323.8, 0, 14415.2)); speeds.Insert(11.2);
		route.Insert(Vector(13348.1, 0, 14433.3)); speeds.Insert(11.4);
		route.Insert(Vector(13371.8, 0, 14452.7)); speeds.Insert(13.9);
		route.Insert(Vector(13392.8, 0, 14474.8)); speeds.Insert(14.7);
		route.Insert(Vector(13408.2, 0, 14501.2)); speeds.Insert(13.6);
		route.Insert(Vector(13421.6, 0, 14528.5)); speeds.Insert(11.5);
		route.Insert(Vector(13434.8, 0, 14555.8)); speeds.Insert(10.9);
		route.Insert(Vector(13446.4, 0, 14584.2)); speeds.Insert(13.5);
		route.Insert(Vector(13454.2, 0, 14613.7)); speeds.Insert(15.2);
		route.Insert(Vector(13458.1, 0, 14644.3)); speeds.Insert(15.3);
		route.Insert(Vector(13456.1, 0, 14674.8)); speeds.Insert(13.4);
		route.Insert(Vector(13448.3, 0, 14704.2)); speeds.Insert(11.8);
		route.Insert(Vector(13439.6, 0, 14733.7)); speeds.Insert(12.6);
		route.Insert(Vector(13431.1, 0, 14762.9)); speeds.Insert(14.6);
		route.Insert(Vector(13420.8, 0, 14791.5)); speeds.Insert(15.5);
		route.Insert(Vector(13405.4, 0, 14818.1)); speeds.Insert(14.0);
		route.Insert(Vector(13386.0, 0, 14841.6)); speeds.Insert(11.9);
		route.Insert(Vector(13366.6, 0, 14865.4)); speeds.Insert(12.4);
		route.Insert(Vector(13347.4, 0, 14889.2)); speeds.Insert(14.6);
		route.Insert(Vector(13345.5, 0, 14891.6)); speeds.Insert(14.6);
	}

	// Chernarus: the recorded route, a point every 8 m and the speed driven there (m/s). A function of its own per map: one
	// function with every map's points is over the script compiler's instruction limit
	static void DL_RouteChernarus(array<vector> route, array<float> speeds)
	{
		route.Insert(Vector(13222.7, 0, 10633.0)); speeds.Insert(7.8);
		route.Insert(Vector(13226.2, 0, 10641.5)); speeds.Insert(8.9);
		route.Insert(Vector(13229.7, 0, 10650.0)); speeds.Insert(9.8);
		route.Insert(Vector(13233.2, 0, 10658.5)); speeds.Insert(11.7);
		route.Insert(Vector(13236.7, 0, 10667.0)); speeds.Insert(13.0);
		route.Insert(Vector(13240.3, 0, 10675.7)); speeds.Insert(14.0);
		route.Insert(Vector(13243.8, 0, 10684.1)); speeds.Insert(14.9);
		route.Insert(Vector(13247.3, 0, 10692.7)); speeds.Insert(15.5);
		route.Insert(Vector(13250.8, 0, 10701.3)); speeds.Insert(16.0);
		route.Insert(Vector(13254.3, 0, 10709.9)); speeds.Insert(16.4);
		route.Insert(Vector(13257.9, 0, 10718.5)); speeds.Insert(16.8);
		route.Insert(Vector(13261.5, 0, 10727.2)); speeds.Insert(17.0);
		route.Insert(Vector(13265.0, 0, 10735.6)); speeds.Insert(17.1);
		route.Insert(Vector(13268.8, 0, 10744.3)); speeds.Insert(17.3);
		route.Insert(Vector(13272.6, 0, 10752.8)); speeds.Insert(17.6);
		route.Insert(Vector(13276.4, 0, 10761.2)); speeds.Insert(17.9);
		route.Insert(Vector(13280.4, 0, 10769.7)); speeds.Insert(18.0);
		route.Insert(Vector(13284.1, 0, 10778.2)); speeds.Insert(18.1);
		route.Insert(Vector(13287.9, 0, 10786.7)); speeds.Insert(18.2);
		route.Insert(Vector(13291.8, 0, 10795.3)); speeds.Insert(18.2);
		route.Insert(Vector(13295.7, 0, 10803.6)); speeds.Insert(18.1);
		route.Insert(Vector(13299.9, 0, 10812.1)); speeds.Insert(18.0);
		route.Insert(Vector(13304.1, 0, 10820.5)); speeds.Insert(18.1);
		route.Insert(Vector(13308.5, 0, 10828.8)); speeds.Insert(18.2);
		route.Insert(Vector(13313.3, 0, 10837.1)); speeds.Insert(18.3);
		route.Insert(Vector(13318.0, 0, 10845.0)); speeds.Insert(18.4);
		route.Insert(Vector(13322.8, 0, 10853.1)); speeds.Insert(18.5);
		route.Insert(Vector(13327.7, 0, 10861.0)); speeds.Insert(18.5);
		route.Insert(Vector(13332.7, 0, 10869.1)); speeds.Insert(18.4);
		route.Insert(Vector(13337.7, 0, 10877.0)); speeds.Insert(18.5);
		route.Insert(Vector(13342.5, 0, 10884.8)); speeds.Insert(18.7);
		route.Insert(Vector(13347.5, 0, 10892.5)); speeds.Insert(18.9);
		route.Insert(Vector(13352.6, 0, 10900.7)); speeds.Insert(19.0);
		route.Insert(Vector(13357.6, 0, 10908.8)); speeds.Insert(19.1);
		route.Insert(Vector(13362.6, 0, 10916.9)); speeds.Insert(19.1);
		route.Insert(Vector(13367.5, 0, 10924.9)); speeds.Insert(19.0);
		route.Insert(Vector(13372.5, 0, 10932.9)); speeds.Insert(19.0);
		route.Insert(Vector(13377.5, 0, 10941.0)); speeds.Insert(19.2);
		route.Insert(Vector(13382.3, 0, 10948.8)); speeds.Insert(19.4);
		route.Insert(Vector(13387.3, 0, 10957.0)); speeds.Insert(19.5);
		route.Insert(Vector(13392.3, 0, 10965.0)); speeds.Insert(19.4);
		route.Insert(Vector(13397.2, 0, 10973.0)); speeds.Insert(19.2);
		route.Insert(Vector(13401.9, 0, 10980.7)); speeds.Insert(19.0);
		route.Insert(Vector(13406.7, 0, 10988.5)); speeds.Insert(18.9);
		route.Insert(Vector(13411.6, 0, 10996.3)); speeds.Insert(19.0);
		route.Insert(Vector(13416.4, 0, 11004.3)); speeds.Insert(19.2);
		route.Insert(Vector(13420.8, 0, 11012.4)); speeds.Insert(19.6);
		route.Insert(Vector(13424.7, 0, 11020.7)); speeds.Insert(19.9);
		route.Insert(Vector(13428.3, 0, 11029.5)); speeds.Insert(20.1);
		route.Insert(Vector(13431.8, 0, 11038.3)); speeds.Insert(20.0);
		route.Insert(Vector(13435.4, 0, 11046.8)); speeds.Insert(19.9);
		route.Insert(Vector(13439.0, 0, 11055.4)); speeds.Insert(19.8);
		route.Insert(Vector(13442.5, 0, 11063.7)); speeds.Insert(19.9);
		route.Insert(Vector(13446.1, 0, 11072.2)); speeds.Insert(20.1);
		route.Insert(Vector(13449.7, 0, 11080.9)); speeds.Insert(20.4);
		route.Insert(Vector(13453.4, 0, 11089.6)); speeds.Insert(20.6);
		route.Insert(Vector(13457.1, 0, 11098.3)); speeds.Insert(20.7);
		route.Insert(Vector(13460.8, 0, 11107.1)); speeds.Insert(20.5);
		route.Insert(Vector(13464.5, 0, 11115.8)); speeds.Insert(20.3);
		route.Insert(Vector(13468.0, 0, 11124.5)); speeds.Insert(20.1);
		route.Insert(Vector(13471.2, 0, 11133.1)); speeds.Insert(20.0);
		route.Insert(Vector(13474.3, 0, 11142.0)); speeds.Insert(20.0);
		route.Insert(Vector(13477.4, 0, 11151.0)); speeds.Insert(19.9);
		route.Insert(Vector(13480.7, 0, 11160.2)); speeds.Insert(19.7);
		route.Insert(Vector(13483.9, 0, 11169.1)); speeds.Insert(19.5);
		route.Insert(Vector(13487.1, 0, 11178.0)); speeds.Insert(19.3);
		route.Insert(Vector(13490.3, 0, 11186.7)); speeds.Insert(19.2);
		route.Insert(Vector(13493.4, 0, 11195.3)); speeds.Insert(19.3);
		route.Insert(Vector(13496.6, 0, 11204.1)); speeds.Insert(19.7);
		route.Insert(Vector(13499.8, 0, 11213.0)); speeds.Insert(20.1);
		route.Insert(Vector(13503.2, 0, 11222.1)); speeds.Insert(20.4);
		route.Insert(Vector(13506.4, 0, 11230.9)); speeds.Insert(20.5);
		route.Insert(Vector(13509.5, 0, 11239.6)); speeds.Insert(20.5);
		route.Insert(Vector(13512.9, 0, 11248.5)); speeds.Insert(20.6);
		route.Insert(Vector(13516.3, 0, 11257.2)); speeds.Insert(20.6);
		route.Insert(Vector(13520.1, 0, 11265.7)); speeds.Insert(20.8);
		route.Insert(Vector(13524.1, 0, 11274.5)); speeds.Insert(21.0);
		route.Insert(Vector(13528.0, 0, 11282.9)); speeds.Insert(21.2);
		route.Insert(Vector(13532.1, 0, 11291.5)); speeds.Insert(21.4);
		route.Insert(Vector(13536.2, 0, 11300.2)); speeds.Insert(21.3);
		route.Insert(Vector(13540.5, 0, 11308.4)); speeds.Insert(21.2);
		route.Insert(Vector(13544.9, 0, 11316.7)); speeds.Insert(21.1);
		route.Insert(Vector(13549.3, 0, 11324.8)); speeds.Insert(21.0);
		route.Insert(Vector(13553.7, 0, 11332.9)); speeds.Insert(20.8);
		route.Insert(Vector(13558.1, 0, 11341.0)); speeds.Insert(20.6);
		route.Insert(Vector(13562.3, 0, 11349.3)); speeds.Insert(20.4);
		route.Insert(Vector(13566.4, 0, 11357.8)); speeds.Insert(20.0);
		route.Insert(Vector(13570.2, 0, 11366.5)); speeds.Insert(19.7);
		route.Insert(Vector(13573.6, 0, 11375.1)); speeds.Insert(19.3);
		route.Insert(Vector(13576.9, 0, 11383.5)); speeds.Insert(18.9);
		route.Insert(Vector(13580.3, 0, 11392.1)); speeds.Insert(18.5);
		route.Insert(Vector(13583.6, 0, 11400.8)); speeds.Insert(18.0);
		route.Insert(Vector(13586.5, 0, 11409.9)); speeds.Insert(17.4);
		route.Insert(Vector(13589.3, 0, 11418.8)); speeds.Insert(16.8);
		route.Insert(Vector(13591.5, 0, 11428.0)); speeds.Insert(16.0);
		route.Insert(Vector(13592.0, 0, 11437.2)); speeds.Insert(15.2);
		route.Insert(Vector(13590.5, 0, 11446.3)); speeds.Insert(14.3);
		route.Insert(Vector(13586.6, 0, 11454.5)); speeds.Insert(13.4);
		route.Insert(Vector(13580.1, 0, 11461.0)); speeds.Insert(12.6);
		route.Insert(Vector(13571.7, 0, 11465.1)); speeds.Insert(11.9);
		route.Insert(Vector(13563.2, 0, 11468.8)); speeds.Insert(11.2);
		route.Insert(Vector(13554.7, 0, 11472.6)); speeds.Insert(10.7);
		route.Insert(Vector(13547.9, 0, 11478.4)); speeds.Insert(10.4);
		route.Insert(Vector(13545.6, 0, 11487.1)); speeds.Insert(10.1);
		route.Insert(Vector(13546.9, 0, 11496.2)); speeds.Insert(10.0);
		route.Insert(Vector(13549.1, 0, 11505.1)); speeds.Insert(10.3);
		route.Insert(Vector(13551.7, 0, 11514.0)); speeds.Insert(10.6);
		route.Insert(Vector(13554.1, 0, 11522.8)); speeds.Insert(10.9);
		route.Insert(Vector(13556.0, 0, 11531.8)); speeds.Insert(11.5);
		route.Insert(Vector(13557.5, 0, 11541.0)); speeds.Insert(12.1);
		route.Insert(Vector(13558.2, 0, 11550.2)); speeds.Insert(12.7);
		route.Insert(Vector(13558.3, 0, 11559.5)); speeds.Insert(13.2);
		route.Insert(Vector(13558.2, 0, 11568.8)); speeds.Insert(13.6);
		route.Insert(Vector(13558.1, 0, 11578.3)); speeds.Insert(13.9);
		route.Insert(Vector(13558.0, 0, 11587.6)); speeds.Insert(14.2);
		route.Insert(Vector(13557.9, 0, 11597.0)); speeds.Insert(14.4);
		route.Insert(Vector(13557.8, 0, 11606.3)); speeds.Insert(14.7);
		route.Insert(Vector(13557.7, 0, 11615.5)); speeds.Insert(15.0);
		route.Insert(Vector(13557.4, 0, 11624.8)); speeds.Insert(15.2);
		route.Insert(Vector(13556.7, 0, 11634.0)); speeds.Insert(15.1);
		route.Insert(Vector(13555.6, 0, 11643.2)); speeds.Insert(14.9);
		route.Insert(Vector(13554.4, 0, 11652.3)); speeds.Insert(14.4);
		route.Insert(Vector(13553.3, 0, 11661.7)); speeds.Insert(13.9);
		route.Insert(Vector(13552.1, 0, 11670.9)); speeds.Insert(13.3);
		route.Insert(Vector(13551.2, 0, 11680.0)); speeds.Insert(12.8);
		route.Insert(Vector(13551.8, 0, 11689.1)); speeds.Insert(12.3);
		route.Insert(Vector(13554.3, 0, 11697.8)); speeds.Insert(11.8);
		route.Insert(Vector(13557.3, 0, 11706.6)); speeds.Insert(11.3);
		route.Insert(Vector(13560.0, 0, 11715.3)); speeds.Insert(11.0);
		route.Insert(Vector(13561.8, 0, 11724.3)); speeds.Insert(10.7);
		route.Insert(Vector(13564.2, 0, 11733.2)); speeds.Insert(10.6);
		route.Insert(Vector(13568.0, 0, 11741.5)); speeds.Insert(10.8);
		route.Insert(Vector(13574.0, 0, 11748.3)); speeds.Insert(11.2);
		route.Insert(Vector(13579.8, 0, 11755.3)); speeds.Insert(11.7);
		route.Insert(Vector(13585.4, 0, 11762.7)); speeds.Insert(12.2);
		route.Insert(Vector(13590.9, 0, 11770.0)); speeds.Insert(12.7);
		route.Insert(Vector(13596.5, 0, 11777.3)); speeds.Insert(13.0);
		route.Insert(Vector(13602.3, 0, 11784.3)); speeds.Insert(12.8);
		route.Insert(Vector(13608.2, 0, 11791.6)); speeds.Insert(12.6);
		route.Insert(Vector(13614.1, 0, 11798.8)); speeds.Insert(12.4);
		route.Insert(Vector(13618.9, 0, 11806.8)); speeds.Insert(12.1);
		route.Insert(Vector(13623.5, 0, 11814.8)); speeds.Insert(11.8);
		route.Insert(Vector(13629.0, 0, 11822.2)); speeds.Insert(11.8);
		route.Insert(Vector(13636.6, 0, 11827.4)); speeds.Insert(11.8);
		route.Insert(Vector(13645.1, 0, 11830.3)); speeds.Insert(11.9);
		route.Insert(Vector(13653.6, 0, 11834.1)); speeds.Insert(12.3);
		route.Insert(Vector(13661.2, 0, 11839.2)); speeds.Insert(12.8);
		route.Insert(Vector(13669.0, 0, 11844.3)); speeds.Insert(13.6);
		route.Insert(Vector(13676.8, 0, 11849.2)); speeds.Insert(14.5);
		route.Insert(Vector(13684.6, 0, 11854.1)); speeds.Insert(15.5);
		route.Insert(Vector(13692.4, 0, 11858.9)); speeds.Insert(16.3);
		route.Insert(Vector(13700.3, 0, 11863.6)); speeds.Insert(17.1);
		route.Insert(Vector(13708.3, 0, 11868.4)); speeds.Insert(17.8);
		route.Insert(Vector(13716.2, 0, 11873.2)); speeds.Insert(18.4);
		route.Insert(Vector(13724.1, 0, 11878.0)); speeds.Insert(18.7);
		route.Insert(Vector(13732.0, 0, 11882.9)); speeds.Insert(18.8);
		route.Insert(Vector(13739.8, 0, 11887.7)); speeds.Insert(18.7);
		route.Insert(Vector(13747.9, 0, 11892.7)); speeds.Insert(18.4);
		route.Insert(Vector(13755.9, 0, 11897.6)); speeds.Insert(18.1);
		route.Insert(Vector(13763.8, 0, 11902.5)); speeds.Insert(18.0);
		route.Insert(Vector(13771.8, 0, 11907.4)); speeds.Insert(18.1);
		route.Insert(Vector(13779.7, 0, 11912.3)); speeds.Insert(18.5);
		route.Insert(Vector(13787.6, 0, 11917.2)); speeds.Insert(18.8);
		route.Insert(Vector(13795.2, 0, 11922.6)); speeds.Insert(19.2);
		route.Insert(Vector(13802.4, 0, 11928.3)); speeds.Insert(19.3);
		route.Insert(Vector(13809.4, 0, 11934.1)); speeds.Insert(19.3);
		route.Insert(Vector(13816.6, 0, 11940.1)); speeds.Insert(19.2);
		route.Insert(Vector(13823.6, 0, 11945.9)); speeds.Insert(19.1);
		route.Insert(Vector(13830.8, 0, 11951.9)); speeds.Insert(19.0);
		route.Insert(Vector(13837.9, 0, 11957.7)); speeds.Insert(18.9);
		route.Insert(Vector(13844.8, 0, 11963.5)); speeds.Insert(18.8);
		route.Insert(Vector(13851.9, 0, 11969.4)); speeds.Insert(18.8);
		route.Insert(Vector(13859.1, 0, 11975.3)); speeds.Insert(18.9);
		route.Insert(Vector(13866.3, 0, 11981.4)); speeds.Insert(18.9);
		route.Insert(Vector(13873.3, 0, 11987.7)); speeds.Insert(18.8);
		route.Insert(Vector(13880.1, 0, 11994.4)); speeds.Insert(18.7);
		route.Insert(Vector(13886.4, 0, 12001.3)); speeds.Insert(18.6);
		route.Insert(Vector(13892.7, 0, 12008.2)); speeds.Insert(18.5);
		route.Insert(Vector(13898.9, 0, 12015.1)); speeds.Insert(18.6);
		route.Insert(Vector(13905.0, 0, 12021.9)); speeds.Insert(18.7);
		route.Insert(Vector(13911.3, 0, 12028.9)); speeds.Insert(18.9);
		route.Insert(Vector(13917.6, 0, 12035.9)); speeds.Insert(19.1);
		route.Insert(Vector(13923.9, 0, 12042.8)); speeds.Insert(19.2);
		route.Insert(Vector(13930.0, 0, 12049.8)); speeds.Insert(19.2);
		route.Insert(Vector(13935.8, 0, 12057.2)); speeds.Insert(19.1);
		route.Insert(Vector(13941.0, 0, 12064.7)); speeds.Insert(18.9);
		route.Insert(Vector(13946.2, 0, 12072.2)); speeds.Insert(18.8);
		route.Insert(Vector(13951.4, 0, 12079.8)); speeds.Insert(18.8);
		route.Insert(Vector(13956.6, 0, 12087.4)); speeds.Insert(19.0);
		route.Insert(Vector(13962.0, 0, 12095.2)); speeds.Insert(19.1);
		route.Insert(Vector(13967.2, 0, 12102.9)); speeds.Insert(19.3);
		route.Insert(Vector(13972.4, 0, 12110.5)); speeds.Insert(19.3);
		route.Insert(Vector(13977.1, 0, 12118.6)); speeds.Insert(19.2);
		route.Insert(Vector(13981.1, 0, 12127.0)); speeds.Insert(19.0);
		route.Insert(Vector(13984.2, 0, 12135.6)); speeds.Insert(18.8);
		route.Insert(Vector(13987.2, 0, 12144.6)); speeds.Insert(18.8);
		route.Insert(Vector(13990.0, 0, 12153.6)); speeds.Insert(18.7);
		route.Insert(Vector(13992.9, 0, 12162.4)); speeds.Insert(18.6);
		route.Insert(Vector(13995.8, 0, 12171.4)); speeds.Insert(18.5);
		route.Insert(Vector(13998.7, 0, 12180.2)); speeds.Insert(18.3);
		route.Insert(Vector(14001.1, 0, 12189.0)); speeds.Insert(18.4);
		route.Insert(Vector(14002.8, 0, 12198.3)); speeds.Insert(18.6);
		route.Insert(Vector(14003.9, 0, 12207.5)); speeds.Insert(19.0);
		route.Insert(Vector(14004.9, 0, 12217.0)); speeds.Insert(19.5);
		route.Insert(Vector(14005.6, 0, 12226.4)); speeds.Insert(20.0);
		route.Insert(Vector(14006.3, 0, 12235.6)); speeds.Insert(20.5);
		route.Insert(Vector(14007.1, 0, 12245.1)); speeds.Insert(20.9);
		route.Insert(Vector(14007.9, 0, 12254.8)); speeds.Insert(21.0);
		route.Insert(Vector(14008.7, 0, 12264.4)); speeds.Insert(20.9);
		route.Insert(Vector(14009.5, 0, 12273.9)); speeds.Insert(20.6);
		route.Insert(Vector(14010.3, 0, 12283.1)); speeds.Insert(20.3);
		route.Insert(Vector(14011.1, 0, 12292.5)); speeds.Insert(20.1);
		route.Insert(Vector(14012.0, 0, 12301.8)); speeds.Insert(20.0);
		route.Insert(Vector(14012.7, 0, 12310.9)); speeds.Insert(19.9);
		route.Insert(Vector(14013.5, 0, 12320.2)); speeds.Insert(19.9);
		route.Insert(Vector(14014.2, 0, 12329.4)); speeds.Insert(19.8);
		route.Insert(Vector(14015.0, 0, 12338.8)); speeds.Insert(19.5);
		route.Insert(Vector(14015.7, 0, 12348.1)); speeds.Insert(19.1);
		route.Insert(Vector(14016.4, 0, 12357.5)); speeds.Insert(18.8);
		route.Insert(Vector(14017.2, 0, 12367.0)); speeds.Insert(18.5);
		route.Insert(Vector(14017.9, 0, 12376.4)); speeds.Insert(18.3);
		route.Insert(Vector(14018.7, 0, 12385.7)); speeds.Insert(18.2);
		route.Insert(Vector(14019.4, 0, 12395.0)); speeds.Insert(18.0);
		route.Insert(Vector(14020.1, 0, 12404.0)); speeds.Insert(18.0);
		route.Insert(Vector(14020.9, 0, 12413.1)); speeds.Insert(18.0);
		route.Insert(Vector(14021.6, 0, 12422.4)); speeds.Insert(17.9);
		route.Insert(Vector(14022.3, 0, 12431.7)); speeds.Insert(17.9);
		route.Insert(Vector(14023.0, 0, 12441.1)); speeds.Insert(18.1);
		route.Insert(Vector(14023.6, 0, 12450.5)); speeds.Insert(18.2);
		route.Insert(Vector(14024.3, 0, 12459.9)); speeds.Insert(18.1);
		route.Insert(Vector(14025.0, 0, 12469.1)); speeds.Insert(18.0);
		route.Insert(Vector(14025.7, 0, 12478.4)); speeds.Insert(17.9);
		route.Insert(Vector(14026.4, 0, 12487.8)); speeds.Insert(17.9);
		route.Insert(Vector(14027.1, 0, 12497.2)); speeds.Insert(18.1);
		route.Insert(Vector(14027.8, 0, 12506.4)); speeds.Insert(18.3);
		route.Insert(Vector(14028.5, 0, 12516.0)); speeds.Insert(18.6);
		route.Insert(Vector(14029.2, 0, 12525.5)); speeds.Insert(18.7);
		route.Insert(Vector(14029.9, 0, 12534.9)); speeds.Insert(18.7);
		route.Insert(Vector(14030.6, 0, 12544.1)); speeds.Insert(18.6);
		route.Insert(Vector(14031.2, 0, 12553.3)); speeds.Insert(18.6);
		route.Insert(Vector(14032.0, 0, 12562.7)); speeds.Insert(18.5);
		route.Insert(Vector(14032.8, 0, 12572.0)); speeds.Insert(18.4);
		route.Insert(Vector(14033.6, 0, 12581.1)); speeds.Insert(18.4);
		route.Insert(Vector(14034.3, 0, 12590.4)); speeds.Insert(18.3);
		route.Insert(Vector(14035.2, 0, 12599.6)); speeds.Insert(18.5);
		route.Insert(Vector(14036.4, 0, 12608.7)); speeds.Insert(18.7);
		route.Insert(Vector(14037.9, 0, 12618.0)); speeds.Insert(19.0);
		route.Insert(Vector(14039.3, 0, 12627.1)); speeds.Insert(19.2);
		route.Insert(Vector(14040.7, 0, 12636.4)); speeds.Insert(19.3);
		route.Insert(Vector(14042.2, 0, 12645.9)); speeds.Insert(19.3);
		route.Insert(Vector(14043.9, 0, 12655.4)); speeds.Insert(19.2);
		route.Insert(Vector(14046.0, 0, 12664.4)); speeds.Insert(19.2);
		route.Insert(Vector(14048.8, 0, 12673.5)); speeds.Insert(19.4);
		route.Insert(Vector(14052.3, 0, 12682.5)); speeds.Insert(19.4);
		route.Insert(Vector(14055.7, 0, 12691.1)); speeds.Insert(19.5);
		route.Insert(Vector(14059.1, 0, 12699.7)); speeds.Insert(19.5);
		route.Insert(Vector(14062.6, 0, 12708.5)); speeds.Insert(19.5);
		route.Insert(Vector(14066.0, 0, 12717.1)); speeds.Insert(19.3);
		route.Insert(Vector(14069.6, 0, 12725.9)); speeds.Insert(19.2);
		route.Insert(Vector(14073.3, 0, 12734.6)); speeds.Insert(19.0);
		route.Insert(Vector(14077.4, 0, 12743.4)); speeds.Insert(18.8);
		route.Insert(Vector(14081.4, 0, 12751.9)); speeds.Insert(18.5);
		route.Insert(Vector(14085.4, 0, 12760.4)); speeds.Insert(18.1);
		route.Insert(Vector(14089.3, 0, 12768.8)); speeds.Insert(17.6);
		route.Insert(Vector(14093.4, 0, 12777.3)); speeds.Insert(17.3);
		route.Insert(Vector(14097.5, 0, 12785.9)); speeds.Insert(17.2);
		route.Insert(Vector(14101.8, 0, 12794.3)); speeds.Insert(17.2);
		route.Insert(Vector(14106.0, 0, 12802.6)); speeds.Insert(17.2);
		route.Insert(Vector(14110.4, 0, 12810.8)); speeds.Insert(17.3);
		route.Insert(Vector(14114.9, 0, 12819.2)); speeds.Insert(17.3);
		route.Insert(Vector(14119.4, 0, 12827.3)); speeds.Insert(17.1);
		route.Insert(Vector(14123.9, 0, 12835.8)); speeds.Insert(16.9);
		route.Insert(Vector(14128.4, 0, 12844.1)); speeds.Insert(16.8);
		route.Insert(Vector(14132.8, 0, 12852.2)); speeds.Insert(16.6);
		route.Insert(Vector(14137.3, 0, 12860.4)); speeds.Insert(16.5);
		route.Insert(Vector(14141.7, 0, 12868.5)); speeds.Insert(16.5);
		route.Insert(Vector(14144.7, 0, 12874.0)); speeds.Insert(16.4);
	}

	// Livonia: the recorded route, a point every 8 m and the speed driven there (m/s). A function of its own per map: one
	// function with every map's points is over the script compiler's instruction limit
	static void DL_RouteLivonia(array<vector> route, array<float> speeds)
	{
		route.Insert(Vector(5207.7, 0, 1677.2)); speeds.Insert(3.0);
		route.Insert(Vector(5214.0, 0, 1672.2)); speeds.Insert(5.6);
		route.Insert(Vector(5220.2, 0, 1667.2)); speeds.Insert(6.3);
		route.Insert(Vector(5226.4, 0, 1662.1)); speeds.Insert(7.8);
		route.Insert(Vector(5232.6, 0, 1657.1)); speeds.Insert(9.0);
		route.Insert(Vector(5239.0, 0, 1652.3)); speeds.Insert(9.6);
		route.Insert(Vector(5245.5, 0, 1647.6)); speeds.Insert(9.7);
		route.Insert(Vector(5251.9, 0, 1642.8)); speeds.Insert(9.9);
		route.Insert(Vector(5258.4, 0, 1638.1)); speeds.Insert(10.4);
		route.Insert(Vector(5264.8, 0, 1633.3)); speeds.Insert(10.8);
		route.Insert(Vector(5271.2, 0, 1628.5)); speeds.Insert(11.2);
		route.Insert(Vector(5277.6, 0, 1623.8)); speeds.Insert(11.4);
		route.Insert(Vector(5284.0, 0, 1619.0)); speeds.Insert(11.5);
		route.Insert(Vector(5290.4, 0, 1614.2)); speeds.Insert(12.3);
		route.Insert(Vector(5296.8, 0, 1609.4)); speeds.Insert(13.0);
		route.Insert(Vector(5303.2, 0, 1604.5)); speeds.Insert(13.4);
		route.Insert(Vector(5309.5, 0, 1599.5)); speeds.Insert(13.8);
		route.Insert(Vector(5315.7, 0, 1594.6)); speeds.Insert(14.3);
		route.Insert(Vector(5322.0, 0, 1589.6)); speeds.Insert(14.7);
		route.Insert(Vector(5328.2, 0, 1584.6)); speeds.Insert(15.2);
		route.Insert(Vector(5334.4, 0, 1579.6)); speeds.Insert(15.6);
		route.Insert(Vector(5340.7, 0, 1574.6)); speeds.Insert(16.0);
		route.Insert(Vector(5346.9, 0, 1569.6)); speeds.Insert(16.4);
		route.Insert(Vector(5353.2, 0, 1564.6)); speeds.Insert(16.8);
		route.Insert(Vector(5359.5, 0, 1559.7)); speeds.Insert(17.1);
		route.Insert(Vector(5365.8, 0, 1554.7)); speeds.Insert(17.2);
		route.Insert(Vector(5372.1, 0, 1549.7)); speeds.Insert(17.3);
		route.Insert(Vector(5378.3, 0, 1544.7)); speeds.Insert(17.2);
		route.Insert(Vector(5384.5, 0, 1539.7)); speeds.Insert(17.2);
		route.Insert(Vector(5390.7, 0, 1534.7)); speeds.Insert(17.3);
		route.Insert(Vector(5397.0, 0, 1529.7)); speeds.Insert(17.4);
		route.Insert(Vector(5403.3, 0, 1524.8)); speeds.Insert(17.5);
		route.Insert(Vector(5409.6, 0, 1519.8)); speeds.Insert(17.5);
		route.Insert(Vector(5416.0, 0, 1515.0)); speeds.Insert(17.6);
		route.Insert(Vector(5422.3, 0, 1510.1)); speeds.Insert(17.7);
		route.Insert(Vector(5428.7, 0, 1505.3)); speeds.Insert(17.7);
		route.Insert(Vector(5435.1, 0, 1500.5)); speeds.Insert(17.8);
		route.Insert(Vector(5441.5, 0, 1495.8)); speeds.Insert(17.6);
		route.Insert(Vector(5448.0, 0, 1491.1)); speeds.Insert(17.5);
		route.Insert(Vector(5454.6, 0, 1486.5)); speeds.Insert(17.2);
		route.Insert(Vector(5461.1, 0, 1481.8)); speeds.Insert(16.9);
		route.Insert(Vector(5467.6, 0, 1477.2)); speeds.Insert(16.6);
		route.Insert(Vector(5473.8, 0, 1472.2)); speeds.Insert(16.3);
		route.Insert(Vector(5479.6, 0, 1466.7)); speeds.Insert(16.0);
		route.Insert(Vector(5484.9, 0, 1460.6)); speeds.Insert(15.7);
		route.Insert(Vector(5489.7, 0, 1454.2)); speeds.Insert(15.4);
		route.Insert(Vector(5494.3, 0, 1447.7)); speeds.Insert(15.3);
		route.Insert(Vector(5498.5, 0, 1440.9)); speeds.Insert(15.2);
		route.Insert(Vector(5502.4, 0, 1433.9)); speeds.Insert(15.2);
		route.Insert(Vector(5506.4, 0, 1427.0)); speeds.Insert(15.1);
		route.Insert(Vector(5510.5, 0, 1420.1)); speeds.Insert(15.1);
		route.Insert(Vector(5514.9, 0, 1413.4)); speeds.Insert(14.9);
		route.Insert(Vector(5519.5, 0, 1406.9)); speeds.Insert(14.6);
		route.Insert(Vector(5524.3, 0, 1400.5)); speeds.Insert(14.4);
		route.Insert(Vector(5529.3, 0, 1394.2)); speeds.Insert(14.2);
		route.Insert(Vector(5534.3, 0, 1388.0)); speeds.Insert(14.0);
		route.Insert(Vector(5539.4, 0, 1381.8)); speeds.Insert(13.6);
		route.Insert(Vector(5544.5, 0, 1375.6)); speeds.Insert(12.4);
		route.Insert(Vector(5549.5, 0, 1369.4)); speeds.Insert(11.0);
		route.Insert(Vector(5554.6, 0, 1363.2)); speeds.Insert(10.3);
		route.Insert(Vector(5559.5, 0, 1356.9)); speeds.Insert(9.8);
		route.Insert(Vector(5564.1, 0, 1350.4)); speeds.Insert(9.5);
		route.Insert(Vector(5568.6, 0, 1343.8)); speeds.Insert(9.2);
		route.Insert(Vector(5572.9, 0, 1337.0)); speeds.Insert(8.6);
		route.Insert(Vector(5577.1, 0, 1330.3)); speeds.Insert(5.8);
		route.Insert(Vector(5581.4, 0, 1323.5)); speeds.Insert(5.3);
		route.Insert(Vector(5585.6, 0, 1316.7)); speeds.Insert(2.7);
		route.Insert(Vector(5590.0, 0, 1310.0)); speeds.Insert(2.8);
		route.Insert(Vector(5594.7, 0, 1303.5)); speeds.Insert(2.7);
		route.Insert(Vector(5599.4, 0, 1297.1)); speeds.Insert(2.7);
		route.Insert(Vector(5604.2, 0, 1290.7)); speeds.Insert(2.7);
		route.Insert(Vector(5608.7, 0, 1284.0)); speeds.Insert(2.7);
		route.Insert(Vector(5612.5, 0, 1277.0)); speeds.Insert(2.8);
		route.Insert(Vector(5615.9, 0, 1269.7)); speeds.Insert(3.0);
		route.Insert(Vector(5618.8, 0, 1262.3)); speeds.Insert(4.5);
		route.Insert(Vector(5621.0, 0, 1254.6)); speeds.Insert(5.3);
		route.Insert(Vector(5622.7, 0, 1246.8)); speeds.Insert(5.7);
		route.Insert(Vector(5624.1, 0, 1238.9)); speeds.Insert(6.3);
		route.Insert(Vector(5625.4, 0, 1231.0)); speeds.Insert(7.4);
		route.Insert(Vector(5626.4, 0, 1223.1)); speeds.Insert(8.2);
		route.Insert(Vector(5627.3, 0, 1215.1)); speeds.Insert(8.9);
		route.Insert(Vector(5628.0, 0, 1207.2)); speeds.Insert(9.6);
		route.Insert(Vector(5628.4, 0, 1199.2)); speeds.Insert(9.9);
		route.Insert(Vector(5628.4, 0, 1191.2)); speeds.Insert(10.4);
		route.Insert(Vector(5628.4, 0, 1183.2)); speeds.Insert(10.6);
		route.Insert(Vector(5628.4, 0, 1175.2)); speeds.Insert(10.6);
		route.Insert(Vector(5628.9, 0, 1167.2)); speeds.Insert(11.2);
		route.Insert(Vector(5629.4, 0, 1159.2)); speeds.Insert(11.8);
		route.Insert(Vector(5629.8, 0, 1151.2)); speeds.Insert(12.2);
		route.Insert(Vector(5630.2, 0, 1143.2)); speeds.Insert(12.3);
		route.Insert(Vector(5630.5, 0, 1135.2)); speeds.Insert(12.5);
		route.Insert(Vector(5630.9, 0, 1127.2)); speeds.Insert(12.5);
		route.Insert(Vector(5631.6, 0, 1119.3)); speeds.Insert(12.4);
		route.Insert(Vector(5632.7, 0, 1111.4)); speeds.Insert(12.4);
		route.Insert(Vector(5634.3, 0, 1103.5)); speeds.Insert(12.3);
		route.Insert(Vector(5636.1, 0, 1095.7)); speeds.Insert(12.5);
		route.Insert(Vector(5638.2, 0, 1088.0)); speeds.Insert(12.8);
		route.Insert(Vector(5641.1, 0, 1080.6)); speeds.Insert(13.0);
		route.Insert(Vector(5644.5, 0, 1073.3)); speeds.Insert(13.4);
		route.Insert(Vector(5648.1, 0, 1066.2)); speeds.Insert(13.9);
		route.Insert(Vector(5652.3, 0, 1059.4)); speeds.Insert(14.4);
		route.Insert(Vector(5656.8, 0, 1052.8)); speeds.Insert(14.7);
		route.Insert(Vector(5661.8, 0, 1046.5)); speeds.Insert(15.1);
		route.Insert(Vector(5667.3, 0, 1040.7)); speeds.Insert(15.3);
		route.Insert(Vector(5672.9, 0, 1035.0)); speeds.Insert(15.1);
		route.Insert(Vector(5678.6, 0, 1029.4)); speeds.Insert(14.8);
		route.Insert(Vector(5684.1, 0, 1023.6)); speeds.Insert(14.5);
		route.Insert(Vector(5689.6, 0, 1017.8)); speeds.Insert(14.1);
		route.Insert(Vector(5695.0, 0, 1011.9)); speeds.Insert(13.7);
		route.Insert(Vector(5700.4, 0, 1006.0)); speeds.Insert(13.3);
		route.Insert(Vector(5705.8, 0, 1000.0)); speeds.Insert(12.8);
		route.Insert(Vector(5711.0, 0, 994.0)); speeds.Insert(11.5);
		route.Insert(Vector(5716.3, 0, 988.0)); speeds.Insert(10.3);
		route.Insert(Vector(5721.6, 0, 982.0)); speeds.Insert(9.8);
		route.Insert(Vector(5726.9, 0, 976.0)); speeds.Insert(9.4);
		route.Insert(Vector(5732.2, 0, 970.0)); speeds.Insert(9.1);
		route.Insert(Vector(5737.7, 0, 964.2)); speeds.Insert(8.9);
		route.Insert(Vector(5743.2, 0, 958.4)); speeds.Insert(8.9);
		route.Insert(Vector(5748.8, 0, 952.6)); speeds.Insert(9.0);
		route.Insert(Vector(5754.3, 0, 946.8)); speeds.Insert(9.3);
		route.Insert(Vector(5759.8, 0, 941.1)); speeds.Insert(9.4);
		route.Insert(Vector(5765.4, 0, 935.3)); speeds.Insert(9.5);
		route.Insert(Vector(5771.0, 0, 929.6)); speeds.Insert(9.5);
		route.Insert(Vector(5776.6, 0, 923.9)); speeds.Insert(9.5);
		route.Insert(Vector(5782.3, 0, 918.3)); speeds.Insert(9.5);
		route.Insert(Vector(5788.0, 0, 912.6)); speeds.Insert(9.6);
		route.Insert(Vector(5793.7, 0, 907.0)); speeds.Insert(9.7);
		route.Insert(Vector(5799.3, 0, 901.4)); speeds.Insert(9.7);
		route.Insert(Vector(5805.0, 0, 895.8)); speeds.Insert(9.8);
		route.Insert(Vector(5810.7, 0, 890.2)); speeds.Insert(9.9);
		route.Insert(Vector(5816.5, 0, 884.6)); speeds.Insert(10.1);
		route.Insert(Vector(5822.3, 0, 879.1)); speeds.Insert(10.3);
		route.Insert(Vector(5828.0, 0, 873.5)); speeds.Insert(10.7);
		route.Insert(Vector(5833.7, 0, 867.9)); speeds.Insert(11.0);
		route.Insert(Vector(5839.5, 0, 862.3)); speeds.Insert(11.4);
		route.Insert(Vector(5845.2, 0, 856.7)); speeds.Insert(11.7);
		route.Insert(Vector(5851.0, 0, 851.2)); speeds.Insert(11.9);
		route.Insert(Vector(5856.7, 0, 845.7)); speeds.Insert(12.2);
		route.Insert(Vector(5862.5, 0, 840.2)); speeds.Insert(12.8);
		route.Insert(Vector(5868.3, 0, 834.6)); speeds.Insert(13.3);
		route.Insert(Vector(5874.1, 0, 829.1)); speeds.Insert(13.8);
		route.Insert(Vector(5880.0, 0, 823.7)); speeds.Insert(14.2);
		route.Insert(Vector(5885.9, 0, 818.2)); speeds.Insert(14.6);
		route.Insert(Vector(5891.7, 0, 812.8)); speeds.Insert(14.9);
		route.Insert(Vector(5897.6, 0, 807.4)); speeds.Insert(15.0);
		route.Insert(Vector(5903.5, 0, 802.0)); speeds.Insert(15.0);
		route.Insert(Vector(5909.5, 0, 796.7)); speeds.Insert(15.0);
		route.Insert(Vector(5915.5, 0, 791.3)); speeds.Insert(15.1);
		route.Insert(Vector(5921.4, 0, 786.0)); speeds.Insert(15.1);
		route.Insert(Vector(5927.4, 0, 780.6)); speeds.Insert(15.2);
		route.Insert(Vector(5933.3, 0, 775.3)); speeds.Insert(15.3);
		route.Insert(Vector(5939.3, 0, 770.0)); speeds.Insert(15.5);
		route.Insert(Vector(5945.3, 0, 764.7)); speeds.Insert(15.6);
		route.Insert(Vector(5951.3, 0, 759.4)); speeds.Insert(15.4);
		route.Insert(Vector(5957.3, 0, 754.1)); speeds.Insert(15.5);
		route.Insert(Vector(5963.3, 0, 748.8)); speeds.Insert(15.6);
		route.Insert(Vector(5969.3, 0, 743.5)); speeds.Insert(15.6);
		route.Insert(Vector(5975.3, 0, 738.2)); speeds.Insert(15.6);
		route.Insert(Vector(5981.2, 0, 732.9)); speeds.Insert(15.7);
		route.Insert(Vector(5987.2, 0, 727.5)); speeds.Insert(15.9);
		route.Insert(Vector(5993.2, 0, 722.2)); speeds.Insert(16.0);
		route.Insert(Vector(5999.1, 0, 716.9)); speeds.Insert(16.1);
		route.Insert(Vector(6005.2, 0, 711.6)); speeds.Insert(16.1);
		route.Insert(Vector(6011.2, 0, 706.4)); speeds.Insert(16.1);
		route.Insert(Vector(6017.2, 0, 701.1)); speeds.Insert(16.2);
		route.Insert(Vector(6023.2, 0, 695.8)); speeds.Insert(16.2);
		route.Insert(Vector(6029.2, 0, 690.5)); speeds.Insert(16.2);
		route.Insert(Vector(6035.2, 0, 685.2)); speeds.Insert(16.1);
		route.Insert(Vector(6041.2, 0, 679.9)); speeds.Insert(16.1);
		route.Insert(Vector(6047.1, 0, 674.6)); speeds.Insert(15.9);
		route.Insert(Vector(6053.0, 0, 669.2)); speeds.Insert(15.7);
		route.Insert(Vector(6058.9, 0, 663.7)); speeds.Insert(15.6);
		route.Insert(Vector(6064.7, 0, 658.2)); speeds.Insert(15.4);
		route.Insert(Vector(6070.5, 0, 652.7)); speeds.Insert(15.3);
		route.Insert(Vector(6076.2, 0, 647.1)); speeds.Insert(15.2);
		route.Insert(Vector(6081.9, 0, 641.5)); speeds.Insert(15.0);
		route.Insert(Vector(6087.5, 0, 635.8)); speeds.Insert(14.8);
		route.Insert(Vector(6092.8, 0, 629.7)); speeds.Insert(14.6);
		route.Insert(Vector(6097.8, 0, 623.5)); speeds.Insert(14.5);
		route.Insert(Vector(6102.2, 0, 616.9)); speeds.Insert(14.4);
		route.Insert(Vector(6105.8, 0, 609.7)); speeds.Insert(14.3);
		route.Insert(Vector(6108.6, 0, 602.2)); speeds.Insert(14.2);
		route.Insert(Vector(6111.1, 0, 594.6)); speeds.Insert(14.2);
		route.Insert(Vector(6112.8, 0, 586.8)); speeds.Insert(14.3);
		route.Insert(Vector(6114.1, 0, 578.9)); speeds.Insert(14.6);
		route.Insert(Vector(6115.4, 0, 571.0)); speeds.Insert(14.9);
		route.Insert(Vector(6116.9, 0, 563.2)); speeds.Insert(15.4);
		route.Insert(Vector(6118.7, 0, 555.4)); speeds.Insert(15.8);
		route.Insert(Vector(6121.1, 0, 547.7)); speeds.Insert(16.2);
		route.Insert(Vector(6123.9, 0, 540.3)); speeds.Insert(16.5);
		route.Insert(Vector(6127.4, 0, 533.1)); speeds.Insert(16.5);
		route.Insert(Vector(6131.4, 0, 526.1)); speeds.Insert(16.4);
		route.Insert(Vector(6135.6, 0, 519.4)); speeds.Insert(16.2);
		route.Insert(Vector(6140.2, 0, 512.8)); speeds.Insert(15.8);
		route.Insert(Vector(6145.6, 0, 506.9)); speeds.Insert(15.3);
		route.Insert(Vector(6151.3, 0, 501.3)); speeds.Insert(14.9);
		route.Insert(Vector(6157.2, 0, 495.9)); speeds.Insert(14.5);
		route.Insert(Vector(6163.3, 0, 490.7)); speeds.Insert(14.1);
		route.Insert(Vector(6169.7, 0, 486.0)); speeds.Insert(13.7);
		route.Insert(Vector(6176.7, 0, 482.0)); speeds.Insert(13.3);
		route.Insert(Vector(6184.0, 0, 478.8)); speeds.Insert(13.0);
		route.Insert(Vector(6191.4, 0, 475.6)); speeds.Insert(12.7);
		route.Insert(Vector(6198.8, 0, 472.6)); speeds.Insert(11.8);
		route.Insert(Vector(6206.3, 0, 469.8)); speeds.Insert(10.4);
		route.Insert(Vector(6213.9, 0, 467.5)); speeds.Insert(9.8);
		route.Insert(Vector(6221.7, 0, 465.5)); speeds.Insert(9.2);
		route.Insert(Vector(6229.4, 0, 463.6)); speeds.Insert(8.7);
		route.Insert(Vector(6237.2, 0, 461.7)); speeds.Insert(8.1);
		route.Insert(Vector(6245.0, 0, 459.9)); speeds.Insert(7.4);
		route.Insert(Vector(6252.9, 0, 458.5)); speeds.Insert(4.8);
		route.Insert(Vector(6260.8, 0, 457.3)); speeds.Insert(4.7);
		route.Insert(Vector(6268.7, 0, 456.2)); speeds.Insert(4.7);
		route.Insert(Vector(6276.7, 0, 455.8)); speeds.Insert(4.7);
		route.Insert(Vector(6284.7, 0, 455.7)); speeds.Insert(4.6);
		route.Insert(Vector(6292.6, 0, 456.3)); speeds.Insert(4.4);
		route.Insert(Vector(6300.5, 0, 457.9)); speeds.Insert(4.3);
		route.Insert(Vector(6308.2, 0, 459.9)); speeds.Insert(4.3);
		route.Insert(Vector(6316.0, 0, 462.0)); speeds.Insert(4.4);
		route.Insert(Vector(6323.7, 0, 464.1)); speeds.Insert(4.5);
		route.Insert(Vector(6331.2, 0, 466.9)); speeds.Insert(4.7);
		route.Insert(Vector(6338.5, 0, 470.2)); speeds.Insert(4.8);
		route.Insert(Vector(6345.8, 0, 473.3)); speeds.Insert(5.0);
		route.Insert(Vector(6353.1, 0, 476.5)); speeds.Insert(5.1);
		route.Insert(Vector(6360.5, 0, 479.7)); speeds.Insert(5.3);
		route.Insert(Vector(6367.8, 0, 483.0)); speeds.Insert(5.5);
		route.Insert(Vector(6375.1, 0, 486.2)); speeds.Insert(5.7);
		route.Insert(Vector(6382.4, 0, 489.4)); speeds.Insert(6.0);
		route.Insert(Vector(6389.8, 0, 492.6)); speeds.Insert(7.3);
		route.Insert(Vector(6397.0, 0, 495.9)); speeds.Insert(8.1);
		route.Insert(Vector(6404.3, 0, 499.3)); speeds.Insert(8.8);
		route.Insert(Vector(6411.5, 0, 502.8)); speeds.Insert(9.4);
		route.Insert(Vector(6418.7, 0, 506.2)); speeds.Insert(10.0);
		route.Insert(Vector(6425.9, 0, 509.7)); speeds.Insert(10.5);
		route.Insert(Vector(6433.1, 0, 513.1)); speeds.Insert(10.9);
		route.Insert(Vector(6440.4, 0, 516.6)); speeds.Insert(12.0);
		route.Insert(Vector(6447.6, 0, 520.0)); speeds.Insert(13.1);
		route.Insert(Vector(6454.9, 0, 523.4)); speeds.Insert(13.8);
		route.Insert(Vector(6462.1, 0, 526.7)); speeds.Insert(14.4);
		route.Insert(Vector(6469.4, 0, 530.1)); speeds.Insert(14.9);
		route.Insert(Vector(6476.6, 0, 533.5)); speeds.Insert(15.5);
		route.Insert(Vector(6483.8, 0, 537.0)); speeds.Insert(15.5);
		route.Insert(Vector(6491.0, 0, 540.4)); speeds.Insert(15.6);
		route.Insert(Vector(6498.2, 0, 543.9)); speeds.Insert(16.0);
		route.Insert(Vector(6505.4, 0, 547.6)); speeds.Insert(15.7);
		route.Insert(Vector(6512.4, 0, 551.4)); speeds.Insert(15.8);
		route.Insert(Vector(6519.3, 0, 555.4)); speeds.Insert(15.5);
		route.Insert(Vector(6526.0, 0, 559.8)); speeds.Insert(15.0);
		route.Insert(Vector(6532.5, 0, 564.5)); speeds.Insert(15.0);
		route.Insert(Vector(6538.7, 0, 569.6)); speeds.Insert(15.0);
		route.Insert(Vector(6544.6, 0, 574.9)); speeds.Insert(15.0);
		route.Insert(Vector(6550.3, 0, 580.5)); speeds.Insert(14.9);
		route.Insert(Vector(6556.0, 0, 586.1)); speeds.Insert(14.9);
		route.Insert(Vector(6561.3, 0, 592.1)); speeds.Insert(15.0);
		route.Insert(Vector(6566.2, 0, 598.4)); speeds.Insert(15.0);
		route.Insert(Vector(6570.6, 0, 605.1)); speeds.Insert(15.5);
		route.Insert(Vector(6574.1, 0, 612.3)); speeds.Insert(16.1);
		route.Insert(Vector(6577.6, 0, 619.5)); speeds.Insert(16.7);
		route.Insert(Vector(6581.2, 0, 626.6)); speeds.Insert(17.3);
		route.Insert(Vector(6584.7, 0, 633.8)); speeds.Insert(17.8);
		route.Insert(Vector(6588.0, 0, 641.1)); speeds.Insert(17.5);
		route.Insert(Vector(6591.3, 0, 648.4)); speeds.Insert(17.1);
		route.Insert(Vector(6594.6, 0, 655.7)); speeds.Insert(17.3);
		route.Insert(Vector(6597.9, 0, 662.9)); speeds.Insert(15.8);
		route.Insert(Vector(6601.3, 0, 670.2)); speeds.Insert(15.7);
		route.Insert(Vector(6604.8, 0, 677.4)); speeds.Insert(15.7);
		route.Insert(Vector(6608.5, 0, 684.5)); speeds.Insert(15.6);
		route.Insert(Vector(6612.2, 0, 691.6)); speeds.Insert(15.5);
		route.Insert(Vector(6615.6, 0, 698.8)); speeds.Insert(15.4);
		route.Insert(Vector(6619.0, 0, 706.1)); speeds.Insert(15.3);
		route.Insert(Vector(6622.3, 0, 713.4)); speeds.Insert(15.3);
		route.Insert(Vector(6625.4, 0, 720.7)); speeds.Insert(15.4);
		route.Insert(Vector(6628.2, 0, 728.2)); speeds.Insert(15.4);
		route.Insert(Vector(6630.7, 0, 735.8)); speeds.Insert(15.4);
		route.Insert(Vector(6633.1, 0, 743.4)); speeds.Insert(15.4);
		route.Insert(Vector(6635.7, 0, 751.0)); speeds.Insert(15.4);
		route.Insert(Vector(6638.8, 0, 758.4)); speeds.Insert(15.5);
		route.Insert(Vector(6642.7, 0, 765.3)); speeds.Insert(15.5);
		route.Insert(Vector(6647.3, 0, 771.9)); speeds.Insert(15.6);
		route.Insert(Vector(6652.1, 0, 778.3)); speeds.Insert(15.8);
		route.Insert(Vector(6657.1, 0, 784.5)); speeds.Insert(15.9);
		route.Insert(Vector(6662.7, 0, 790.3)); speeds.Insert(16.0);
		route.Insert(Vector(6668.9, 0, 795.3)); speeds.Insert(16.1);
		route.Insert(Vector(6675.1, 0, 800.3)); speeds.Insert(16.2);
		route.Insert(Vector(6681.3, 0, 805.5)); speeds.Insert(16.2);
		route.Insert(Vector(6687.6, 0, 810.3)); speeds.Insert(16.0);
		route.Insert(Vector(6694.6, 0, 814.3)); speeds.Insert(15.7);
		route.Insert(Vector(6701.7, 0, 818.0)); speeds.Insert(15.8);
		route.Insert(Vector(6708.9, 0, 821.4)); speeds.Insert(15.8);
		route.Insert(Vector(6716.3, 0, 824.4)); speeds.Insert(16.0);
		route.Insert(Vector(6723.7, 0, 827.3)); speeds.Insert(16.1);
		route.Insert(Vector(6731.2, 0, 830.3)); speeds.Insert(16.0);
		route.Insert(Vector(6738.6, 0, 833.4)); speeds.Insert(15.9);
		route.Insert(Vector(6745.7, 0, 836.9)); speeds.Insert(16.2);
		route.Insert(Vector(6752.4, 0, 841.3)); speeds.Insert(15.7);
		route.Insert(Vector(6759.0, 0, 845.9)); speeds.Insert(15.5);
		route.Insert(Vector(6765.4, 0, 850.6)); speeds.Insert(15.7);
		route.Insert(Vector(6771.9, 0, 855.3)); speeds.Insert(15.9);
		route.Insert(Vector(6778.4, 0, 859.9)); speeds.Insert(16.3);
		route.Insert(Vector(6785.1, 0, 864.3)); speeds.Insert(15.2);
		route.Insert(Vector(6792.1, 0, 868.2)); speeds.Insert(15.7);
		route.Insert(Vector(6799.2, 0, 871.8)); speeds.Insert(16.1);
		route.Insert(Vector(6806.4, 0, 875.3)); speeds.Insert(16.0);
		route.Insert(Vector(6813.7, 0, 878.7)); speeds.Insert(16.1);
		route.Insert(Vector(6821.0, 0, 882.0)); speeds.Insert(16.7);
		route.Insert(Vector(6828.2, 0, 885.4)); speeds.Insert(16.9);
		route.Insert(Vector(6835.5, 0, 888.8)); speeds.Insert(16.8);
		route.Insert(Vector(6842.8, 0, 892.1)); speeds.Insert(16.8);
		route.Insert(Vector(6850.1, 0, 895.2)); speeds.Insert(17.1);
		route.Insert(Vector(6857.6, 0, 898.0)); speeds.Insert(17.4);
		route.Insert(Vector(6865.1, 0, 900.8)); speeds.Insert(17.6);
		route.Insert(Vector(6872.7, 0, 903.4)); speeds.Insert(17.9);
		route.Insert(Vector(6880.3, 0, 905.7)); speeds.Insert(18.1);
		route.Insert(Vector(6888.0, 0, 908.1)); speeds.Insert(18.1);
		route.Insert(Vector(6895.7, 0, 910.3)); speeds.Insert(18.0);
		route.Insert(Vector(6903.5, 0, 912.1)); speeds.Insert(17.7);
		route.Insert(Vector(6911.3, 0, 913.9)); speeds.Insert(17.4);
		route.Insert(Vector(6919.0, 0, 915.7)); speeds.Insert(17.1);
		route.Insert(Vector(6926.8, 0, 917.6)); speeds.Insert(16.8);
		route.Insert(Vector(6934.6, 0, 919.5)); speeds.Insert(16.4);
		route.Insert(Vector(6942.4, 0, 921.2)); speeds.Insert(16.2);
		route.Insert(Vector(6950.2, 0, 922.8)); speeds.Insert(15.9);
		route.Insert(Vector(6958.1, 0, 924.3)); speeds.Insert(15.7);
		route.Insert(Vector(6966.0, 0, 925.5)); speeds.Insert(15.6);
		route.Insert(Vector(6973.9, 0, 926.6)); speeds.Insert(15.3);
		route.Insert(Vector(6981.9, 0, 927.7)); speeds.Insert(14.9);
		route.Insert(Vector(6989.8, 0, 928.6)); speeds.Insert(14.8);
		route.Insert(Vector(6997.7, 0, 929.6)); speeds.Insert(14.8);
		route.Insert(Vector(7005.7, 0, 930.5)); speeds.Insert(14.9);
		route.Insert(Vector(7013.6, 0, 931.4)); speeds.Insert(15.2);
		route.Insert(Vector(7021.6, 0, 932.3)); speeds.Insert(15.5);
		route.Insert(Vector(7029.5, 0, 933.1)); speeds.Insert(15.8);
		route.Insert(Vector(7037.5, 0, 934.0)); speeds.Insert(16.0);
		route.Insert(Vector(7045.5, 0, 934.8)); speeds.Insert(16.2);
		route.Insert(Vector(7053.4, 0, 935.5)); speeds.Insert(16.7);
		route.Insert(Vector(7061.4, 0, 936.1)); speeds.Insert(17.2);
		route.Insert(Vector(7069.4, 0, 936.7)); speeds.Insert(17.6);
		route.Insert(Vector(7077.4, 0, 937.2)); speeds.Insert(17.9);
		route.Insert(Vector(7085.4, 0, 937.3)); speeds.Insert(18.2);
		route.Insert(Vector(7093.4, 0, 937.3)); speeds.Insert(18.5);
		route.Insert(Vector(7101.4, 0, 937.2)); speeds.Insert(17.2);
		route.Insert(Vector(7109.4, 0, 937.2)); speeds.Insert(17.1);
		route.Insert(Vector(7117.4, 0, 937.1)); speeds.Insert(17.0);
		route.Insert(Vector(7125.4, 0, 937.1)); speeds.Insert(16.2);
		route.Insert(Vector(7133.4, 0, 937.2)); speeds.Insert(16.0);
		route.Insert(Vector(7141.4, 0, 937.5)); speeds.Insert(15.9);
		route.Insert(Vector(7149.3, 0, 938.3)); speeds.Insert(15.8);
		route.Insert(Vector(7157.2, 0, 939.5)); speeds.Insert(15.8);
		route.Insert(Vector(7165.0, 0, 941.3)); speeds.Insert(16.0);
		route.Insert(Vector(7172.7, 0, 943.5)); speeds.Insert(16.1);
		route.Insert(Vector(7180.3, 0, 946.0)); speeds.Insert(16.2);
		route.Insert(Vector(7187.7, 0, 949.1)); speeds.Insert(16.4);
		route.Insert(Vector(7195.0, 0, 952.3)); speeds.Insert(16.7);
		route.Insert(Vector(7202.3, 0, 955.6)); speeds.Insert(16.9);
		route.Insert(Vector(7209.5, 0, 959.0)); speeds.Insert(17.0);
		route.Insert(Vector(7216.8, 0, 962.4)); speeds.Insert(17.3);
		route.Insert(Vector(7224.0, 0, 965.8)); speeds.Insert(17.7);
		route.Insert(Vector(7231.3, 0, 969.2)); speeds.Insert(18.0);
		route.Insert(Vector(7238.5, 0, 972.6)); speeds.Insert(18.2);
		route.Insert(Vector(7245.7, 0, 976.0)); speeds.Insert(18.5);
		route.Insert(Vector(7252.8, 0, 979.7)); speeds.Insert(18.7);
		route.Insert(Vector(7259.9, 0, 983.4)); speeds.Insert(18.9);
		route.Insert(Vector(7267.1, 0, 987.0)); speeds.Insert(19.0);
		route.Insert(Vector(7274.2, 0, 990.7)); speeds.Insert(19.0);
		route.Insert(Vector(7281.2, 0, 994.6)); speeds.Insert(18.8);
		route.Insert(Vector(7288.2, 0, 998.3)); speeds.Insert(18.6);
		route.Insert(Vector(7295.3, 0, 1002.1)); speeds.Insert(18.3);
		route.Insert(Vector(7302.2, 0, 1006.1)); speeds.Insert(17.7);
		route.Insert(Vector(7309.2, 0, 1010.0)); speeds.Insert(16.5);
		route.Insert(Vector(7316.1, 0, 1014.0)); speeds.Insert(16.1);
		route.Insert(Vector(7323.1, 0, 1018.0)); speeds.Insert(15.6);
		route.Insert(Vector(7330.0, 0, 1021.9)); speeds.Insert(15.1);
		route.Insert(Vector(7337.0, 0, 1025.9)); speeds.Insert(14.7);
		route.Insert(Vector(7343.8, 0, 1030.0)); speeds.Insert(14.5);
		route.Insert(Vector(7350.6, 0, 1034.2)); speeds.Insert(14.4);
		route.Insert(Vector(7357.4, 0, 1038.5)); speeds.Insert(14.3);
		route.Insert(Vector(7364.0, 0, 1043.0)); speeds.Insert(14.0);
		route.Insert(Vector(7370.4, 0, 1047.9)); speeds.Insert(13.1);
		route.Insert(Vector(7376.6, 0, 1052.8)); speeds.Insert(11.8);
		route.Insert(Vector(7382.7, 0, 1058.0)); speeds.Insert(11.7);
		route.Insert(Vector(7388.5, 0, 1063.5)); speeds.Insert(11.9);
		route.Insert(Vector(7393.0, 0, 1070.1)); speeds.Insert(11.6);
		route.Insert(Vector(7395.0, 0, 1077.8)); speeds.Insert(10.8);
		route.Insert(Vector(7394.5, 0, 1085.8)); speeds.Insert(10.5);
		route.Insert(Vector(7394.1, 0, 1093.8)); speeds.Insert(10.7);
		route.Insert(Vector(7394.0, 0, 1101.8)); speeds.Insert(11.0);
		route.Insert(Vector(7394.6, 0, 1109.7)); speeds.Insert(11.5);
		route.Insert(Vector(7396.0, 0, 1117.6)); speeds.Insert(12.1);
		route.Insert(Vector(7398.7, 0, 1125.1)); speeds.Insert(12.4);
		route.Insert(Vector(7402.1, 0, 1132.4)); speeds.Insert(13.2);
		route.Insert(Vector(7405.9, 0, 1139.4)); speeds.Insert(13.9);
		route.Insert(Vector(7410.5, 0, 1145.9)); speeds.Insert(14.6);
		route.Insert(Vector(7415.3, 0, 1152.4)); speeds.Insert(14.8);
		route.Insert(Vector(7420.4, 0, 1158.5)); speeds.Insert(14.7);
		route.Insert(Vector(7426.2, 0, 1164.0)); speeds.Insert(14.6);
		route.Insert(Vector(7432.6, 0, 1168.8)); speeds.Insert(14.5);
		route.Insert(Vector(7439.0, 0, 1173.6)); speeds.Insert(14.7);
		route.Insert(Vector(7445.5, 0, 1178.3)); speeds.Insert(13.9);
		route.Insert(Vector(7452.1, 0, 1182.9)); speeds.Insert(14.4);
		route.Insert(Vector(7458.7, 0, 1187.3)); speeds.Insert(15.1);
		route.Insert(Vector(7465.7, 0, 1191.2)); speeds.Insert(16.0);
		route.Insert(Vector(7473.0, 0, 1194.6)); speeds.Insert(17.2);
		route.Insert(Vector(7480.5, 0, 1197.3)); speeds.Insert(16.4);
		route.Insert(Vector(7488.1, 0, 1199.8)); speeds.Insert(17.4);
		route.Insert(Vector(7495.7, 0, 1202.1)); speeds.Insert(17.6);
		route.Insert(Vector(7503.4, 0, 1204.3)); speeds.Insert(16.1);
		route.Insert(Vector(7511.1, 0, 1206.6)); speeds.Insert(16.4);
		route.Insert(Vector(7518.8, 0, 1208.8)); speeds.Insert(16.0);
		route.Insert(Vector(7526.4, 0, 1211.2)); speeds.Insert(16.1);
		route.Insert(Vector(7534.1, 0, 1213.5)); speeds.Insert(16.1);
		route.Insert(Vector(7541.7, 0, 1215.9)); speeds.Insert(15.9);
		route.Insert(Vector(7549.3, 0, 1218.2)); speeds.Insert(15.8);
		route.Insert(Vector(7557.0, 0, 1220.6)); speeds.Insert(15.5);
		route.Insert(Vector(7564.6, 0, 1223.1)); speeds.Insert(15.3);
		route.Insert(Vector(7572.2, 0, 1225.6)); speeds.Insert(15.0);
		route.Insert(Vector(7579.7, 0, 1228.3)); speeds.Insert(14.7);
		route.Insert(Vector(7587.3, 0, 1230.9)); speeds.Insert(14.7);
		route.Insert(Vector(7594.8, 0, 1233.6)); speeds.Insert(14.7);
		route.Insert(Vector(7602.4, 0, 1236.2)); speeds.Insert(14.5);
		route.Insert(Vector(7610.0, 0, 1238.6)); speeds.Insert(14.4);
		route.Insert(Vector(7617.6, 0, 1241.0)); speeds.Insert(14.3);
		route.Insert(Vector(7625.3, 0, 1243.2)); speeds.Insert(14.3);
		route.Insert(Vector(7633.2, 0, 1244.7)); speeds.Insert(14.5);
		route.Insert(Vector(7641.1, 0, 1245.9)); speeds.Insert(14.9);
		route.Insert(Vector(7649.1, 0, 1246.3)); speeds.Insert(15.3);
		route.Insert(Vector(7657.1, 0, 1246.0)); speeds.Insert(16.0);
		route.Insert(Vector(7665.1, 0, 1245.4)); speeds.Insert(16.3);
		route.Insert(Vector(7673.0, 0, 1244.5)); speeds.Insert(16.3);
		route.Insert(Vector(7680.9, 0, 1243.3)); speeds.Insert(17.1);
		route.Insert(Vector(7688.8, 0, 1241.8)); speeds.Insert(17.8);
		route.Insert(Vector(7696.6, 0, 1240.0)); speeds.Insert(16.8);
		route.Insert(Vector(7704.3, 0, 1238.0)); speeds.Insert(17.0);
		route.Insert(Vector(7711.9, 0, 1235.4)); speeds.Insert(17.3);
		route.Insert(Vector(7719.4, 0, 1232.6)); speeds.Insert(16.2);
		route.Insert(Vector(7726.8, 0, 1229.6)); speeds.Insert(16.6);
		route.Insert(Vector(7734.0, 0, 1226.1)); speeds.Insert(16.8);
		route.Insert(Vector(7741.2, 0, 1222.7)); speeds.Insert(15.9);
		route.Insert(Vector(7748.2, 0, 1218.8)); speeds.Insert(16.1);
		route.Insert(Vector(7755.2, 0, 1214.9)); speeds.Insert(15.5);
		route.Insert(Vector(7762.1, 0, 1210.9)); speeds.Insert(13.4);
		route.Insert(Vector(7768.8, 0, 1206.4)); speeds.Insert(12.6);
		route.Insert(Vector(7774.9, 0, 1201.3)); speeds.Insert(10.8);
		route.Insert(Vector(7779.9, 0, 1195.1)); speeds.Insert(9.0);
		route.Insert(Vector(7784.2, 0, 1188.4)); speeds.Insert(9.2);
		route.Insert(Vector(7788.1, 0, 1181.4)); speeds.Insert(8.3);
		route.Insert(Vector(7791.4, 0, 1174.1)); speeds.Insert(6.5);
		route.Insert(Vector(7794.7, 0, 1166.8)); speeds.Insert(5.9);
		route.Insert(Vector(7801.0, 0, 1162.5)); speeds.Insert(3.9);
		route.Insert(Vector(7808.7, 0, 1164.4)); speeds.Insert(2.7);
		route.Insert(Vector(7816.4, 0, 1166.4)); speeds.Insert(4.3);
		route.Insert(Vector(7824.2, 0, 1168.3)); speeds.Insert(5.2);
		route.Insert(Vector(7832.0, 0, 1170.1)); speeds.Insert(5.5);
		route.Insert(Vector(7839.8, 0, 1171.9)); speeds.Insert(6.6);
		route.Insert(Vector(7847.6, 0, 1173.7)); speeds.Insert(7.1);
		route.Insert(Vector(7855.2, 0, 1176.2)); speeds.Insert(7.5);
		route.Insert(Vector(7862.5, 0, 1179.5)); speeds.Insert(8.0);
		route.Insert(Vector(7869.4, 0, 1183.5)); speeds.Insert(8.4);
		route.Insert(Vector(7875.9, 0, 1188.2)); speeds.Insert(8.9);
		route.Insert(Vector(7882.2, 0, 1193.0)); speeds.Insert(9.4);
		route.Insert(Vector(7888.4, 0, 1198.1)); speeds.Insert(10.0);
		route.Insert(Vector(7894.6, 0, 1203.2)); speeds.Insert(11.1);
		route.Insert(Vector(7900.8, 0, 1208.3)); speeds.Insert(11.8);
		route.Insert(Vector(7906.9, 0, 1213.4)); speeds.Insert(12.7);
		route.Insert(Vector(7913.1, 0, 1218.4)); speeds.Insert(12.2);
		route.Insert(Vector(7919.3, 0, 1223.5)); speeds.Insert(11.6);
		route.Insert(Vector(7925.5, 0, 1228.6)); speeds.Insert(11.7);
		route.Insert(Vector(7931.7, 0, 1233.7)); speeds.Insert(11.2);
		route.Insert(Vector(7937.8, 0, 1238.7)); speeds.Insert(12.2);
		route.Insert(Vector(7944.1, 0, 1243.7)); speeds.Insert(11.7);
		route.Insert(Vector(7950.4, 0, 1248.6)); speeds.Insert(11.5);
		route.Insert(Vector(7956.7, 0, 1253.6)); speeds.Insert(12.6);
		route.Insert(Vector(7963.0, 0, 1258.5)); speeds.Insert(12.3);
		route.Insert(Vector(7969.4, 0, 1263.4)); speeds.Insert(13.2);
		route.Insert(Vector(7975.8, 0, 1268.2)); speeds.Insert(12.4);
		route.Insert(Vector(7982.3, 0, 1272.8)); speeds.Insert(13.4);
		route.Insert(Vector(7988.8, 0, 1277.4)); speeds.Insert(12.5);
		route.Insert(Vector(7995.5, 0, 1281.7)); speeds.Insert(12.3);
		route.Insert(Vector(8002.6, 0, 1285.6)); speeds.Insert(12.4);
		route.Insert(Vector(8009.8, 0, 1289.1)); speeds.Insert(13.3);
		route.Insert(Vector(8017.2, 0, 1292.1)); speeds.Insert(13.1);
		route.Insert(Vector(8024.7, 0, 1294.8)); speeds.Insert(13.5);
		route.Insert(Vector(8032.4, 0, 1296.9)); speeds.Insert(13.5);
		route.Insert(Vector(8040.2, 0, 1298.9)); speeds.Insert(14.1);
		route.Insert(Vector(8047.9, 0, 1300.9)); speeds.Insert(14.8);
		route.Insert(Vector(8055.7, 0, 1302.9)); speeds.Insert(14.0);
		route.Insert(Vector(8063.4, 0, 1304.9)); speeds.Insert(14.3);
		route.Insert(Vector(8071.2, 0, 1306.8)); speeds.Insert(15.1);
		route.Insert(Vector(8078.9, 0, 1308.9)); speeds.Insert(15.7);
		route.Insert(Vector(8086.7, 0, 1310.8)); speeds.Insert(15.9);
		route.Insert(Vector(8094.4, 0, 1312.8)); speeds.Insert(13.6);
		route.Insert(Vector(8102.2, 0, 1314.8)); speeds.Insert(13.4);
		route.Insert(Vector(8109.8, 0, 1317.1)); speeds.Insert(12.6);
		route.Insert(Vector(8117.2, 0, 1320.1)); speeds.Insert(13.1);
		route.Insert(Vector(8124.2, 0, 1324.0)); speeds.Insert(13.3);
		route.Insert(Vector(8130.8, 0, 1328.6)); speeds.Insert(12.7);
		route.Insert(Vector(8136.8, 0, 1333.8)); speeds.Insert(12.6);
		route.Insert(Vector(8142.1, 0, 1339.8)); speeds.Insert(12.8);
		route.Insert(Vector(8146.0, 0, 1346.8)); speeds.Insert(13.7);
		route.Insert(Vector(8148.7, 0, 1354.3)); speeds.Insert(14.4);
		route.Insert(Vector(8151.0, 0, 1362.0)); speeds.Insert(15.2);
		route.Insert(Vector(8152.8, 0, 1369.7)); speeds.Insert(14.9);
		route.Insert(Vector(8153.5, 0, 1377.7)); speeds.Insert(15.4);
		route.Insert(Vector(8153.0, 0, 1385.7)); speeds.Insert(15.7);
		route.Insert(Vector(8152.3, 0, 1393.7)); speeds.Insert(15.9);
		route.Insert(Vector(8151.5, 0, 1401.6)); speeds.Insert(15.9);
		route.Insert(Vector(8150.5, 0, 1409.5)); speeds.Insert(15.8);
		route.Insert(Vector(8149.3, 0, 1417.5)); speeds.Insert(15.5);
		route.Insert(Vector(8148.0, 0, 1425.4)); speeds.Insert(14.8);
		route.Insert(Vector(8146.3, 0, 1433.2)); speeds.Insert(14.1);
		route.Insert(Vector(8144.4, 0, 1440.9)); speeds.Insert(13.8);
		route.Insert(Vector(8142.6, 0, 1448.7)); speeds.Insert(13.6);
		route.Insert(Vector(8140.7, 0, 1456.5)); speeds.Insert(13.6);
		route.Insert(Vector(8138.9, 0, 1464.3)); speeds.Insert(13.8);
		route.Insert(Vector(8137.2, 0, 1472.1)); speeds.Insert(14.1);
		route.Insert(Vector(8135.7, 0, 1480.0)); speeds.Insert(14.5);
		route.Insert(Vector(8134.5, 0, 1487.9)); speeds.Insert(15.1);
		route.Insert(Vector(8133.6, 0, 1495.8)); speeds.Insert(15.8);
		route.Insert(Vector(8132.6, 0, 1503.8)); speeds.Insert(16.3);
		route.Insert(Vector(8131.7, 0, 1511.7)); speeds.Insert(16.9);
		route.Insert(Vector(8131.0, 0, 1519.7)); speeds.Insert(17.5);
		route.Insert(Vector(8130.4, 0, 1527.7)); speeds.Insert(18.0);
		route.Insert(Vector(8130.0, 0, 1535.6)); speeds.Insert(17.1);
		route.Insert(Vector(8129.9, 0, 1543.6)); speeds.Insert(16.6);
		route.Insert(Vector(8129.9, 0, 1551.6)); speeds.Insert(16.9);
		route.Insert(Vector(8129.8, 0, 1559.6)); speeds.Insert(17.2);
		route.Insert(Vector(8129.6, 0, 1567.6)); speeds.Insert(17.3);
		route.Insert(Vector(8129.0, 0, 1575.6)); speeds.Insert(17.2);
		route.Insert(Vector(8128.2, 0, 1583.6)); speeds.Insert(16.9);
		route.Insert(Vector(8127.4, 0, 1591.5)); speeds.Insert(16.7);
		route.Insert(Vector(8126.5, 0, 1599.5)); speeds.Insert(16.6);
		route.Insert(Vector(8125.6, 0, 1607.4)); speeds.Insert(16.4);
		route.Insert(Vector(8124.8, 0, 1615.4)); speeds.Insert(16.3);
		route.Insert(Vector(8124.0, 0, 1623.3)); speeds.Insert(16.1);
		route.Insert(Vector(8122.9, 0, 1631.3)); speeds.Insert(16.0);
		route.Insert(Vector(8121.8, 0, 1639.2)); speeds.Insert(15.8);
		route.Insert(Vector(8120.7, 0, 1647.1)); speeds.Insert(15.7);
		route.Insert(Vector(8119.6, 0, 1655.0)); speeds.Insert(15.5);
		route.Insert(Vector(8118.3, 0, 1662.9)); speeds.Insert(15.4);
		route.Insert(Vector(8117.0, 0, 1670.8)); speeds.Insert(15.3);
		route.Insert(Vector(8115.6, 0, 1678.7)); speeds.Insert(15.3);
		route.Insert(Vector(8114.4, 0, 1686.6)); speeds.Insert(15.2);
		route.Insert(Vector(8113.2, 0, 1694.5)); speeds.Insert(15.2);
		route.Insert(Vector(8112.0, 0, 1702.4)); speeds.Insert(15.2);
		route.Insert(Vector(8110.9, 0, 1710.4)); speeds.Insert(15.1);
		route.Insert(Vector(8109.6, 0, 1718.3)); speeds.Insert(14.5);
		route.Insert(Vector(8109.2, 0, 1720.7)); speeds.Insert(13.7);
	}

	// Badlands: the recorded route, a point every 8 m and the speed driven there (m/s). A function of its own per map: one
	// function with every map's points is over the script compiler's instruction limit
	static void DL_RouteBadlands(array<vector> route, array<float> speeds)
	{
		route.Insert(Vector(13737.3, 0, 1173.2)); speeds.Insert(2.4);
		route.Insert(Vector(13732.5, 0, 1179.6)); speeds.Insert(4.4);
		route.Insert(Vector(13727.8, 0, 1186.1)); speeds.Insert(5.8);
		route.Insert(Vector(13723.1, 0, 1192.6)); speeds.Insert(6.5);
		route.Insert(Vector(13718.4, 0, 1199.0)); speeds.Insert(8.0);
		route.Insert(Vector(13713.7, 0, 1205.5)); speeds.Insert(8.7);
		route.Insert(Vector(13709.0, 0, 1212.0)); speeds.Insert(9.3);
		route.Insert(Vector(13704.4, 0, 1218.5)); speeds.Insert(9.7);
		route.Insert(Vector(13699.7, 0, 1225.0)); speeds.Insert(9.9);
		route.Insert(Vector(13695.0, 0, 1231.4)); speeds.Insert(9.9);
		route.Insert(Vector(13690.3, 0, 1237.9)); speeds.Insert(9.3);
		route.Insert(Vector(13685.6, 0, 1244.4)); speeds.Insert(9.1);
		route.Insert(Vector(13681.0, 0, 1251.0)); speeds.Insert(9.2);
		route.Insert(Vector(13676.3, 0, 1257.4)); speeds.Insert(9.3);
		route.Insert(Vector(13671.7, 0, 1263.9)); speeds.Insert(9.8);
		route.Insert(Vector(13667.0, 0, 1270.5)); speeds.Insert(10.3);
		route.Insert(Vector(13662.3, 0, 1276.9)); speeds.Insert(11.2);
		route.Insert(Vector(13657.7, 0, 1283.5)); speeds.Insert(11.8);
		route.Insert(Vector(13653.0, 0, 1290.0)); speeds.Insert(12.2);
		route.Insert(Vector(13648.3, 0, 1296.4)); speeds.Insert(12.5);
		route.Insert(Vector(13643.5, 0, 1302.8)); speeds.Insert(12.8);
		route.Insert(Vector(13638.8, 0, 1309.3)); speeds.Insert(13.1);
		route.Insert(Vector(13634.1, 0, 1315.7)); speeds.Insert(13.4);
		route.Insert(Vector(13629.3, 0, 1322.2)); speeds.Insert(13.7);
		route.Insert(Vector(13624.6, 0, 1328.6)); speeds.Insert(13.8);
		route.Insert(Vector(13619.8, 0, 1335.0)); speeds.Insert(14.0);
		route.Insert(Vector(13615.0, 0, 1341.4)); speeds.Insert(14.1);
		route.Insert(Vector(13610.2, 0, 1347.9)); speeds.Insert(14.1);
		route.Insert(Vector(13605.4, 0, 1354.3)); speeds.Insert(13.4);
		route.Insert(Vector(13600.7, 0, 1360.7)); speeds.Insert(11.8);
		route.Insert(Vector(13596.0, 0, 1367.2)); speeds.Insert(11.4);
		route.Insert(Vector(13591.1, 0, 1373.5)); speeds.Insert(11.6);
		route.Insert(Vector(13586.1, 0, 1379.7)); speeds.Insert(11.4);
		route.Insert(Vector(13581.0, 0, 1385.9)); speeds.Insert(11.0);
		route.Insert(Vector(13575.8, 0, 1392.0)); speeds.Insert(10.8);
		route.Insert(Vector(13570.7, 0, 1398.1)); speeds.Insert(11.0);
		route.Insert(Vector(13565.4, 0, 1404.2)); speeds.Insert(11.8);
		route.Insert(Vector(13560.3, 0, 1410.3)); speeds.Insert(12.2);
		route.Insert(Vector(13555.0, 0, 1416.3)); speeds.Insert(11.8);
		route.Insert(Vector(13549.7, 0, 1422.3)); speeds.Insert(11.1);
		route.Insert(Vector(13544.5, 0, 1428.3)); speeds.Insert(11.2);
		route.Insert(Vector(13539.1, 0, 1434.3)); speeds.Insert(11.3);
		route.Insert(Vector(13533.6, 0, 1440.1)); speeds.Insert(11.7);
		route.Insert(Vector(13528.2, 0, 1445.9)); speeds.Insert(11.6);
		route.Insert(Vector(13522.7, 0, 1451.8)); speeds.Insert(12.0);
		route.Insert(Vector(13517.2, 0, 1457.6)); speeds.Insert(12.3);
		route.Insert(Vector(13511.6, 0, 1463.3)); speeds.Insert(12.4);
		route.Insert(Vector(13506.1, 0, 1469.1)); speeds.Insert(12.0);
		route.Insert(Vector(13500.4, 0, 1474.7)); speeds.Insert(11.5);
		route.Insert(Vector(13494.6, 0, 1480.2)); speeds.Insert(11.5);
		route.Insert(Vector(13488.7, 0, 1485.7)); speeds.Insert(12.1);
		route.Insert(Vector(13482.9, 0, 1491.1)); speeds.Insert(12.5);
		route.Insert(Vector(13477.0, 0, 1496.6)); speeds.Insert(12.8);
		route.Insert(Vector(13471.1, 0, 1501.9)); speeds.Insert(13.1);
		route.Insert(Vector(13465.1, 0, 1507.3)); speeds.Insert(13.4);
		route.Insert(Vector(13459.2, 0, 1512.7)); speeds.Insert(13.3);
		route.Insert(Vector(13453.3, 0, 1518.1)); speeds.Insert(12.8);
		route.Insert(Vector(13447.4, 0, 1523.4)); speeds.Insert(11.5);
		route.Insert(Vector(13441.5, 0, 1528.9)); speeds.Insert(11.3);
		route.Insert(Vector(13435.3, 0, 1533.9)); speeds.Insert(11.2);
		route.Insert(Vector(13429.3, 0, 1539.2)); speeds.Insert(11.4);
		route.Insert(Vector(13423.0, 0, 1544.1)); speeds.Insert(11.7);
		route.Insert(Vector(13416.5, 0, 1548.8)); speeds.Insert(12.1);
		route.Insert(Vector(13410.1, 0, 1553.6)); speeds.Insert(12.5);
		route.Insert(Vector(13403.7, 0, 1558.4)); speeds.Insert(12.8);
		route.Insert(Vector(13397.2, 0, 1563.1)); speeds.Insert(13.1);
		route.Insert(Vector(13390.8, 0, 1567.8)); speeds.Insert(13.4);
		route.Insert(Vector(13384.3, 0, 1572.5)); speeds.Insert(13.7);
		route.Insert(Vector(13377.8, 0, 1577.2)); speeds.Insert(13.9);
		route.Insert(Vector(13371.2, 0, 1581.7)); speeds.Insert(14.2);
		route.Insert(Vector(13364.6, 0, 1586.3)); speeds.Insert(14.4);
		route.Insert(Vector(13358.0, 0, 1590.7)); speeds.Insert(14.6);
		route.Insert(Vector(13351.3, 0, 1595.1)); speeds.Insert(14.8);
		route.Insert(Vector(13344.6, 0, 1599.5)); speeds.Insert(15.0);
		route.Insert(Vector(13337.9, 0, 1603.9)); speeds.Insert(15.1);
		route.Insert(Vector(13331.2, 0, 1608.2)); speeds.Insert(15.2);
		route.Insert(Vector(13324.4, 0, 1612.4)); speeds.Insert(15.3);
		route.Insert(Vector(13317.6, 0, 1616.7)); speeds.Insert(15.5);
		route.Insert(Vector(13310.8, 0, 1620.9)); speeds.Insert(15.7);
		route.Insert(Vector(13303.9, 0, 1625.0)); speeds.Insert(15.8);
		route.Insert(Vector(13297.1, 0, 1629.1)); speeds.Insert(15.5);
		route.Insert(Vector(13290.2, 0, 1633.2)); speeds.Insert(14.2);
		route.Insert(Vector(13283.4, 0, 1637.5)); speeds.Insert(13.3);
		route.Insert(Vector(13276.5, 0, 1641.4)); speeds.Insert(12.4);
		route.Insert(Vector(13269.4, 0, 1645.2)); speeds.Insert(12.1);
		route.Insert(Vector(13262.3, 0, 1648.9)); speeds.Insert(12.2);
		route.Insert(Vector(13255.3, 0, 1652.7)); speeds.Insert(12.4);
		route.Insert(Vector(13248.2, 0, 1656.4)); speeds.Insert(12.5);
		route.Insert(Vector(13241.1, 0, 1660.1)); speeds.Insert(12.8);
		route.Insert(Vector(13234.1, 0, 1663.9)); speeds.Insert(13.1);
		route.Insert(Vector(13227.0, 0, 1667.7)); speeds.Insert(13.3);
		route.Insert(Vector(13220.0, 0, 1671.6)); speeds.Insert(13.6);
		route.Insert(Vector(13213.1, 0, 1675.5)); speeds.Insert(13.9);
		route.Insert(Vector(13206.1, 0, 1679.5)); speeds.Insert(14.1);
		route.Insert(Vector(13199.3, 0, 1683.6)); speeds.Insert(14.3);
		route.Insert(Vector(13192.4, 0, 1687.7)); speeds.Insert(14.6);
		route.Insert(Vector(13185.6, 0, 1691.9)); speeds.Insert(14.8);
		route.Insert(Vector(13178.8, 0, 1696.1)); speeds.Insert(15.0);
		route.Insert(Vector(13172.0, 0, 1700.3)); speeds.Insert(15.2);
		route.Insert(Vector(13165.2, 0, 1704.5)); speeds.Insert(15.5);
		route.Insert(Vector(13158.4, 0, 1708.8)); speeds.Insert(15.7);
		route.Insert(Vector(13151.6, 0, 1713.0)); speeds.Insert(15.9);
		route.Insert(Vector(13144.8, 0, 1717.2)); speeds.Insert(16.1);
		route.Insert(Vector(13138.0, 0, 1721.4)); speeds.Insert(16.3);
		route.Insert(Vector(13131.2, 0, 1725.6)); speeds.Insert(16.4);
		route.Insert(Vector(13124.3, 0, 1729.7)); speeds.Insert(16.6);
		route.Insert(Vector(13117.5, 0, 1733.9)); speeds.Insert(16.7);
		route.Insert(Vector(13110.7, 0, 1738.0)); speeds.Insert(16.5);
		route.Insert(Vector(13103.8, 0, 1742.2)); speeds.Insert(16.3);
		route.Insert(Vector(13097.0, 0, 1746.3)); speeds.Insert(16.1);
		route.Insert(Vector(13090.2, 0, 1750.5)); speeds.Insert(15.4);
		route.Insert(Vector(13083.4, 0, 1754.8)); speeds.Insert(15.3);
		route.Insert(Vector(13076.6, 0, 1758.9)); speeds.Insert(15.5);
		route.Insert(Vector(13069.8, 0, 1763.2)); speeds.Insert(15.7);
		route.Insert(Vector(13063.0, 0, 1767.4)); speeds.Insert(16.0);
		route.Insert(Vector(13056.1, 0, 1771.5)); speeds.Insert(16.2);
		route.Insert(Vector(13049.3, 0, 1775.6)); speeds.Insert(16.4);
		route.Insert(Vector(13042.4, 0, 1779.8)); speeds.Insert(16.6);
		route.Insert(Vector(13035.6, 0, 1784.0)); speeds.Insert(16.7);
		route.Insert(Vector(13028.9, 0, 1788.3)); speeds.Insert(16.4);
		route.Insert(Vector(13022.1, 0, 1792.5)); speeds.Insert(15.5);
		route.Insert(Vector(13015.4, 0, 1796.9)); speeds.Insert(15.3);
		route.Insert(Vector(13008.7, 0, 1801.3)); speeds.Insert(15.4);
		route.Insert(Vector(13002.2, 0, 1805.9)); speeds.Insert(15.7);
		route.Insert(Vector(12995.6, 0, 1810.5)); speeds.Insert(15.9);
		route.Insert(Vector(12989.0, 0, 1815.1)); speeds.Insert(16.2);
		route.Insert(Vector(12982.5, 0, 1819.7)); speeds.Insert(16.4);
		route.Insert(Vector(12976.0, 0, 1824.3)); speeds.Insert(16.6);
		route.Insert(Vector(12969.5, 0, 1829.0)); speeds.Insert(16.8);
		route.Insert(Vector(12963.1, 0, 1833.8)); speeds.Insert(17.0);
		route.Insert(Vector(12956.8, 0, 1838.7)); speeds.Insert(16.9);
		route.Insert(Vector(12950.4, 0, 1843.5)); speeds.Insert(16.6);
		route.Insert(Vector(12944.0, 0, 1848.4)); speeds.Insert(15.5);
		route.Insert(Vector(12937.7, 0, 1853.2)); speeds.Insert(14.0);
		route.Insert(Vector(12931.3, 0, 1858.1)); speeds.Insert(13.8);
		route.Insert(Vector(12925.1, 0, 1863.1)); speeds.Insert(13.3);
		route.Insert(Vector(12918.9, 0, 1868.2)); speeds.Insert(13.1);
		route.Insert(Vector(12912.7, 0, 1873.3)); speeds.Insert(13.3);
		route.Insert(Vector(12906.6, 0, 1878.4)); speeds.Insert(13.7);
		route.Insert(Vector(12900.5, 0, 1883.6)); speeds.Insert(13.9);
		route.Insert(Vector(12894.5, 0, 1888.9)); speeds.Insert(14.2);
		route.Insert(Vector(12888.5, 0, 1894.1)); speeds.Insert(14.5);
		route.Insert(Vector(12882.4, 0, 1899.4)); speeds.Insert(14.7);
		route.Insert(Vector(12876.4, 0, 1904.7)); speeds.Insert(15.0);
		route.Insert(Vector(12870.5, 0, 1910.0)); speeds.Insert(15.2);
		route.Insert(Vector(12864.5, 0, 1915.3)); speeds.Insert(15.4);
		route.Insert(Vector(12858.5, 0, 1920.6)); speeds.Insert(15.6);
		route.Insert(Vector(12852.5, 0, 1925.9)); speeds.Insert(15.9);
		route.Insert(Vector(12846.6, 0, 1931.3)); speeds.Insert(16.1);
		route.Insert(Vector(12840.6, 0, 1936.6)); speeds.Insert(16.2);
		route.Insert(Vector(12834.6, 0, 1941.9)); speeds.Insert(16.4);
		route.Insert(Vector(12828.6, 0, 1947.2)); speeds.Insert(16.6);
		route.Insert(Vector(12822.6, 0, 1952.5)); speeds.Insert(16.8);
		route.Insert(Vector(12816.7, 0, 1957.8)); speeds.Insert(16.9);
		route.Insert(Vector(12810.7, 0, 1963.1)); speeds.Insert(17.0);
		route.Insert(Vector(12804.7, 0, 1968.5)); speeds.Insert(17.1);
		route.Insert(Vector(12798.7, 0, 1973.8)); speeds.Insert(17.2);
		route.Insert(Vector(12792.7, 0, 1979.1)); speeds.Insert(17.3);
		route.Insert(Vector(12786.8, 0, 1984.4)); speeds.Insert(17.4);
		route.Insert(Vector(12780.8, 0, 1989.7)); speeds.Insert(17.5);
		route.Insert(Vector(12774.8, 0, 1995.0)); speeds.Insert(17.6);
		route.Insert(Vector(12768.8, 0, 2000.3)); speeds.Insert(17.6);
		route.Insert(Vector(12762.8, 0, 2005.6)); speeds.Insert(17.7);
		route.Insert(Vector(12756.8, 0, 2011.0)); speeds.Insert(17.5);
		route.Insert(Vector(12750.9, 0, 2016.3)); speeds.Insert(17.0);
		route.Insert(Vector(12744.9, 0, 2021.6)); speeds.Insert(14.8);
		route.Insert(Vector(12738.9, 0, 2026.9)); speeds.Insert(14.6);
		route.Insert(Vector(12732.9, 0, 2032.2)); speeds.Insert(14.4);
		route.Insert(Vector(12726.9, 0, 2037.5)); speeds.Insert(14.4);
		route.Insert(Vector(12720.9, 0, 2042.7)); speeds.Insert(14.1);
		route.Insert(Vector(12714.9, 0, 2048.0)); speeds.Insert(14.2);
		route.Insert(Vector(12708.8, 0, 2053.2)); speeds.Insert(14.4);
		route.Insert(Vector(12702.7, 0, 2058.5)); speeds.Insert(14.6);
		route.Insert(Vector(12696.7, 0, 2063.7)); speeds.Insert(14.8);
		route.Insert(Vector(12690.6, 0, 2068.9)); speeds.Insert(15.0);
		route.Insert(Vector(12684.5, 0, 2074.0)); speeds.Insert(15.2);
		route.Insert(Vector(12678.3, 0, 2079.1)); speeds.Insert(15.4);
		route.Insert(Vector(12672.1, 0, 2084.2)); speeds.Insert(15.6);
		route.Insert(Vector(12665.9, 0, 2089.3)); speeds.Insert(15.8);
		route.Insert(Vector(12659.8, 0, 2094.4)); speeds.Insert(16.0);
		route.Insert(Vector(12653.6, 0, 2099.4)); speeds.Insert(16.2);
		route.Insert(Vector(12647.3, 0, 2104.4)); speeds.Insert(16.3);
		route.Insert(Vector(12641.1, 0, 2109.4)); speeds.Insert(16.5);
		route.Insert(Vector(12634.8, 0, 2114.3)); speeds.Insert(16.6);
		route.Insert(Vector(12628.5, 0, 2119.3)); speeds.Insert(16.7);
		route.Insert(Vector(12622.1, 0, 2124.2)); speeds.Insert(16.9);
		route.Insert(Vector(12615.8, 0, 2129.1)); speeds.Insert(17.0);
		route.Insert(Vector(12609.5, 0, 2134.0)); speeds.Insert(17.1);
		route.Insert(Vector(12603.2, 0, 2138.9)); speeds.Insert(17.2);
		route.Insert(Vector(12596.8, 0, 2143.7)); speeds.Insert(17.3);
		route.Insert(Vector(12590.4, 0, 2148.5)); speeds.Insert(17.4);
		route.Insert(Vector(12584.0, 0, 2153.3)); speeds.Insert(17.4);
		route.Insert(Vector(12577.5, 0, 2158.0)); speeds.Insert(17.5);
		route.Insert(Vector(12571.1, 0, 2162.8)); speeds.Insert(17.6);
		route.Insert(Vector(12564.7, 0, 2167.5)); speeds.Insert(17.6);
		route.Insert(Vector(12558.2, 0, 2172.3)); speeds.Insert(17.7);
		route.Insert(Vector(12551.8, 0, 2177.0)); speeds.Insert(17.7);
		route.Insert(Vector(12545.4, 0, 2181.8)); speeds.Insert(17.7);
		route.Insert(Vector(12538.9, 0, 2186.5)); speeds.Insert(17.8);
		route.Insert(Vector(12532.5, 0, 2191.3)); speeds.Insert(17.8);
		route.Insert(Vector(12526.1, 0, 2196.1)); speeds.Insert(17.8);
		route.Insert(Vector(12519.7, 0, 2200.9)); speeds.Insert(17.8);
		route.Insert(Vector(12513.3, 0, 2205.7)); speeds.Insert(17.8);
		route.Insert(Vector(12506.9, 0, 2210.5)); speeds.Insert(17.8);
		route.Insert(Vector(12500.6, 0, 2215.4)); speeds.Insert(17.9);
		route.Insert(Vector(12494.2, 0, 2220.2)); speeds.Insert(17.9);
		route.Insert(Vector(12487.8, 0, 2225.1)); speeds.Insert(17.9);
		route.Insert(Vector(12481.5, 0, 2229.9)); speeds.Insert(18.0);
		route.Insert(Vector(12475.1, 0, 2234.8)); speeds.Insert(18.0);
		route.Insert(Vector(12468.7, 0, 2239.6)); speeds.Insert(18.0);
		route.Insert(Vector(12462.4, 0, 2244.4)); speeds.Insert(18.1);
		route.Insert(Vector(12456.0, 0, 2249.3)); speeds.Insert(18.1);
		route.Insert(Vector(12449.6, 0, 2254.1)); speeds.Insert(18.1);
		route.Insert(Vector(12443.3, 0, 2259.0)); speeds.Insert(18.2);
		route.Insert(Vector(12436.9, 0, 2263.8)); speeds.Insert(18.2);
		route.Insert(Vector(12430.5, 0, 2268.6)); speeds.Insert(18.2);
		route.Insert(Vector(12424.0, 0, 2273.3)); speeds.Insert(18.2);
		route.Insert(Vector(12417.5, 0, 2278.0)); speeds.Insert(18.2);
		route.Insert(Vector(12411.0, 0, 2282.7)); speeds.Insert(18.2);
		route.Insert(Vector(12404.5, 0, 2287.3)); speeds.Insert(18.1);
		route.Insert(Vector(12398.0, 0, 2292.0)); speeds.Insert(18.1);
		route.Insert(Vector(12391.5, 0, 2296.7)); speeds.Insert(18.1);
		route.Insert(Vector(12385.0, 0, 2301.3)); speeds.Insert(18.0);
		route.Insert(Vector(12378.5, 0, 2306.0)); speeds.Insert(18.0);
		route.Insert(Vector(12372.0, 0, 2310.6)); speeds.Insert(18.0);
		route.Insert(Vector(12365.5, 0, 2315.3)); speeds.Insert(17.9);
		route.Insert(Vector(12359.0, 0, 2319.9)); speeds.Insert(17.9);
		route.Insert(Vector(12352.5, 0, 2324.6)); speeds.Insert(17.9);
		route.Insert(Vector(12346.0, 0, 2329.2)); speeds.Insert(17.9);
		route.Insert(Vector(12339.5, 0, 2333.9)); speeds.Insert(17.9);
		route.Insert(Vector(12332.9, 0, 2338.5)); speeds.Insert(17.9);
		route.Insert(Vector(12326.4, 0, 2343.2)); speeds.Insert(17.9);
		route.Insert(Vector(12319.9, 0, 2347.8)); speeds.Insert(17.9);
		route.Insert(Vector(12313.4, 0, 2352.4)); speeds.Insert(17.9);
		route.Insert(Vector(12306.9, 0, 2357.1)); speeds.Insert(17.9);
		route.Insert(Vector(12300.4, 0, 2361.7)); speeds.Insert(17.8);
		route.Insert(Vector(12293.8, 0, 2366.4)); speeds.Insert(17.8);
		route.Insert(Vector(12287.3, 0, 2370.9)); speeds.Insert(17.8);
		route.Insert(Vector(12280.7, 0, 2375.5)); speeds.Insert(17.9);
		route.Insert(Vector(12274.0, 0, 2379.9)); speeds.Insert(17.9);
		route.Insert(Vector(12267.4, 0, 2384.3)); speeds.Insert(17.9);
		route.Insert(Vector(12260.7, 0, 2388.7)); speeds.Insert(18.0);
		route.Insert(Vector(12254.0, 0, 2393.1)); speeds.Insert(18.0);
		route.Insert(Vector(12247.3, 0, 2397.5)); speeds.Insert(18.0);
		route.Insert(Vector(12240.6, 0, 2401.9)); speeds.Insert(18.1);
		route.Insert(Vector(12233.9, 0, 2406.3)); speeds.Insert(18.1);
		route.Insert(Vector(12227.2, 0, 2410.6)); speeds.Insert(18.1);
		route.Insert(Vector(12220.4, 0, 2414.8)); speeds.Insert(18.0);
		route.Insert(Vector(12213.6, 0, 2419.1)); speeds.Insert(18.0);
		route.Insert(Vector(12206.8, 0, 2423.2)); speeds.Insert(17.9);
		route.Insert(Vector(12199.9, 0, 2427.3)); speeds.Insert(17.9);
		route.Insert(Vector(12193.1, 0, 2431.4)); speeds.Insert(18.0);
		route.Insert(Vector(12186.2, 0, 2435.5)); speeds.Insert(18.0);
		route.Insert(Vector(12179.2, 0, 2439.5)); speeds.Insert(18.1);
		route.Insert(Vector(12172.3, 0, 2443.5)); speeds.Insert(18.1);
		route.Insert(Vector(12165.4, 0, 2447.6)); speeds.Insert(18.1);
		route.Insert(Vector(12158.5, 0, 2451.6)); speeds.Insert(18.2);
		route.Insert(Vector(12151.6, 0, 2455.6)); speeds.Insert(18.2);
		route.Insert(Vector(12144.7, 0, 2459.7)); speeds.Insert(18.2);
		route.Insert(Vector(12137.8, 0, 2463.7)); speeds.Insert(18.3);
		route.Insert(Vector(12130.9, 0, 2467.7)); speeds.Insert(18.3);
		route.Insert(Vector(12124.0, 0, 2471.8)); speeds.Insert(18.3);
		route.Insert(Vector(12117.1, 0, 2475.8)); speeds.Insert(18.4);
		route.Insert(Vector(12110.1, 0, 2479.8)); speeds.Insert(18.4);
		route.Insert(Vector(12103.2, 0, 2483.8)); speeds.Insert(18.4);
		route.Insert(Vector(12096.3, 0, 2487.8)); speeds.Insert(18.4);
		route.Insert(Vector(12089.3, 0, 2491.8)); speeds.Insert(18.5);
		route.Insert(Vector(12082.4, 0, 2495.8)); speeds.Insert(18.5);
		route.Insert(Vector(12075.5, 0, 2499.8)); speeds.Insert(18.5);
		route.Insert(Vector(12068.6, 0, 2503.8)); speeds.Insert(18.5);
		route.Insert(Vector(12061.6, 0, 2507.8)); speeds.Insert(18.5);
		route.Insert(Vector(12054.7, 0, 2511.8)); speeds.Insert(18.6);
		route.Insert(Vector(12047.8, 0, 2515.8)); speeds.Insert(18.6);
		route.Insert(Vector(12040.8, 0, 2519.8)); speeds.Insert(18.6);
		route.Insert(Vector(12033.9, 0, 2523.8)); speeds.Insert(18.6);
		route.Insert(Vector(12027.0, 0, 2527.8)); speeds.Insert(18.6);
		route.Insert(Vector(12020.1, 0, 2531.8)); speeds.Insert(18.6);
		route.Insert(Vector(12013.1, 0, 2535.8)); speeds.Insert(18.7);
		route.Insert(Vector(12006.2, 0, 2539.8)); speeds.Insert(18.7);
		route.Insert(Vector(11999.3, 0, 2543.8)); speeds.Insert(18.7);
		route.Insert(Vector(11992.3, 0, 2547.8)); speeds.Insert(18.7);
		route.Insert(Vector(11985.4, 0, 2551.7)); speeds.Insert(18.7);
		route.Insert(Vector(11978.5, 0, 2555.7)); speeds.Insert(18.7);
		route.Insert(Vector(11971.5, 0, 2559.7)); speeds.Insert(18.7);
		route.Insert(Vector(11964.6, 0, 2563.7)); speeds.Insert(18.6);
		route.Insert(Vector(11957.7, 0, 2567.7)); speeds.Insert(18.6);
		route.Insert(Vector(11950.7, 0, 2571.7)); speeds.Insert(18.6);
		route.Insert(Vector(11943.8, 0, 2575.7)); speeds.Insert(18.6);
		route.Insert(Vector(11936.9, 0, 2579.7)); speeds.Insert(18.5);
		route.Insert(Vector(11929.9, 0, 2583.7)); speeds.Insert(18.5);
		route.Insert(Vector(11922.9, 0, 2587.5)); speeds.Insert(18.5);
		route.Insert(Vector(11915.8, 0, 2591.2)); speeds.Insert(18.4);
		route.Insert(Vector(11908.7, 0, 2595.0)); speeds.Insert(18.4);
		route.Insert(Vector(11901.7, 0, 2598.7)); speeds.Insert(18.4);
		route.Insert(Vector(11894.6, 0, 2602.4)); speeds.Insert(18.4);
		route.Insert(Vector(11887.5, 0, 2606.1)); speeds.Insert(18.4);
		route.Insert(Vector(11880.4, 0, 2609.9)); speeds.Insert(18.3);
		route.Insert(Vector(11873.3, 0, 2613.6)); speeds.Insert(18.3);
		route.Insert(Vector(11866.2, 0, 2617.1)); speeds.Insert(18.3);
		route.Insert(Vector(11859.0, 0, 2620.7)); speeds.Insert(18.2);
		route.Insert(Vector(11851.9, 0, 2624.3)); speeds.Insert(18.2);
		route.Insert(Vector(11844.7, 0, 2627.9)); speeds.Insert(18.2);
		route.Insert(Vector(11837.6, 0, 2631.6)); speeds.Insert(18.2);
		route.Insert(Vector(11830.5, 0, 2635.3)); speeds.Insert(18.2);
		route.Insert(Vector(11823.5, 0, 2639.1)); speeds.Insert(18.2);
		route.Insert(Vector(11816.4, 0, 2642.8)); speeds.Insert(18.2);
		route.Insert(Vector(11809.4, 0, 2646.6)); speeds.Insert(18.2);
		route.Insert(Vector(11802.3, 0, 2650.4)); speeds.Insert(18.2);
		route.Insert(Vector(11795.3, 0, 2654.2)); speeds.Insert(18.1);
		route.Insert(Vector(11788.2, 0, 2657.9)); speeds.Insert(18.1);
		route.Insert(Vector(11781.2, 0, 2661.7)); speeds.Insert(18.1);
		route.Insert(Vector(11774.1, 0, 2665.5)); speeds.Insert(18.1);
		route.Insert(Vector(11767.1, 0, 2669.3)); speeds.Insert(18.1);
		route.Insert(Vector(11760.0, 0, 2673.1)); speeds.Insert(18.1);
		route.Insert(Vector(11753.0, 0, 2676.8)); speeds.Insert(18.1);
		route.Insert(Vector(11746.0, 0, 2680.7)); speeds.Insert(18.1);
		route.Insert(Vector(11739.0, 0, 2684.7)); speeds.Insert(18.1);
		route.Insert(Vector(11732.1, 0, 2688.7)); speeds.Insert(18.1);
		route.Insert(Vector(11725.2, 0, 2692.7)); speeds.Insert(18.1);
		route.Insert(Vector(11718.4, 0, 2696.9)); speeds.Insert(18.1);
		route.Insert(Vector(11711.6, 0, 2701.2)); speeds.Insert(18.1);
		route.Insert(Vector(11704.9, 0, 2705.5)); speeds.Insert(18.2);
		route.Insert(Vector(11698.1, 0, 2709.8)); speeds.Insert(18.2);
		route.Insert(Vector(11691.3, 0, 2714.0)); speeds.Insert(18.3);
		route.Insert(Vector(11684.6, 0, 2718.3)); speeds.Insert(18.3);
		route.Insert(Vector(11677.8, 0, 2722.6)); speeds.Insert(18.3);
		route.Insert(Vector(11671.1, 0, 2726.9)); speeds.Insert(18.4);
		route.Insert(Vector(11664.3, 0, 2731.2)); speeds.Insert(18.4);
		route.Insert(Vector(11657.5, 0, 2735.4)); speeds.Insert(18.3);
		route.Insert(Vector(11650.6, 0, 2739.4)); speeds.Insert(18.3);
		route.Insert(Vector(11643.6, 0, 2743.3)); speeds.Insert(18.3);
		route.Insert(Vector(11636.7, 0, 2747.3)); speeds.Insert(18.3);
		route.Insert(Vector(11629.8, 0, 2751.3)); speeds.Insert(18.3);
		route.Insert(Vector(11622.8, 0, 2755.3)); speeds.Insert(18.3);
		route.Insert(Vector(11615.9, 0, 2759.2)); speeds.Insert(18.3);
		route.Insert(Vector(11608.9, 0, 2763.2)); speeds.Insert(18.3);
		route.Insert(Vector(11601.9, 0, 2767.1)); speeds.Insert(18.3);
		route.Insert(Vector(11594.9, 0, 2771.0)); speeds.Insert(18.2);
		route.Insert(Vector(11587.9, 0, 2774.9)); speeds.Insert(18.2);
		route.Insert(Vector(11580.9, 0, 2778.7)); speeds.Insert(18.2);
		route.Insert(Vector(11573.9, 0, 2782.5)); speeds.Insert(18.2);
		route.Insert(Vector(11566.9, 0, 2786.3)); speeds.Insert(18.2);
		route.Insert(Vector(11559.8, 0, 2790.1)); speeds.Insert(18.2);
		route.Insert(Vector(11552.8, 0, 2794.0)); speeds.Insert(18.2);
		route.Insert(Vector(11545.8, 0, 2797.8)); speeds.Insert(18.2);
		route.Insert(Vector(11538.8, 0, 2801.7)); speeds.Insert(18.1);
		route.Insert(Vector(11531.8, 0, 2805.5)); speeds.Insert(18.1);
		route.Insert(Vector(11524.8, 0, 2809.4)); speeds.Insert(18.1);
		route.Insert(Vector(11517.7, 0, 2813.2)); speeds.Insert(18.1);
		route.Insert(Vector(11510.7, 0, 2817.1)); speeds.Insert(18.1);
		route.Insert(Vector(11503.7, 0, 2820.9)); speeds.Insert(18.1);
		route.Insert(Vector(11496.7, 0, 2824.8)); speeds.Insert(18.1);
		route.Insert(Vector(11489.7, 0, 2828.7)); speeds.Insert(18.1);
		route.Insert(Vector(11482.7, 0, 2832.5)); speeds.Insert(18.1);
		route.Insert(Vector(11475.7, 0, 2836.4)); speeds.Insert(18.1);
		route.Insert(Vector(11468.7, 0, 2840.3)); speeds.Insert(18.1);
		route.Insert(Vector(11461.7, 0, 2844.2)); speeds.Insert(18.1);
		route.Insert(Vector(11454.7, 0, 2848.1)); speeds.Insert(18.1);
		route.Insert(Vector(11447.8, 0, 2852.0)); speeds.Insert(18.1);
		route.Insert(Vector(11440.8, 0, 2856.0)); speeds.Insert(18.1);
		route.Insert(Vector(11433.8, 0, 2859.9)); speeds.Insert(18.1);
		route.Insert(Vector(11426.9, 0, 2863.8)); speeds.Insert(18.0);
		route.Insert(Vector(11419.9, 0, 2867.7)); speeds.Insert(18.0);
		route.Insert(Vector(11412.9, 0, 2871.6)); speeds.Insert(18.0);
		route.Insert(Vector(11405.9, 0, 2875.5)); speeds.Insert(18.0);
		route.Insert(Vector(11398.9, 0, 2879.3)); speeds.Insert(18.0);
		route.Insert(Vector(11391.9, 0, 2883.2)); speeds.Insert(18.0);
		route.Insert(Vector(11384.9, 0, 2887.1)); speeds.Insert(18.0);
		route.Insert(Vector(11377.9, 0, 2891.0)); speeds.Insert(18.0);
		route.Insert(Vector(11370.9, 0, 2894.8)); speeds.Insert(18.0);
		route.Insert(Vector(11363.9, 0, 2898.7)); speeds.Insert(18.0);
		route.Insert(Vector(11356.8, 0, 2902.5)); speeds.Insert(18.0);
		route.Insert(Vector(11349.8, 0, 2906.3)); speeds.Insert(18.0);
		route.Insert(Vector(11342.8, 0, 2910.1)); speeds.Insert(18.0);
		route.Insert(Vector(11335.7, 0, 2913.9)); speeds.Insert(18.0);
		route.Insert(Vector(11328.7, 0, 2917.6)); speeds.Insert(18.0);
		route.Insert(Vector(11321.6, 0, 2921.4)); speeds.Insert(18.0);
		route.Insert(Vector(11314.5, 0, 2925.1)); speeds.Insert(17.9);
		route.Insert(Vector(11307.4, 0, 2928.8)); speeds.Insert(17.9);
		route.Insert(Vector(11300.3, 0, 2932.6)); speeds.Insert(17.9);
		route.Insert(Vector(11293.3, 0, 2936.3)); speeds.Insert(17.8);
		route.Insert(Vector(11286.2, 0, 2940.0)); speeds.Insert(17.8);
		route.Insert(Vector(11279.1, 0, 2943.8)); speeds.Insert(17.7);
		route.Insert(Vector(11272.1, 0, 2947.6)); speeds.Insert(17.7);
		route.Insert(Vector(11265.1, 0, 2951.4)); speeds.Insert(17.7);
		route.Insert(Vector(11258.0, 0, 2955.2)); speeds.Insert(17.7);
		route.Insert(Vector(11250.9, 0, 2958.9)); speeds.Insert(17.7);
		route.Insert(Vector(11243.9, 0, 2962.7)); speeds.Insert(17.7);
		route.Insert(Vector(11236.8, 0, 2966.5)); speeds.Insert(17.7);
		route.Insert(Vector(11229.8, 0, 2970.3)); speeds.Insert(17.7);
		route.Insert(Vector(11222.8, 0, 2974.1)); speeds.Insert(17.7);
		route.Insert(Vector(11215.8, 0, 2978.0)); speeds.Insert(17.7);
		route.Insert(Vector(11208.8, 0, 2981.8)); speeds.Insert(17.8);
		route.Insert(Vector(11201.8, 0, 2985.7)); speeds.Insert(17.8);
		route.Insert(Vector(11194.8, 0, 2989.6)); speeds.Insert(17.8);
		route.Insert(Vector(11187.8, 0, 2993.5)); speeds.Insert(17.8);
		route.Insert(Vector(11180.8, 0, 2997.3)); speeds.Insert(17.8);
		route.Insert(Vector(11173.8, 0, 3001.2)); speeds.Insert(17.8);
		route.Insert(Vector(11166.8, 0, 3005.1)); speeds.Insert(17.8);
		route.Insert(Vector(11159.8, 0, 3009.0)); speeds.Insert(17.8);
		route.Insert(Vector(11152.8, 0, 3012.8)); speeds.Insert(17.8);
		route.Insert(Vector(11145.8, 0, 3016.7)); speeds.Insert(17.8);
		route.Insert(Vector(11138.8, 0, 3020.7)); speeds.Insert(17.8);
		route.Insert(Vector(11131.9, 0, 3024.7)); speeds.Insert(17.8);
		route.Insert(Vector(11125.0, 0, 3028.7)); speeds.Insert(17.7);
		route.Insert(Vector(11118.1, 0, 3032.7)); speeds.Insert(17.7);
		route.Insert(Vector(11111.2, 0, 3036.7)); speeds.Insert(17.7);
		route.Insert(Vector(11104.3, 0, 3040.8)); speeds.Insert(17.7);
		route.Insert(Vector(11097.5, 0, 3045.0)); speeds.Insert(17.7);
		route.Insert(Vector(11090.7, 0, 3049.2)); speeds.Insert(17.7);
		route.Insert(Vector(11083.9, 0, 3053.5)); speeds.Insert(17.7);
		route.Insert(Vector(11077.1, 0, 3057.8)); speeds.Insert(17.7);
		route.Insert(Vector(11070.4, 0, 3062.1)); speeds.Insert(17.5);
		route.Insert(Vector(11063.7, 0, 3066.5)); speeds.Insert(17.1);
		route.Insert(Vector(11057.0, 0, 3070.9)); speeds.Insert(17.1);
		route.Insert(Vector(11050.4, 0, 3075.4)); speeds.Insert(17.0);
		route.Insert(Vector(11043.7, 0, 3079.8)); speeds.Insert(17.0);
		route.Insert(Vector(11037.1, 0, 3084.2)); speeds.Insert(16.9);
		route.Insert(Vector(11030.4, 0, 3088.7)); speeds.Insert(16.9);
		route.Insert(Vector(11023.8, 0, 3093.1)); speeds.Insert(16.9);
		route.Insert(Vector(11017.1, 0, 3097.6)); speeds.Insert(16.9);
		route.Insert(Vector(11010.5, 0, 3102.1)); speeds.Insert(16.9);
		route.Insert(Vector(11003.9, 0, 3106.6)); speeds.Insert(16.8);
		route.Insert(Vector(10997.3, 0, 3111.1)); speeds.Insert(16.7);
		route.Insert(Vector(10990.7, 0, 3115.6)); speeds.Insert(16.7);
		route.Insert(Vector(10984.1, 0, 3120.2)); speeds.Insert(16.7);
		route.Insert(Vector(10977.5, 0, 3124.7)); speeds.Insert(16.7);
		route.Insert(Vector(10970.9, 0, 3129.2)); speeds.Insert(16.8);
		route.Insert(Vector(10964.3, 0, 3133.7)); speeds.Insert(16.8);
		route.Insert(Vector(10957.7, 0, 3138.3)); speeds.Insert(16.8);
		route.Insert(Vector(10951.1, 0, 3142.8)); speeds.Insert(16.9);
		route.Insert(Vector(10944.5, 0, 3147.3)); speeds.Insert(16.9);
		route.Insert(Vector(10937.9, 0, 3151.8)); speeds.Insert(17.0);
		route.Insert(Vector(10931.3, 0, 3156.4)); speeds.Insert(17.0);
		route.Insert(Vector(10924.7, 0, 3160.9)); speeds.Insert(17.0);
		route.Insert(Vector(10918.1, 0, 3165.4)); speeds.Insert(17.0);
		route.Insert(Vector(10911.5, 0, 3169.9)); speeds.Insert(17.0);
		route.Insert(Vector(10910.7, 0, 3170.5)); speeds.Insert(17.0);
	}
}

modded class DayZIntroScenePC
{
	static const float DL_CAR_GAP = 8;        // m between the vehicles of a convoy
	static const float DL_BOAT_GAP = 11;      // boats: m back ...
	static const float DL_BOAT_STEP = 5;      // ... and m out to the side, each (an echelon: the line recedes)
	protected ref DL_MenuScene m_DL_Scene;
	// made in DL_BuildRoad: SetScene runs from the base constructor, before these members' initialisers
	protected ref array<Object> m_DL_Made;
	protected ref array<Transport> m_DL_Cars;
	protected vector m_DL_LookAt;
	protected bool m_DL_Aimed;
	// boats under way: they sail on along the heading, the camera rides along; after a run of open water all of it
	// jumps back to the start (fog and sea hide the cut)
	static const float DL_SAIL_SPEED = 5;     // m/s, about 18 km/h
	protected bool m_DL_Sailing;
	protected vector m_DL_SailDir;
	protected ref array<vector> m_DL_SailStart;
	// boats on a recorded sea route (Frostline: the one way in and out is by water): they follow it like the cars
	// their road, each in its place of the group (behind, and out to one side or the other)
	protected bool m_DL_SeaRoute;
	protected ref array<float> m_DL_BoatS;      // each boat's distance along the route (m)
	static const float DL_SEA_SPEED_X = 0.8;    // share of the recorded speed the group sails at
	protected vector m_DL_CamOff;             // camera - lead boat, at the start
	protected vector m_DL_LookOff;            // look target - lead boat
	protected vector m_DL_Follow;             // the lead boat's position, smoothed (no bobbing camera)
	protected float m_DL_SailRun;             // m of open water ahead of every boat
	protected float m_DL_Sailed;
	protected int m_DL_SailAt;                // g_Game.GetTime() when they set off (the crew sits down first)
	protected int m_DL_SailLast;
	protected bool m_DL_BoatsOn;              // the boats' engines are started
	protected int m_DL_FogAt;
	// cars under way: they drive along a path traced down the road (it follows the bends), the camera keeps its place
	// beside and ahead of the lead car in the road's own frame; at the path's end all of it starts over
	static const float DL_DRIVE_SPEED = 8;    // m/s, about 29 km/h
	static const float DL_PATH_STEP = 4;      // m between the path's points
	static const float DL_RUN_END = 40;       // the run ends this far before the route's end
	protected bool m_DL_Driving;
	protected float m_DL_SpotS;               // the spot's distance along the path
	protected ref array<vector> m_DL_Path;
	protected ref array<float> m_DL_CarS;     // each car's distance along the path (m), from its position
	protected ref array<float> m_DL_PathV;    // the recorded speed at each path point (m/s), or null
	static const float DL_REC_SPEED_X = 0.7;  // the convoy drives a recorded route at this share of the recorded speed
	protected ref array<int> m_DL_ShiftAt;     // g_Game.GetTime() of each car's last gear change
	protected ref array<float> m_DL_StuckFor;  // s each car has had throttle without moving
	protected ref map<int, float> m_DL_Asked = new map<int, float>; // the throttle each car was last given
	protected ref map<int, int> m_DL_Rescues = new map<int, int>;   // times each car was put back on the road
	protected ref map<int, bool> m_DL_Glide = new map<int, bool>;   // a car that can't drive (engine, parts): glided
	protected ref map<int, int> m_DL_GlideAt = new map<int, int>;   // when it is frozen for that (wheels straightened first)
	protected bool m_DL_HasTruck;             // a truck in the convoy: everyone drives the recorded speeds
	protected float m_DL_CamPull;             // the camera pulled in towards the lead (a tree, a rock in the way)
	protected float m_DL_CamAlong;            // camera / look target relative to the lead car: along the road,
	protected float m_DL_CamSide;             // out to the side,
	protected float m_DL_CamUp;               // and up
	protected float m_DL_LookAlong;
	protected float m_DL_LookSide;
	protected vector m_DL_CamDir;             // the road's direction at the lead car, smoothed
	protected int m_DL_DriveAt;
	protected int m_DL_DriveLast;
	protected ref array<float> m_DL_CarLift;  // each car's origin above the road on its wheels (for the jump back)
	protected float m_DL_Gap;
	protected ref array<float> m_DL_GapOf;    // m of road between each vehicle and the one ahead (by their lengths)
	protected ref array<float> m_DL_Back;     // each vehicle's place behind the lead (m)
	protected float m_DL_LeadSpeed;           // the lead's target speed now (m/s)
	protected bool m_DL_EnginesOn;
	protected ref map<int, bool> m_DL_ShiftUp = new map<int, bool>;   // each vehicle's last shift was up
	protected ref map<int, float> m_DL_LastV = new map<int, float>;   // its speed (m/s) a second ago,
	protected ref map<int, float> m_DL_UpSpeed = new map<int, float>; // vehicle * 100 + gear: speed it left that gear at
	protected ref map<int, int> m_DL_AccelAt = new map<int, int>;     // when that was,
	protected ref map<int, float> m_DL_Accel = new map<int, float>;   // and its speed change since (m/s per s)
	protected int m_DL_FogLogAt;
	// people on foot (no vehicles): they walk the route in a loose group, two abreast, the camera walks with them
	static const float DL_WALK_ROW = 1.8;     // m between the rows
	static const float DL_WALK_SIDE = 0.75;   // m out from the road's middle line, either side
	protected bool m_DL_Walking;
	protected ref array<PlayerBase> m_DL_Walkers;
	protected ref array<float> m_DL_WalkTrack; // each walker's distance along the path (m)
	protected float m_DL_WalkLead;            // the group's front along the path (m)
	protected int m_DL_WalkAt;
	protected int m_DL_WalkLast;
	protected int m_DL_WalkLogAt;
	protected ref array<int> m_DL_WalkGo;     // when each walker sets off (staggered: their steps don't move in step)
	static const int DL_WALK_STAGGER = 1500;  // ms, at most
	protected int m_DL_MySlot;                // the menu character's place in the group + 1 (0: not walking)

	override void SetScene()
	{
		super.SetScene();
		m_DL_Scene = DL_MenuScene.Load();
		if (!m_DL_Scene || m_DL_Scene.Left() <= 0 || !m_Camera)
			return;
		DL_Log.Detail("menu scene: on the road to " + m_DL_Scene.target + ", " + m_DL_Scene.vehicles.Count().ToString() + " vehicles, " + m_DL_Scene.members.Count().ToString() + " people");
		DL_BuildRoad();
		DL_ThinFog();
	}

	// the vanilla scenes' weather can be thick fog, and it drifts back: keep the party visible
	protected void DL_ThinFog()
	{
		if (!m_Weather)
			return;
		int now = g_Game.GetTime();
		if (now - m_DL_FogLogAt > 20000)
		{
			m_DL_FogLogAt = now;
			string fl = "menu scene weather: fog " + m_Weather.GetFog().GetActual().ToString();
			fl += ", overcast " + m_Weather.GetOvercast().GetActual().ToString();
			fl += ", volumetric distance " + m_Weather.GetDynVolFogDistanceDensity().ToString();
			fl += " height " + m_Weather.GetDynVolFogHeightDensity().ToString();
			DL_Log.Detail(fl);
		}
		Fog fog = m_Weather.GetFog();
		if (fog.GetActual() > 0.05)
		{
			fog.SetLimits(0, 0.05);
			fog.Set(0.05, 0, 0);
		}
		Overcast oc = m_Weather.GetOvercast();
		if (oc.GetActual() > 0.4)
		{
			oc.SetLimits(0, 0.4);
			oc.Set(0.4, 0, 0);
		}
		// the volumetric fog works on a tiny scale and creeps up over minutes (0.0001 -> 0.006 in one lap): held about
		// where the scene starts
		if (m_Weather.GetDynVolFogDistanceDensity() > 0.0003)
			m_Weather.SetDynVolFogDistanceDensity(0.0003, 0);
		if (m_Weather.GetDynVolFogHeightDensity() > 0.005)
			m_Weather.SetDynVolFogHeightDensity(0.005, 0);
	}

	// the main menu turns its character with a mouse drag: not while the party is shown (he sits in a car)
	override void CharacterRotationStart()
	{
		if (m_DL_Aimed)
			return;
		super.CharacterRotationStart();
	}

	override void CharacterRotate()
	{
		if (m_DL_Aimed)
			return;
		super.CharacterRotate();
	}

	// the main menu aims the camera at the menu character when it opens (mainmenu.c Init); with the party shown it
	// keeps looking at the party (the seated character's own position is not where he sits)
	override void ResetIntroCamera()
	{
		if (m_DL_Aimed && m_Camera)
		{
			m_Camera.LookAt(m_DL_LookAt);
			return;
		}
		super.ResetIntroCamera();
	}

	protected override void ClearIntroScene()
	{
		if (m_DL_Made)
		{
			foreach (Object o : m_DL_Made)
			{
				if (o)
					g_Game.ObjectDelete(o);
			}
			m_DL_Made.Clear();
		}
		m_DL_Cars = null;
		super.ClearIntroScene();
	}

	// attach each item to e in its slot (or wherever it fits)
	// DayZ puts the first wheel a vehicle gets into its spare-wheel slot, ahead of the road wheels (even with the road
	// slot named), so with four wheels one corner stays empty and the car drags a bare hub (the Gunter crawled at
	// 1600 rpm). When nothing in the scene is meant for the spare slot, the spare slot gets one more of its wheels first.
	protected void DL_FillSpareFirst(EntityAI e, array<ref DL_MenuItem> items)
	{
		TStringArray slots = new TStringArray;
		g_Game.ConfigGetTextArray("CfgVehicles " + e.GetType() + " attachments", slots);
		bool hasSpare = false;
		foreach (string sn : slots)
		{
			string lsn = sn;
			lsn.ToLower();
			if (lsn.Contains("spare"))
				hasSpare = true;
		}
		if (!hasSpare)
			return;
		string wheel = "";
		foreach (DL_MenuItem it : items)
		{
			if (!it)
				continue;
			string ls = it.slot;
			ls.ToLower();
			if (ls.Contains("spare"))
				return;                                 // the scene has its own spare
			if (wheel == "" && g_Game.IsKindOf(it.type, "CarWheel"))
				wheel = it.type;
		}
		if (wheel == "")
			return;
		e.GetInventory().CreateAttachment(wheel);
		DL_Log.Detail("menu scene: " + e.GetType() + " gets a spare " + wheel + " first (keeps its road wheels on the road)");
	}

	protected void DL_Dress(EntityAI e, array<ref DL_MenuItem> items)
	{
		if (!e || !items)
			return;
		DL_FillSpareFirst(e, items);
		foreach (DL_MenuItem it : items)
		{
			if (!it || it.type == "" || !g_Game.ConfigIsExisting("CfgVehicles " + it.type) && !g_Game.ConfigIsExisting("CfgWeapons " + it.type) && !g_Game.ConfigIsExisting("CfgMagazines " + it.type))
				continue;                           // a class from a mod this client doesn't have at the menu
			int slot = -1;
			if (it.slot != "")
				slot = InventorySlots.GetSlotIdFromString(it.slot);
			EntityAI made;
			if (slot >= 0)
				made = e.GetInventory().CreateAttachmentEx(it.type, slot);
			if (!made)
			{
				if (slot >= 0)
					DL_Log.Detail("menu scene: " + it.type + " didn't go into " + it.slot + " (slot id " + slot.ToString() + ") on " + e.GetType());
				e.GetInventory().CreateAttachment(it.type);
			}
		}
	}


	protected bool DL_ClassExists(string type)
	{
		return type != "" && g_Game.ConfigIsExisting("CfgVehicles " + type);
	}

	protected void DL_BuildRoad()
	{
		m_DL_Made = new array<Object>;
		m_DL_Cars = new array<Transport>;
		m_DL_CarLift = new array<float>;
		bool boats = false;
		foreach (DL_MenuVehicle bv : m_DL_Scene.vehicles)
		{
			if (bv && g_Game.IsKindOf(bv.type, "BoatScript"))
				boats = true;
		}
		vector lead = m_CharacterPos;
		vector toCam = m_Camera.GetPosition() - m_CharacterPos;
		toCam[1] = 0;
		toCam.Normalize();
		// the vehicles face across the view with their left (driver's) side to the camera
		vector fwd = Vector(-toCam[2], 0, toCam[0]);
		// this map's own spot (a road, or open water for boats) instead of the vanilla scene's
		vector spotPos;
		vector spotFwd;
		bool roadFit = false;
		array<vector> route = new array<vector>;
		array<float> speeds = new array<float>;
		if (DL_MenuSpots.Find(g_Game.GetWorldName(), boats, spotPos, spotFwd, route, speeds))
		{
			lead = spotPos;
			fwd = spotFwd;
			if (!boats && route.Count() >= 2 && DL_MakePath(route, speeds))
			{
				m_DL_SpotS = DL_PathS(spotPos);
				lead = DL_PathAt(m_DL_SpotS);
				fwd = DL_PathDir(m_DL_SpotS);
				roadFit = true;
			}
			else if (boats && route.Count() >= 2 && DL_MakePath(route, speeds))
			{
				// on the water's surface, not the sea bed under it
				for (int pi = 0; pi < m_DL_Path.Count(); pi++)
				{
					vector pq = m_DL_Path[pi];
					pq[1] = DL_Ground(pq[0], pq[2]);
					m_DL_Path[pi] = pq;
				}
				m_DL_SpotS = DL_PathS(spotPos);
				lead = DL_PathAt(m_DL_SpotS);
				fwd = DL_PathDir(m_DL_SpotS);
				m_DL_SeaRoute = true;
			}
			toCam = Vector(fwd[2], 0, -fwd[0]);
			DL_Log.Detail("menu scene: " + g_Game.GetWorldName() + " spot " + spotPos.ToString() + ", heading " + Math.Round(Math.Atan2(fwd[0], fwd[2]) * Math.RAD2DEG).ToString());
		}
		else if (boats)
			DL_Log.Detail("menu scene: no water spot for " + g_Game.GetWorldName() + ", the boat crew stands");
		bool landBoats = boats && lead == m_CharacterPos;   // no water here: a boat would sit on land
		lead[1] = DL_Ground(lead[0], lead[2]);
		float yaw = Math.Atan2(fwd[0], fwd[2]) * Math.RAD2DEG;
		int vehCount = 0;
		float gap = DL_CAR_GAP;
		float step = 0;
		if (boats && !landBoats)
		{
			gap = DL_BOAT_GAP;
			step = DL_BOAT_STEP;
		}
		// on a route each vehicle is made right at its place on the road, facing along it: moving a freshly made heavy
		// vehicle there afterwards jolts it, and a V3S's stiff rear suspension bounces itself over from that
		float placeS = m_DL_SpotS;
		float prevLen = -1;
		foreach (DL_MenuVehicle mv : m_DL_Scene.vehicles)
		{
			Transport t;
			if (DL_ClassExists(mv.type) && !(landBoats && g_Game.IsKindOf(mv.type, "BoatScript")))
			{
				vector at = DL_LinePlace(lead, fwd, toCam, gap, step, vehCount);
				float vyaw = yaw;
				if (m_DL_SeaRoute && g_Game.IsKindOf(mv.type, "BoatScript"))
				{
					at = DL_BoatPlace(vehCount, m_DL_SpotS);
					vector bd = DL_PathDir(Math.Max(m_DL_SpotS - DL_BOAT_GAP * vehCount, 0));
					vyaw = Math.Atan2(bd[0], bd[2]) * Math.RAD2DEG;
				}
				if (roadFit && !boats)
				{
					if (prevLen >= 0)
						placeS = Math.Max(placeS - (prevLen / 2 + 2.5 + 5), 0);
					at = DL_PathAt(placeS);
					at[1] = DL_RoadY(at[0], at[2]);
					vector pd = DL_PathDir(placeS);
					vyaw = Math.Atan2(pd[0], pd[2]) * Math.RAD2DEG;
				}
				if (g_Game.IsKindOf(mv.type, "BoatScript"))
					t = Transport.Cast(g_Game.CreateObjectEx(mv.type, at, ECE_SETUP | ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
				else
					t = Transport.Cast(g_Game.CreateObjectEx(mv.type, at, ECE_PLACE_ON_SURFACE));
			}
			float lift = 0;
			if (t)
			{
				// the game placed it on the surface at its true resting height: every later placement (start, loop,
				// rescue, glide) uses this, so no wheel starts inside the road (a V3S's stiff rear suspension bounces
				// itself over from a few cm of clipping)
				vector tp = t.GetPosition();
				lift = Math.Clamp(tp[1] - DL_RoadY(tp[0], tp[2]), 0, 4);
				t.SetOrientation(Vector(vyaw, 0, 0));
				if (roadFit && !boats)
					dBodyDynamic(t, false);              // held still while the crew sits down (DL_StartEngines lets go)
				vector box[2];
				prevLen = 4.5;
				if (t.GetCollisionBox(box))
				{
					vector lo = box[0];
					vector hi = box[1];
					prevLen = Math.Clamp(hi[2] - lo[2], 3, 14);
				}
				DL_Dress(t, mv.parts);
				m_DL_Made.Insert(t);
				vehCount++;
				DL_Log.Detail("menu scene: " + t.GetType() + " rests " + lift.ToString() + " m above the road");
			}
			else
				DL_Log.Detail("menu scene: no " + mv.type + " here (a mod not loaded at the menu?), its crew stands");
			m_DL_Cars.Insert(t);                    // null keeps the indexes right
			m_DL_CarLift.Insert(lift);
		}
		// walkers (and anyone whose vehicle couldn't be made) in a row ahead of the line, facing the camera; with no
		// vehicles at all on a route, a group on the road that walks it
		int walkers = 0;
		bool walkRoute = roadFit && !boats && vehCount == 0;
		m_DL_MySlot = 0;
		if (walkRoute)
		{
			m_DL_Walkers = new array<PlayerBase>;
			m_DL_WalkLead = Math.Max(m_DL_SpotS, DL_WalkBack(m_DL_Scene.members.Count()) + 5);
			lead = DL_PathAt(m_DL_WalkLead);
			fwd = DL_PathDir(m_DL_WalkLead);
			toCam = Vector(fwd[2], 0, -fwd[0]);
		}
		for (int i = 0; i < m_DL_Scene.members.Count(); i++)
		{
			DL_MenuMember mm = m_DL_Scene.members[i];
			Transport ride = null;
			if (mm.vehicle >= 0 && mm.vehicle < m_DL_Cars.Count())
				ride = m_DL_Cars[mm.vehicle];
			if (i == m_DL_Scene.you)
				continue;                           // the menu's own character, below
			if (!DL_ClassExists(mm.charType))
				continue;
			vector spot = DL_WalkerSpot(lead, fwd, toCam, vehCount, walkers);
			if (walkRoute)
				spot = DL_WalkPlace(walkers, m_DL_WalkLead);
			int slot = walkers;
			if (!ride)
				walkers++;
			PlayerBase pb = PlayerBase.Cast(g_Game.CreateObjectEx(mm.charType, spot, ECE_PLACE_ON_SURFACE));
			if (walkRoute && !ride)
				m_DL_Walkers.Insert(pb);                 // null keeps the slots right
			if (!pb)
				continue;
			m_DL_Made.Insert(pb);
			DL_Dress(pb, mm.worn);
			if (ride && mm.seat >= 0)
				pb.DL_MenuSeat(ride, mm.seat);
			else if (walkRoute)
				pb.SetOrientation(Vector(DL_WalkYaw(slot, m_DL_WalkLead), 0, 0));
			else
				pb.SetOrientation(Vector(Math.Atan2(toCam[0], toCam[2]) * Math.RAD2DEG, 0, 0));
		}
		// the menu's own character: his seat, or his place in the row
		vector mySpot = DL_WalkerSpot(lead, fwd, toCam, vehCount, walkers);
		if (m_DL_Scene.you < 0 || m_DL_Scene.you >= m_DL_Scene.members.Count() || m_DL_Scene.members[m_DL_Scene.you].vehicle < 0)
		{
			if (walkRoute)
			{
				mySpot = DL_WalkPlace(walkers, m_DL_WalkLead);
				m_DL_MySlot = walkers + 1;
				m_DL_Walkers.Insert(null);               // DL_SeatMe puts him in
			}
			walkers++;
		}
		g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(DL_SeatMe, 1000, false, mySpot);
		int lastIdx = vehCount - 1;
		if (lastIdx < 0)
			lastIdx = 0;
		vector last = DL_LinePlace(lead, fwd, toCam, gap, step, lastIdx);
		if (m_DL_SeaRoute)
			last = DL_BoatPlace(lastIdx, m_DL_SpotS);
		if (walkRoute && walkers > 0)
			last = DL_WalkPlace(walkers - 1, m_DL_WalkLead);
		if (vehCount == 0 && !walkRoute)
		{
			// only walkers: side on to the row, as before
			float half = walkers;
			half = half * 0.6;
			vector row = lead + fwd * half;
			row[1] = DL_Ground(row[0], row[2]);
			DL_Aim(row + toCam * (4.5 + walkers * 0.7) + Vector(0, 1.7, 0), row + Vector(0, 1.0, 0));
			return;
		}
		// a three-quarter shot from ahead of the lead vehicle on the drivers' side, looking back down the line: the
		// lead big in front, the others trailing off. The scene spots are full of props: closer until nothing but the
		// party is between the camera and the first and last vehicle; the other side if this one stays blocked.
		float span = vector.Distance(lead, last);
		float ahead = 4.5 + span * 0.12;
		float outward = 6.5 + span * 0.25;
		float camY = 1.4;
		if (boats)
			camY = 1.9;
		vector cam;
		bool clear = false;
		// from the far side of the line (the menu's panels are on the right: seen from here the lead stands left
		// and the line trails off to the right, over the panels)
		for (int sideTry = 0; sideTry < 2 && !clear; sideTry++)
		{
			vector side = toCam * -1;
			if (sideTry == 1)
				side = toCam;
			for (float sc = 1.0; sc >= 0.5; sc -= 0.1)
			{
				cam = lead + fwd * (ahead * sc) + side * (outward * sc);
				cam[1] = DL_Ground(cam[0], cam[2]) + camY;
				if (DL_ClearView(cam, lead) && DL_ClearView(cam, last))
				{
					clear = true;
					break;
				}
			}
		}
		if (!clear)
		{
			cam = lead + fwd * ahead - toCam * outward;
			cam[1] = DL_Ground(cam[0], cam[2]) + camY;
		}
		// look between the lead and the line's middle, so the whole line is in the frame
		vector target = lead + (last - lead) * 0.3;
		target[1] = DL_Ground(target[0], target[2]) + 1.0;
		DL_Log.Detail("menu scene: camera " + cam.ToString() + ", clear view " + clear.ToString());
		DL_Aim(cam, target);
		if (boats && !landBoats)
			DL_StartSailing(lead, fwd, cam, target);
		else if (walkRoute)
			DL_StartWalking(lead, fwd, cam, target);
		else if (roadFit && !boats)
			DL_StartDriving(lead, fwd, gap, vehCount, cam, target);
	}

	protected void DL_StartSailing(vector lead, vector fwd, vector cam, vector target)
	{
		m_DL_SailStart = new array<vector>;
		foreach (Transport t : m_DL_Cars)
		{
			if (t)
				m_DL_SailStart.Insert(t.GetPosition());
		}
		if (m_DL_SailStart.Count() == 0)
			return;
		if (m_DL_SeaRoute)
		{
			float rlen = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
			if (rlen - m_DL_SpotS < DL_RUN_END + 60)
			{
				DL_Log.Detail("menu scene: only " + (rlen - m_DL_SpotS).ToString() + " m of sea route ahead, the boats stay put");
				return;
			}
			m_DL_BoatS = new array<float>;
			for (int bs = 0; bs < m_DL_Cars.Count(); bs++)
				m_DL_BoatS.Insert(Math.Max(m_DL_SpotS - DL_BOAT_GAP * bs, 0));
			DL_CamFrame(lead, fwd, cam, target);
			m_DL_SailAt = g_Game.GetTime() + 2500;
			m_DL_SailLast = 0;
			m_DL_Sailing = true;
			DL_Log.Detail("menu scene: " + Math.Round(rlen - m_DL_SpotS).ToString() + " m of sea route ahead, the boats sail it, then start over");
			return;
		}
		// how far every boat can go before land (10 m steps, at most 400 m)
		m_DL_SailRun = 0;
		for (float run = 10; run <= 400; run += 10)
		{
			bool sea = true;
			foreach (vector st : m_DL_SailStart)
			{
				vector ahead = st + fwd * (run + 15);
				if (!g_Game.SurfaceIsSea(ahead[0], ahead[2]))
					sea = false;
			}
			if (!sea)
				break;
			m_DL_SailRun = run;
		}
		if (m_DL_SailRun < 60)
		{
			DL_Log.Detail("menu scene: only " + m_DL_SailRun.ToString() + " m of open water ahead, the boats stay put");
			return;
		}
		m_DL_SailDir = fwd;
		m_DL_CamOff = cam - lead;
		m_DL_LookOff = target - lead;
		m_DL_Follow = lead;
		m_DL_Sailed = 0;
		m_DL_SailAt = g_Game.GetTime() + 2500;
		m_DL_SailLast = 0;
		m_DL_Sailing = true;
		DL_Log.Detail("menu scene: the boats sail " + m_DL_SailRun.ToString() + " m, then start over");
	}

	override void Update()
	{
		super.Update();
		if (!m_DL_Aimed)
			return;
		if (m_DL_Sailing)
			DL_Sail();
		if (m_DL_Driving)
			DL_Drive();
		if (m_DL_Walking)
			DL_Walk();
		int now = g_Game.GetTime();
		if (now - m_DL_FogAt > 2000)
		{
			m_DL_FogAt = now;
			DL_ThinFog();
		}
	}

	// The boats really sail (like the cars drive): engines on, each boat steers along its own lane of the echelon (from
	// where it started, along the heading) at a point ahead, throttle holds the lead's speed and keeps the others in
	// their place beside and behind it. Propellers turn: wake, spray, engine sound. At the end of the open water all
	// of them start over.
	protected void DL_Sail()
	{
		int now = g_Game.GetTime();
		if (now < m_DL_SailAt || !m_DL_Cars || m_DL_Cars.Count() == 0 || !m_DL_Cars[0])
			return;
		if (!m_DL_BoatsOn)
		{
			foreach (Transport pb : m_DL_Cars)
			{
				if (pb)
					DL_V.Prepare(pb);
			}
			m_DL_BoatsOn = true;
		}
		float dt = 0;
		if (m_DL_SailLast > 0)
			dt = Math.Clamp((now - m_DL_SailLast) / 1000.0, 0, 0.1);
		m_DL_SailLast = now;
		if (m_DL_SeaRoute)
		{
			DL_SailRoute(dt);
			return;
		}
		vector start0 = m_DL_SailStart[0];
		float leadAlong = vector.Dot(m_DL_Cars[0].GetPosition() - start0, m_DL_SailDir);
		if (leadAlong > m_DL_SailRun)
		{
			// back to the start, standing still
			int k = 0;
			foreach (Transport rt : m_DL_Cars)
			{
				if (!rt)
					continue;
				if (k < m_DL_SailStart.Count())
				{
					rt.SetPosition(m_DL_SailStart[k]);
					rt.SetDirection(m_DL_SailDir);
					SetVelocity(rt, Vector(0, 0, 0));
				}
				k++;
			}
			m_DL_Follow = start0;
			DL_Log.Detail("menu scene: boats back at the start");
			return;
		}
		int bi = 0;
		for (int i = 0; i < m_DL_Cars.Count(); i++)
		{
			Transport b = m_DL_Cars[i];
			if (!b)
				continue;
			if (bi >= m_DL_SailStart.Count())
				break;
			vector lane = m_DL_SailStart[bi];
			bi++;
			vector pos = b.GetPosition();
			float along = vector.Dot(pos - lane, m_DL_SailDir);
			vector aim = lane + m_DL_SailDir * (along + 20);
			vector to = aim - pos;
			to[1] = 0;
			to.Normalize();
			vector f = b.GetDirection();
			f[1] = 0;
			f.Normalize();
			float cross = f[0] * to[2] - f[2] * to[0];
			float angle = Math.Atan2(cross, vector.Dot(f, to));
			float steer = Math.Clamp(-angle * 2.0, -1, 1);
			float speed = GetVelocity(b).Length();
			float target = DL_SAIL_SPEED;
			if (bi > 1)
				target = Math.Clamp(DL_SAIL_SPEED + (leadAlong - along) * 0.4, 0, DL_SAIL_SPEED + 3);
			float throttle = Math.Clamp(0.35 + (target - speed) * 0.3, 0, 1);
			DL_V.Drive(b, throttle, steer, 0);
			if (!DL_V.EngineOn(b))
				DL_V.EngineStart(b);
		}
		// the camera follows the lead boat's way, not its bobbing
		vector lp = m_DL_Cars[0].GetPosition();
		float k2 = Math.Min(1, dt * 3);
		m_DL_Follow = m_DL_Follow + (lp - m_DL_Follow) * k2;
		vector fl = m_DL_Follow;
		fl[1] = g_Game.SurfaceGetSeaLevel();
		m_DL_LookAt = fl + m_DL_LookOff;
		m_Camera.SetPosition(fl + m_DL_CamOff);
		m_Camera.LookAt(m_DL_LookAt);
	}

	// boat k of a group on a sea route: DL_BOAT_GAP m back each, out to one side or the other in turn (a channel
	// between islands is too narrow for a whole echelon to one side)
	protected float DL_BoatSide(int k)
	{
		int r = k % 4;
		if (r == 1)
			return DL_BOAT_STEP;
		if (r == 2)
			return -DL_BOAT_STEP;
		return 0;
	}

	protected vector DL_BoatPlace(int k, float leadS)
	{
		float s = Math.Max(leadS - DL_BOAT_GAP * k, 0);
		vector d = DL_PathDir(s);
		vector side = Vector(d[2], 0, -d[0]);
		vector p = DL_PathAt(s) + side * DL_BoatSide(k);
		p[1] = DL_Ground(p[0], p[2]);
		return p;
	}

	// boats on a recorded route: each steers at a point 20 m ahead on its own line, the lead holds the recorded
	// speed (a share of it), the others their place behind it; at the route's end all start over
	protected void DL_SailRoute(float dt)
	{
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		for (int ti = 0; ti < m_DL_Cars.Count(); ti++)
		{
			if (m_DL_Cars[ti])
				m_DL_BoatS[ti] = DL_TrackS(m_DL_Cars[ti].GetPosition(), m_DL_BoatS[ti]);
		}
		float leadS = m_DL_BoatS[0];
		bool restart = leadS > len - DL_RUN_END;
		if (restart)
		{
			float s0 = DL_BOAT_GAP * (m_DL_Cars.Count() - 1) + 20;
			for (int r = 0; r < m_DL_Cars.Count(); r++)
			{
				Transport rt = m_DL_Cars[r];
				if (!rt)
					continue;
				vector rp = DL_BoatPlace(r, s0);
				rt.SetPosition(rp);
				rt.SetDirection(DL_PathDir(Math.Max(s0 - DL_BOAT_GAP * r, 0)));
				SetVelocity(rt, Vector(0, 0, 0));
				m_DL_BoatS[r] = Math.Max(s0 - DL_BOAT_GAP * r, 0);
			}
			m_DL_CamDir = Vector(0, 0, 0);
			leadS = s0;
			DL_Log.Detail("menu scene: boats back at the start of the sea route");
		}
		float want = DL_SAIL_SPEED;
		float rec = DL_PathSpeed(leadS + 20);
		if (rec >= 0)
			want = Math.Clamp(rec * DL_SEA_SPEED_X, 3, 14);
		for (int i = 0; i < m_DL_Cars.Count(); i++)
		{
			Transport b = m_DL_Cars[i];
			if (!b)
				continue;
			float s = m_DL_BoatS[i];
			vector pos = b.GetPosition();
			// lost (ran aground, pushed off by another boat): back in its place
			vector mine = DL_BoatPlace(i, s + DL_BOAT_GAP * i);
			float off = vector.Distance(Vector(pos[0], 0, pos[2]), Vector(mine[0], 0, mine[2]));
			float behind = (leadS - DL_BOAT_GAP * i) - s;
			if (i > 0 && (off > 25 || behind > 60))
			{
				vector back = DL_BoatPlace(i, leadS);
				b.SetPosition(back);
				b.SetDirection(DL_PathDir(Math.Max(leadS - DL_BOAT_GAP * i, 0)));
				SetVelocity(b, GetVelocity(m_DL_Cars[0]));
				m_DL_BoatS[i] = Math.Max(leadS - DL_BOAT_GAP * i, 0);
				DL_Log.Detail("menu scene: boat " + i.ToString() + " back in its place (" + Math.Round(off).ToString() + " m off its line)");
				continue;
			}
			vector aim = DL_BoatPlace(i, Math.Min(s + DL_BOAT_GAP * i + 20, len));
			vector to = aim - pos;
			to[1] = 0;
			to.Normalize();
			vector f = b.GetDirection();
			f[1] = 0;
			f.Normalize();
			float cross = f[0] * to[2] - f[2] * to[0];
			float angle = Math.Atan2(cross, vector.Dot(f, to));
			float steer = Math.Clamp(-angle * 2.0, -1, 1);
			float speed = GetVelocity(b).Length();
			float target = want;
			if (i > 0)
				target = Math.Clamp(want + behind * 0.4, 0, want + 3);
			float throttle = Math.Clamp(0.35 + (target - speed) * 0.3, 0, 1);
			DL_V.Drive(b, throttle, steer, 0);
			if (!DL_V.EngineOn(b))
				DL_V.EngineStart(b);
		}
		if (!m_DL_Cars[0])
			return;
		vector lpos = m_DL_Cars[0].GetPosition();
		lpos[1] = g_Game.SurfaceGetSeaLevel();       // its way, not its bobbing
		vector lvel = GetVelocity(m_DL_Cars[0]);
		lvel[1] = 0;
		DL_FollowCam(lpos, lvel, leadS, dt, restart);
	}

	// vehicle k of the line: gap m back each, and (boats) step m out each, away from the camera's side
	protected vector DL_LinePlace(vector lead, vector fwd, vector toCam, float gap, float step, int k)
	{
		float back = gap * k;
		float wide = step * k;
		vector at = lead - fwd * back + toCam * wide;
		at[1] = DL_Ground(at[0], at[2]);
		return at;
	}

	protected void DL_Aim(vector cam, vector target)
	{
		m_Camera.SetPosition(cam);
		m_DL_LookAt = target;
		m_DL_Aimed = true;
		m_Camera.LookAt(m_DL_LookAt);
	}

	// ---------------- cars under way ----------------

	// the road's height (a road object can stand a little above the terrain)
	protected float DL_RoadY(float x, float z)
	{
		return Math.Max(g_Game.SurfaceY(x, z), g_Game.SurfaceRoadY(x, z, RoadSurfaceDetection.CLOSEST));
	}

	// the path's point at s m (between its points)
	protected vector DL_PathAt(float s)
	{
		int last = m_DL_Path.Count() - 1;
		float f = s / DL_PATH_STEP;
		int i = Math.Floor(f);
		if (i < 0)
			return m_DL_Path[0];
		if (i >= last)
			return m_DL_Path[last];
		float t = f - i;
		return m_DL_Path[i] + (m_DL_Path[i + 1] - m_DL_Path[i]) * t;
	}

	// the route as points every DL_PATH_STEP m, on the road's height
	// (speeds: one per route point, m/s as recorded; empty = the speed comes from the road's bends)
	protected bool DL_MakePath(array<vector> route, array<float> speeds)
	{
		m_DL_Path = new array<vector>;
		m_DL_PathV = null;
		bool withV = speeds && speeds.Count() == route.Count();
		if (withV)
			m_DL_PathV = new array<float>;
		// walk the route, a point every DL_PATH_STEP m of it (s along the path is then real metres)
		float carry = 0;                           // m into the current segment of the next point
		for (int i = 0; i + 1 < route.Count(); i++)
		{
			vector a = route[i];
			vector b = route[i + 1];
			float segLen = vector.Distance(a, b);
			if (segLen < 0.01)
				continue;
			float d = carry;
			while (d < segLen)
			{
				float t = d / segLen;
				vector q = a + (b - a) * t;
				q[1] = DL_RoadY(q[0], q[2]);
				m_DL_Path.Insert(q);
				if (withV)
					m_DL_PathV.Insert(speeds[i] + (speeds[i + 1] - speeds[i]) * t);
				d += DL_PATH_STEP;
			}
			carry = d - segLen;
		}
		vector end = route[route.Count() - 1];
		end[1] = DL_RoadY(end[0], end[2]);
		m_DL_Path.Insert(end);
		if (withV)
			m_DL_PathV.Insert(speeds[speeds.Count() - 1]);
		return m_DL_Path.Count() >= 2;
	}

	// the recorded speed at s (m/s), -1 = none recorded
	protected float DL_PathSpeed(float s)
	{
		if (!m_DL_PathV || m_DL_PathV.Count() == 0)
			return -1;
		int i = s / DL_PATH_STEP;
		if (i < 0)
			i = 0;
		if (i > m_DL_PathV.Count() - 1)
			i = m_DL_PathV.Count() - 1;
		return m_DL_PathV[i];
	}

	// the distance along the path of its point nearest to p
	protected float DL_PathS(vector p)
	{
		int best = 0;
		float bestD = 1000000000.0;
		for (int i = 0; i < m_DL_Path.Count(); i++)
		{
			float d = vector.DistanceSq(Vector(p[0], 0, p[2]), Vector(m_DL_Path[i][0], 0, m_DL_Path[i][2]));
			if (d < bestD)
			{
				bestD = d;
				best = i;
			}
		}
		return best * DL_PATH_STEP;
	}

	// how steeply the road rises over the next 12 m from s (0.1 = 10 %, < 0 downhill)
	protected float DL_Grade(float s)
	{
		vector a = DL_PathAt(s);
		vector b = DL_PathAt(s + 12);
		return (b[1] - a[1]) / 12;
	}

	// the path's level direction at s
	protected vector DL_PathDir(float s)
	{
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		vector d = DL_PathAt(Math.Min(s + 6, len)) - DL_PathAt(Math.Max(s - 6, 0));
		d[1] = 0;
		d.Normalize();
		return d;
	}

	// The cars really drive (an idea from Triumph's Moving AI Convoy: engine, throttle and steering, no teleporting):
	// each one steers at a point ahead on the path, throttle / brake hold a speed (slower before bends), the others keep
	// their gap to the car ahead, gears go up and down on the revs. Wheels turn, suspension works, the engines run.
	protected void DL_StartDriving(vector lead, vector fwd, float gap, int vehCount, vector cam, vector target)
	{
		if (vehCount < 1 || !m_DL_Path)
			return;
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		if (len - m_DL_SpotS < DL_RUN_END + 60)
		{
			DL_Log.Detail("menu scene: only " + (len - m_DL_SpotS).ToString() + " m of route ahead, the cars stay put");
			return;                                  // parked where they were made
		}
		m_DL_Gap = gap;
		DL_MeasureGaps();
		m_DL_CarS = new array<float>;
		m_DL_ShiftAt = new array<int>;
		m_DL_StuckFor = new array<float>;
		// every vehicle onto the road at its place behind the lead (the straight line from the spot can leave the
		// road, and a truck needs more room than a hatchback)
		for (int ci = 0; ci < m_DL_Cars.Count(); ci++)
		{
			float cs = m_DL_SpotS;
			if (m_DL_Cars[ci])
				cs = DL_PathS(m_DL_Cars[ci].GetPosition());   // made on the road already (DL_BuildRoad)
			m_DL_CarS.Insert(cs);
			m_DL_ShiftAt.Insert(0);
			m_DL_StuckFor.Insert(0);
		}
		DL_CamFrame(DL_PathAt(m_DL_SpotS), fwd, cam, target);
		m_DL_DriveAt = g_Game.GetTime() + 2500;    // the crews sit down, the cars settle on their wheels
		m_DL_DriveLast = 0;
		m_DL_EnginesOn = false;
		m_DL_Driving = true;
		DL_Log.Detail("menu scene: " + Math.Round(len - m_DL_SpotS).ToString() + " m of route ahead, the cars drive it, then start over");
	}

	// the camera's and the look target's place relative to the lead (lp), in the road's frame
	protected void DL_CamFrame(vector lp, vector fwd, vector cam, vector target)
	{
		vector side = Vector(fwd[2], 0, -fwd[0]);
		vector dc = cam - lp;
		vector dl = target - lp;
		m_DL_CamAlong = vector.Dot(dc, fwd);
		m_DL_CamSide = vector.Dot(dc, side);
		m_DL_CamUp = cam[1] - Math.Max(DL_RoadY(cam[0], cam[2]), DL_Ground(cam[0], cam[2]));
		m_DL_LookAlong = vector.Dot(dl, fwd);
		m_DL_LookSide = vector.Dot(dl, side);
		m_DL_CamDir = fwd;
		m_DL_Follow = lp;
	}

	// each vehicle's room: half its length, half the length of the one ahead, and 5 m of road between them
	protected void DL_MeasureGaps()
	{
		m_DL_GapOf = new array<float>;
		m_DL_Back = new array<float>;
		float prevLen = -1;
		float back = 0;
		foreach (Transport t : m_DL_Cars)
		{
			float l = 4.5;
			vector box[2];
			if (t && t.GetCollisionBox(box))
			{
				vector lo = box[0];
				vector hi = box[1];
				l = Math.Clamp(hi[2] - lo[2], 3, 14);
			}
			float g = 0;
			if (t && prevLen >= 0)
				g = prevLen / 2 + l / 2 + 5;
			back += g;
			m_DL_GapOf.Insert(g);
			m_DL_Back.Insert(back);
			if (t)
				prevLen = l;
		}
		DL_Log.Detail("menu scene: convoy " + Math.Round(back).ToString() + " m long");
	}


	// set down on their wheels (the stable rescue placement), physics on, standing still; engines on, first gear,
	// brakes off
	protected void DL_StartEngines()
	{
		for (int pi = 0; pi < m_DL_Cars.Count(); pi++)
		{
			if (!m_DL_Cars[pi])
				continue;
			dBodyDynamic(m_DL_Cars[pi], true);
			DL_PutOnPath(pi, m_DL_CarS[pi]);
		}
		foreach (Transport t : m_DL_Cars)
		{
			if (!t || !(Car.Cast(t) || Motorbike.Cast(t)))
				continue;
			DL_V.Prepare(t);
			DL_Log.Detail("menu scene: " + t.GetType() + " engine " + DL_V.EngineOn(t).ToString() + ", manual " + DL_V.Manual(t).ToString() + ", gear " + DL_V.Gear(t).ToString());
		}
		m_DL_EnginesOn = true;
		m_DL_HasTruck = false;
		foreach (Transport tt : m_DL_Cars)
		{
			if (tt && (g_Game.IsKindOf(tt.GetType(), "Truck_01_Base") || (dBodyGetMass(tt) > 3000 && DL_V.Manual(tt))))
				m_DL_HasTruck = true;
		}
		DL_Log.Detail("menu scene: truck in the convoy " + m_DL_HasTruck.ToString() + " (the recorded speeds, else at least the road's pace)");
	}

	// the path's nearest point to p, looking near the last one (cars only move forward along it)
	protected float DL_TrackS(vector p, float last)
	{
		int from = last / DL_PATH_STEP;
		from = from - 4;
		if (from < 0)
			from = 0;
		int to = from + 16;
		if (to > m_DL_Path.Count() - 1)
			to = m_DL_Path.Count() - 1;
		int best = from;
		float bestD = 1000000000.0;
		for (int i = from; i <= to; i++)
		{
			vector q = m_DL_Path[i];
			float d = (p[0] - q[0]) * (p[0] - q[0]) + (p[2] - q[2]) * (p[2] - q[2]);
			if (d < bestD)
			{
				bestD = d;
				best = i;
			}
		}
		return best * DL_PATH_STEP;
	}

	// Down a gear like an automatic gearbox: only once it has slowed well below the speed it left the lower gear at
	// (a motorbike's top gear sits at ~30 % revs at cruising speed; shifting down there only cost it speed). Without
	// that speed known (just started): low revs while losing way on the pedal. Always when the revs nearly stall.
	protected bool DL_ShouldShiftDown(int i, int gear, float speed, float rev, float asked, float accel, bool wasUp, int since)
	{
		if (rev < 0.12)
			return true;
		if (wasUp && since < 2500)
			return false;                            // just shifted up: the clutch costs a little speed, let it pull
		int key = i * 100 + gear - 1;
		if (m_DL_UpSpeed.Contains(key))
			return speed < m_DL_UpSpeed.Get(key) * 0.75;
		return rev < 0.35 && asked > 0.6 && accel < 0.3 && (!wasUp || since > 2500);
	}

	// a car put back on the road twice: it can't keep up under its own power, it glides from now on
	protected void DL_Rescued(int i)
	{
		int n = m_DL_Rescues.Get(i) + 1;
		m_DL_Rescues.Set(i, n);
		if (n < 2 || m_DL_Glide.Get(i) || !m_DL_Cars[i])
			return;
		m_DL_Glide.Set(i, true);
		m_DL_GlideAt.Set(i, g_Game.GetTime() + 600);   // half a second braked with the wheels straight, then frozen
		DL_Log.Detail("menu scene: " + m_DL_Cars[i].GetType() + " can't keep up on its own, it glides along with the others");
	}

	// at pos, facing dir (with its slope), upright: the whole transform, so a car that rolled over comes back on its wheels
	protected void DL_SetUpright(Transport t, vector pos, vector dir)
	{
		vector m[4];
		Math3D.DirectionAndUpMatrix(dir, "0 1 0", m);
		m[3] = pos;
		t.SetTransform(m);
	}

	// tipped past ~60 degrees (rolled, on its side, on its roof)
	protected bool DL_Tipped(Transport t)
	{
		vector m[4];
		t.GetTransform(m);
		vector up = m[1];
		return up[1] < 0.5;
	}

	// every car back on the path (the loop's start, or one that ran off): position, heading, standing still
	protected void DL_PutOnPath(int i, float s)
	{
		Transport t = m_DL_Cars[i];
		if (!t)
			return;
		vector at = DL_PathAt(s);
		at[1] = DL_RoadY(at[0], at[2]) + 0.1;     // set down on its wheels (resting height below)
		if (m_DL_CarLift && i < m_DL_CarLift.Count())
			at[1] = at[1] + m_DL_CarLift[i];
		DL_SetUpright(t, at, DL_PathDir(s));
		SetVelocity(t, Vector(0, 0, 0));
		dBodyActive(t, ActiveState.ACTIVE);
		m_DL_CarS[i] = s;
	}

	protected void DL_Drive()
	{
		int now = g_Game.GetTime();
		if (now < m_DL_DriveAt || !m_DL_Path)
			return;
		if (!m_DL_EnginesOn)
			DL_StartEngines();
		float dt = 0;
		if (m_DL_DriveLast > 0)
			dt = Math.Clamp((now - m_DL_DriveLast) / 1000.0, 0, 0.1);
		m_DL_DriveLast = now;
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		// the lead's speed: slower before a bend (how much the road turns in the next 30 m)
		float leadS = m_DL_CarS[0];
		vector d0 = DL_PathDir(leadS);
		vector d1 = DL_PathDir(Math.Min(leadS + 30, len));
		float bend = Math.Acos(Math.Clamp(vector.Dot(d0, d1), -1, 1));
		float want = DL_DRIVE_SPEED;
		if (bend > 0.35)
			want = DL_DRIVE_SPEED * 0.55;
		else if (bend > 0.18)
			want = DL_DRIVE_SPEED * 0.75;
		float rec = DL_PathSpeed(leadS + 15);       // a recorded route: the speed driven there, a little ahead
		// The route was recorded in a V3S, which crawls up a steep hill: with a truck in the convoy that is the pace for
		// all; without one they keep at least the road's own pace (slower before bends) and take the hill with some way
		// on (a Humvee at the truck's 10 km/h stalled on a 27 % climb)
		if (rec >= 0)
		{
			float recWant = Math.Max(rec * DL_REC_SPEED_X, 3);
			if (m_DL_HasTruck)
				want = recWant;
			else
				want = Math.Max(recWant, want);
		}
		// the convoy waits for its slowest: the more any vehicle has dropped back behind the one ahead of it, the
		// slower the lead goes (a loaded truck can't hold a hatchback's pace uphill)
		float lag = 0;
		float prevS = -1;
		for (int li = 0; li < m_DL_Cars.Count(); li++)
		{
			if (!m_DL_Cars[li] || m_DL_Glide.Get(li))
				continue;
			if (prevS >= 0)
				lag = Math.Max(lag, (prevS - m_DL_CarS[li]) - m_DL_GapOf[li]);
			prevS = m_DL_CarS[li];
		}
		if (lag > 2)
			want = Math.Max(want - (lag - 2) * 0.5, 2);
		m_DL_LeadSpeed = m_DL_LeadSpeed + (want - m_DL_LeadSpeed) * Math.Min(1, dt * 0.8);
		bool restart = false;
		float aheadS = -1;
		for (int i = 0; i < m_DL_Cars.Count(); i++)
		{
			Transport c = m_DL_Cars[i];
			if (!c || !(Car.Cast(c) || Motorbike.Cast(c)))
				continue;
			if (m_DL_Glide.Get(i) && m_DL_GlideAt.Get(i) > now)
			{
				// about to glide: wheels straight first (a frozen car runs no input step, they'd stay turned)
				DL_V.Drive(c, 0, 0, 1);
				aheadS = m_DL_CarS[i];
				continue;
			}
			if (m_DL_Glide.Get(i))
			{
				if (dBodyIsDynamic(c))
					dBodyDynamic(c, false);
				// kept in line behind the car ahead, along the road, physics off
				float gs = m_DL_CarS[i] + m_DL_LeadSpeed * dt;
				if (aheadS >= 0)
					gs = Math.Min(gs + (aheadS - m_DL_GapOf[i] - gs) * Math.Min(1, dt), aheadS - m_DL_GapOf[i] * 0.8);
				m_DL_CarS[i] = gs;
				vector gat = DL_PathAt(gs);
				gat[1] = DL_RoadY(gat[0], gat[2]);
				if (m_DL_CarLift && i < m_DL_CarLift.Count())
					gat[1] = gat[1] + m_DL_CarLift[i];
				vector glook = DL_PathAt(Math.Min(gs + 2.5, len)) - DL_PathAt(Math.Max(gs - 2.5, 0));
				glook.Normalize();
				DL_SetUpright(c, gat, glook);
				DL_V.Drive(c, 0, 0, 0);                  // front wheels straight (they kept the last turn they had)
				aheadS = gs;
				continue;
			}
			vector pos = c.GetPosition();
			float s = DL_TrackS(pos, m_DL_CarS[i]);
			m_DL_CarS[i] = s;
			// its ride height as it drives, settled on its springs (made a gliding car float less: the height at spawn
			// was before the suspension took the weight)
			if (m_DL_CarLift && i < m_DL_CarLift.Count() && !DL_Tipped(c))
			{
				float rideH = Math.Clamp(pos[1] - DL_RoadY(pos[0], pos[2]), 0, 4);
				m_DL_CarLift[i] = m_DL_CarLift[i] + (rideH - m_DL_CarLift[i]) * Math.Min(1, dt * 0.5);
			}
			vector onPath = DL_PathAt(s);
			float off = Math.Sqrt((pos[0] - onPath[0]) * (pos[0] - onPath[0]) + (pos[2] - onPath[2]) * (pos[2] - onPath[2]));
			if (DL_Tipped(c))
			{
				DL_Log.Detail("menu scene: " + c.GetType() + " rolled over at " + Math.Round(s).ToString() + " m, back on its wheels");
				DL_PutOnPath(i, s);
				DL_Rescued(i);
				continue;
			}
			if (off > 8 || pos[1] < DL_RoadY(pos[0], pos[2]) - 1.5)
			{
				DL_Log.Detail("menu scene: " + c.GetType() + " left the road (" + Math.Round(off).ToString() + " m), back on it");
				DL_PutOnPath(i, s);
				continue;
			}
			if (i == 0 && s > len - DL_RUN_END)
				restart = true;
			// stuck on something (a barrier, a tree): throttle but no way; back on the road a little further on
			if (m_DL_Asked.Get(i) > 0.4 && DL_V.Speed(c) < 2)
				m_DL_StuckFor[i] = m_DL_StuckFor[i] + dt;
			else
				m_DL_StuckFor[i] = 0;
			if (m_DL_StuckFor[i] > 3)
			{
				DL_Log.Detail("menu scene: " + c.GetType() + " stuck at " + Math.Round(s).ToString() + " m (grade " + Math.Round(DL_Grade(s) * 100).ToString() + "%, gear " + DL_V.Gear(c).ToString() + "), back on the road further on");
				m_DL_StuckFor[i] = 0;
				// a little further on, but never onto the vehicle ahead (a Gunter put 8 m on landed on a motorbike)
				float hop = Math.Min(s + 8, len - DL_RUN_END);
				if (aheadS >= 0)
					hop = Math.Max(Math.Min(hop, aheadS - m_DL_GapOf[i]), s);
				DL_PutOnPath(i, hop);
				DL_Rescued(i);
				continue;
			}
			// steering: at a point ahead on the path, further ahead the faster it goes
			float speed = DL_V.Speed(c) / 3.6;
			vector aim = DL_PathAt(Math.Min(s + 7 + speed * 0.9, len));
			vector to = aim - pos;
			to[1] = 0;
			to.Normalize();
			vector f = c.GetDirection();
			f[1] = 0;
			f.Normalize();
			float cross = f[0] * to[2] - f[2] * to[0];           // > 0: the aim is to the left
			float angle = Math.Atan2(cross, vector.Dot(f, to));
			float steer = Math.Clamp(-angle * 2.2 / (1 + speed / 10), -1, 1);   // gentler the faster (a truck rolls)
			// fell far behind (a weak engine, a bump): back in line behind the car ahead
			if (aheadS >= 0 && aheadS - s > m_DL_GapOf[i] * 4 && aheadS - m_DL_GapOf[i] > s)
			{
				DL_Log.Detail("menu scene: " + c.GetType() + " fell " + Math.Round(aheadS - s).ToString() + " m behind, back in line");
				DL_PutOnPath(i, aheadS - m_DL_GapOf[i]);
				aheadS = aheadS - m_DL_GapOf[i];
				DL_Rescued(i);
				continue;
			}
			// speed: the lead its target, the others close up to their gap behind the car ahead
			float target = m_DL_LeadSpeed;
			if (aheadS >= 0)
				target = Math.Clamp(m_DL_LeadSpeed + ((aheadS - s) - m_DL_GapOf[i]) * 0.4, 0, m_DL_LeadSpeed + 3);
			aheadS = s;
			float err = target - speed;
			float brake = 0;
			if (err < -1.5)
				brake = Math.Clamp(-err * 0.25, 0, 1);
			// a climb takes more pedal for the same speed (a steep Livonia hill stopped a 4x4 in second gear on the
			// speed gap alone): some throttle for the slope itself, on top
			float grade = DL_Grade(s);
			float throttle = Math.Clamp(err * 0.35 + Math.Max(grade, 0) * 4, 0, 1);
			// a gentle foot: the pedal only rises gradually, and a heavy vehicle gets at most 60 % in first gear
			// (full throttle from standstill made a V3S hop on its stiff rear suspension until it rolled over)
			throttle = Math.Min(throttle, m_DL_Asked.Get(i) + dt * 0.6);
			// (a manual's first gear only: an automatic's gear numbers mean something else, and a 3.5 t Humvee held to
			// 60 % stalled on a 20 % Livonia climb)
			if (dBodyGetMass(c) > 3000 && DL_V.Manual(c) && DL_V.Gear(c) <= DL_V.NeutralGear(c) + 1)
				throttle = Math.Min(throttle, 0.6);
			m_DL_Asked.Set(i, throttle);
			DL_V.Drive(c, throttle, steer, brake);
			// a slope steeper than a weak engine can take (Livonia: up to 27 %; a Gunter, then a Humvee stalled there): an
			// unseen push up the road takes the slope away, so it climbs on its own wheels as on the flat
			if (grade > 0.05 && throttle > 0.5 && speed < 6)
			{
				vector up = DL_PathAt(s + 6) - DL_PathAt(s);
				up.Normalize();
				dBodyApplyForce(c, up * (dBodyGetMass(c) * 9.81 * grade));
			}
			// a manual gearbox, by each engine's own rev range (idle .. redline; a truck's is far lower than a car's):
			// up high in the range, down low in it when pulling hard (a hill after a bend) or when the revs sag,
			// first gear when nearly stopped
			if (DL_V.Manual(c) && now - m_DL_ShiftAt[i] > 800)
			{
				float rpm = DL_V.Rpm(c);
				float idle = DL_V.RpmIdle(c);
				float range = Math.Max(DL_V.RpmRedline(c) - idle, 1);
				float rev = (rpm - idle) / range;
				int gear = DL_V.Gear(c);
				int first = DL_V.NeutralGear(c) + 1;
				float asked = m_DL_Asked.Get(i);
				// is it still gaining speed? low revs while speeding up is fine; low revs and losing way on the pedal is a
				// hill (a close-ratio top gear sits low in its revs for a while and would hunt down and up)
				if (now - m_DL_AccelAt.Get(i) > 1000)
				{
					float lastV = m_DL_LastV.Get(i);
					m_DL_Accel.Set(i, (speed - lastV) * 1000 / Math.Max(now - m_DL_AccelAt.Get(i), 1));
					m_DL_LastV.Set(i, speed);
					m_DL_AccelAt.Set(i, now);
				}
				float accel = m_DL_Accel.Get(i);
				// hysteresis: an upshift drops the revs, so no downshift for a while unless they really sag (and the
				// other way round), else a close-ratio gearbox (a motorbike) hunts up and down
				int since = now - m_DL_ShiftAt[i];
				bool wasUp = m_DL_ShiftUp.Get(i);
				string shifted = "";
				// on a climb the next gear up would sag below the hill downshift at once (a V3S went 2-3-2-3): only near
				// the redline there
				float upAt = 0.8;
				if (grade > 0.08)
					upAt = 0.93;
				if (gear < first || (speed < 3 && gear > first + 1) || (speed < 3 && grade > 0.08 && gear > first))
				{
					DL_V.ShiftTo(c, first);
					shifted = "to first";
				}
				else if (gear > first && grade > 0.08 && rev < 0.45 && asked > 0.6 && since > 1500)
				{
					// a hill: down before the revs sag into a stall, not after
					DL_V.ShiftDown(c);
					shifted = "down for the hill";
					m_DL_ShiftUp.Set(i, false);
				}
				else if (rev > upAt && gear < DL_V.GearCount(c) - 1 && (wasUp || since > 1500))
				{
					DL_V.ShiftUp(c);
					shifted = "up";
					m_DL_ShiftUp.Set(i, true);
					m_DL_UpSpeed.Set(i * 100 + gear, speed);     // left this gear at this speed
				}
				else if (gear > first && DL_ShouldShiftDown(i, gear, speed, rev, asked, accel, wasUp, since))
				{
					DL_V.ShiftDown(c);
					shifted = "down";
					m_DL_ShiftUp.Set(i, false);
				}
				if (shifted != "")
				{
					m_DL_ShiftAt[i] = now;
					string gl = "menu scene gear: " + c.GetType() + " " + shifted + " from " + gear.ToString();
					gl += " of " + DL_V.GearCount(c).ToString() + ", rev " + (Math.Round(rev * 100)).ToString() + "%";
					gl += ", " + Math.Round(speed * 3.6).ToString() + " km/h";
					DL_Log.Detail(gl);
				}
			}
			if (!DL_V.EngineOn(c))
				DL_V.EngineStart(c);
			// a heavy vehicle's stiff, lightly damped springs (a V3S) turn every bump into bucking that grows until it
			// rolls: take some of its pitch and roll rotation away every frame, like a stronger shock absorber.
			// Turning (yaw) is left alone; hills change its pitch slowly enough to pass.
			if (dBodyGetMass(c) > 3000)
			{
				vector av = dBodyGetAngularVelocity(c);
				float keep = Math.Pow(0.85, dt * 60);
				dBodySetAngularVelocity(c, Vector(av[0] * keep, av[1], av[2] * keep));
			}
		}
		if (restart)
		{
			// the whole line back to the route's start, the lead a little way in
			float s0 = m_DL_Back[m_DL_Back.Count() - 1] + 20;
			for (int r = 0; r < m_DL_Cars.Count(); r++)
			{
				if (m_DL_Cars[r])
					DL_PutOnPath(r, s0 - m_DL_Back[r]);
			}
			m_DL_CamDir = Vector(0, 0, 0);
			m_DL_LeadSpeed = 0;
			DL_Log.Detail("menu scene: cars back at the start");
		}
		// the camera rides along: its place beside and ahead of the lead car, in the road's frame
		if (!m_DL_Cars[0])
			return;
		DL_FollowCam(m_DL_Cars[0].GetPosition(), GetVelocity(m_DL_Cars[0]), m_DL_CarS[0], dt, restart);
	}

	// the camera at its place beside and ahead of the lead (at lpos, leadS along the path), turning with the road
	// (vel: the lead's velocity, m/s. The smoothing alone trailed a moving car by a quarter second of its way, 2-4 m:
	// aimed that far ahead it settles right on the car and only the bumps are smoothed away.)
	protected void DL_FollowCam(vector lpos, vector vel, float leadS, float dt, bool restart)
	{
		vector want = lpos + vel * 0.25;
		m_DL_Follow = m_DL_Follow + (want - m_DL_Follow) * Math.Min(1, dt * 4);
		if (restart)
			m_DL_Follow = lpos;
		vector rd = DL_PathDir(leadS);
		if (m_DL_CamDir.Length() < 0.5)
			m_DL_CamDir = rd;
		m_DL_CamDir = m_DL_CamDir + (rd - m_DL_CamDir) * Math.Min(1, dt * 1.2);
		m_DL_CamDir.Normalize();
		vector cside = Vector(m_DL_CamDir[2], 0, -m_DL_CamDir[0]);
		vector lp = m_DL_Follow;
		vector cam = lp + m_DL_CamDir * m_DL_CamAlong + cside * m_DL_CamSide;
		cam[1] = Math.Max(Math.Max(DL_RoadY(cam[0], cam[2]), DL_Ground(cam[0], cam[2])), lp[1]) + m_DL_CamUp;
		vector tgt = lp + m_DL_CamDir * m_DL_LookAlong + cside * m_DL_LookSide;
		tgt[1] = lp[1] + 1.0;
		// something between the lead and the camera (a tree trunk, a rock): the camera comes in in front of it
		vector from = lp + Vector(0, 1.2, 0);
		float camWant = 0;
		vector hit;
		vector hitDir;
		int hitComp;
		set<Object> hits = new set<Object>;
		if (DayZPhysics.RaycastRV(from, cam, hit, hitDir, hitComp, hits, null, null, false, false, ObjIntersectView, 0.3))
		{
			// only the world counts (trees, rocks, walls): any vehicle, its parts or a person is the party
			bool ours = hits.Count() > 0;
			for (int h = 0; h < hits.Count(); h++)
			{
				EntityAI he = EntityAI.Cast(hits.Get(h));
				if (!he || !(Transport.Cast(he.GetHierarchyRoot()) || Man.Cast(he.GetHierarchyRoot())))
					ours = false;
			}
			if (!ours)
				camWant = Math.Clamp(1 - (vector.Distance(from, hit) - 1) / vector.Distance(from, cam), 0, 0.85);
		}
		// in quickly, out slowly
		if (camWant > m_DL_CamPull)
			m_DL_CamPull = camWant;
		else
			m_DL_CamPull = m_DL_CamPull + (camWant - m_DL_CamPull) * Math.Min(1, dt * 0.7);
		cam = cam + (from - cam) * m_DL_CamPull;
		m_DL_LookAt = tgt;
		m_Camera.SetPosition(cam);
		m_Camera.LookAt(tgt);
	}

	// the ground, or the sea's surface over the sea
	protected float DL_Ground(float x, float z)
	{
		float y = g_Game.SurfaceY(x, z);
		if (g_Game.SurfaceIsSea(x, z))
			y = Math.Max(y, g_Game.SurfaceGetSeaLevel());
		return y;
	}

	// nothing but our own vehicles / people (and the menu character) on the line from the camera to pos
	protected bool DL_ClearView(vector from, vector pos)
	{
		vector to = pos + Vector(0, 1.2, 0);
		vector cp;
		vector cd;
		int cc;
		set<Object> hits = new set<Object>;
		if (!DayZPhysics.RaycastRV(from, to, cp, cd, cc, hits, null, null, false, false, ObjIntersectView, 0.2))
			return true;
		if (hits.Count() == 0)
			return vector.Distance(cp, to) < 2;      // the ground, but only right at the party
		PlayerBase me;
		if (m_Character)
			me = m_Character.GetCharacterObj();
		for (int i = 0; i < hits.Count(); i++)
		{
			Object o = hits.Get(i);
			if (!o || o == me || m_DL_Made.Find(o) >= 0)
				continue;
			EntityAI e = EntityAI.Cast(o);
			if (e && e.GetHierarchyRoot() && m_DL_Made.Find(e.GetHierarchyRoot()) >= 0)
				continue;                       // a part on one of our vehicles
			return false;
		}
		return true;
	}

	// the k-th walker: beside the lead vehicle's front, or (no vehicles) in a row from where the character stood
	protected vector DL_WalkerSpot(vector lead, vector fwd, vector toCam, int vehCount, int k)
	{
		vector p;
		float along = k;
		along = along * 1.2;
		if (vehCount > 0)
			p = lead + fwd * (3.5 + along) + toCam * 1.5;
		else
			p = lead + fwd * along;
		p[1] = DL_Ground(p[0], p[2]);
		return p;
	}

	protected void DL_SeatMe(vector spot)
	{
		PlayerBase me;
		if (m_Character)
			me = m_Character.GetCharacterObj();
		if (!me || !m_DL_Scene || !m_DL_Cars)
			return;
		int you = m_DL_Scene.you;
		if (you >= 0 && you < m_DL_Scene.members.Count())
		{
			DL_MenuMember mm = m_DL_Scene.members[you];
			if (mm.vehicle >= 0 && mm.vehicle < m_DL_Cars.Count() && m_DL_Cars[mm.vehicle] && mm.seat >= 0)
			{
				me.DL_MenuSeat(m_DL_Cars[mm.vehicle], mm.seat);
				return;
			}
		}
		if (m_DL_Scene.members.Count() > 1 || m_DL_MySlot > 0)
			me.SetPosition(spot);
		int mine = m_DL_MySlot - 1;
		if (m_DL_Walkers && mine >= 0 && mine < m_DL_Walkers.Count())
		{
			m_DL_Walkers[mine] = me;
			me.SetOrientation(Vector(DL_WalkYaw(mine, m_DL_WalkLead), 0, 0));
		}
	}

	// ---------------- people on foot ----------------

	// the k-th walker's place in the group: m behind its front, and out to the side (two abreast, the first alone
	// in front)
	protected float DL_WalkBack(int k)
	{
		int row = (k + 1) / 2;
		return row * DL_WALK_ROW;
	}

	protected float DL_WalkSide(int k)
	{
		if (k == 0)
			return 0;
		if (k % 2 == 1)
			return -DL_WALK_SIDE;
		return DL_WALK_SIDE;
	}

	protected vector DL_WalkPlace(int k, float front)
	{
		float s = Math.Max(front - DL_WalkBack(k), 0);
		vector d = DL_PathDir(s);
		vector side = Vector(d[2], 0, -d[0]);
		vector p = DL_PathAt(s) + side * DL_WalkSide(k);
		p[1] = DL_Ground(p[0], p[2]);
		return p;
	}

	protected float DL_WalkYaw(int k, float front)
	{
		vector d = DL_PathDir(Math.Max(front - DL_WalkBack(k), 0));
		return Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
	}

	// the distance along the path of p, between the path's points (a walker moves ~1.5 m/s: the 4 m points alone
	// would make the camera step), looking near the last one
	protected float DL_TrackFine(vector p, float last)
	{
		int i = DL_TrackS(p, last) / DL_PATH_STEP;
		float best = i * DL_PATH_STEP;
		float bestD = 1000000000.0;
		for (int j = i - 1; j <= i; j++)
		{
			if (j < 0 || j + 1 >= m_DL_Path.Count())
				continue;
			vector a = m_DL_Path[j];
			vector b = m_DL_Path[j + 1];
			float ax = b[0] - a[0];
			float az = b[2] - a[2];
			float l2 = ax * ax + az * az;
			if (l2 < 0.0001)
				continue;
			float t = Math.Clamp(((p[0] - a[0]) * ax + (p[2] - a[2]) * az) / l2, 0, 1);
			float qx = a[0] + ax * t - p[0];
			float qz = a[2] + az * t - p[2];
			float d = qx * qx + qz * qz;
			if (d < bestD)
			{
				bestD = d;
				best = (j + t) * DL_PATH_STEP;
			}
		}
		return best;
	}

	protected void DL_StartWalking(vector lead, vector fwd, vector cam, vector target)
	{
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		if (!m_DL_Walkers || m_DL_Walkers.Count() == 0 || len - m_DL_WalkLead < DL_RUN_END + 60)
			return;
		m_DL_WalkTrack = new array<float>;
		m_DL_WalkGo = new array<int>;
		for (int i = 0; i < m_DL_Walkers.Count(); i++)
		{
			m_DL_WalkTrack.Insert(Math.Max(m_DL_WalkLead - DL_WalkBack(i), 0));
			m_DL_WalkGo.Insert(0);
		}
		DL_CamFrame(lead, fwd, cam, target);
		m_DL_WalkAt = g_Game.GetTime() + 2500;      // everyone in his place first (the menu character after 1 s)
		for (int g = 0; g < m_DL_WalkGo.Count(); g++)
			m_DL_WalkGo[g] = m_DL_WalkAt + Math.RandomInt(0, DL_WALK_STAGGER);
		m_DL_WalkLast = 0;
		m_DL_Walking = true;
		DL_Log.Detail("menu scene: " + m_DL_Walkers.Count().ToString() + " on foot, " + Math.Round(len - m_DL_WalkLead).ToString() + " m of route ahead, they walk it, then start over");
	}

	// Each walker heads for a point a few metres ahead on his own line (the road's middle, or a pace out to the side)
	// and walks; whoever drops back from his place in the group steps out faster, whoever is ahead slows. The group's
	// front is where its walkers are on average: nobody is left behind. At the route's end all start over.
	protected void DL_Walk()
	{
		int now = g_Game.GetTime();
		if (now < m_DL_WalkAt)
			return;
		float dt = 0;
		if (m_DL_WalkLast > 0)
			dt = Math.Clamp((now - m_DL_WalkLast) / 1000.0, 0, 0.1);
		m_DL_WalkLast = now;
		float len = (m_DL_Path.Count() - 1) * DL_PATH_STEP;
		// where the group's front is: each walker's place along the path plus his place in the group, averaged
		float sum = 0;
		int n = 0;
		for (int i = 0; i < m_DL_Walkers.Count(); i++)
		{
			PlayerBase w = m_DL_Walkers[i];
			if (!w)
				continue;
			float ws = DL_TrackFine(w.GetPosition(), m_DL_WalkTrack[i]);
			m_DL_WalkTrack[i] = ws;
			sum += ws + DL_WalkBack(i);
			n++;
		}
		if (n == 0)
			return;
		float front = sum / n;
		bool restart = front > len - DL_RUN_END;
		if (restart)
		{
			front = DL_WalkBack(m_DL_Walkers.Count()) + 10;
			for (int r = 0; r < m_DL_Walkers.Count(); r++)
			{
				if (!m_DL_Walkers[r])
					continue;
				m_DL_Walkers[r].SetPosition(DL_WalkPlace(r, front));
				m_DL_Walkers[r].SetOrientation(Vector(DL_WalkYaw(r, front), 0, 0));
				m_DL_WalkTrack[r] = Math.Max(front - DL_WalkBack(r), 0);
				m_DL_WalkGo[r] = now + Math.RandomInt(0, DL_WALK_STAGGER);
			}
			m_DL_CamDir = Vector(0, 0, 0);
			DL_Log.Detail("menu scene: walkers back at the start");
		}
		m_DL_WalkLead = front;
		bool log = now - m_DL_WalkLogAt > 10000;
		if (log)
			m_DL_WalkLogAt = now;
		for (int k = 0; k < m_DL_Walkers.Count(); k++)
		{
			PlayerBase pb = m_DL_Walkers[k];
			if (!pb)
				continue;
			if (now < m_DL_WalkGo[k])
			{
				pb.DL_MenuWalk(DL_WalkYaw(k, front), 0);    // standing, facing up the road, until his turn
				continue;
			}
			float s = m_DL_WalkTrack[k];
			float behind = (front - DL_WalkBack(k)) - s;  // > 0: dropped back from his place
			vector pos = pb.GetPosition();
			vector onPath = DL_WalkPlace(k, s + DL_WalkBack(k));
			float off = vector.Distance(Vector(pos[0], 0, pos[2]), Vector(onPath[0], 0, onPath[2]));
			if (off > 6 || Math.AbsFloat(behind) > 12)
			{
				// lost (a fence, a rock, a wrong turn): back in his place
				pb.SetPosition(DL_WalkPlace(k, front));
				pb.SetOrientation(Vector(DL_WalkYaw(k, front), 0, 0));
				m_DL_WalkTrack[k] = Math.Max(front - DL_WalkBack(k), 0);
				DL_Log.Detail("menu scene: walker " + k.ToString() + " back in his place (" + Math.Round(off).ToString() + " m off his line)");
				continue;
			}
			vector aim = DL_WalkPlace(k, Math.Min(s + DL_WalkBack(k) + 3.5, len));
			float yaw = Math.Atan2(aim[0] - pos[0], aim[2] - pos[2]) * Math.RAD2DEG;
			float speed = Math.Clamp(1 + behind * 0.5, 0.5, 2);
			pb.DL_MenuWalk(yaw, speed);
			if (log)
			{
				string wl = "menu scene walk: " + k.ToString() + " s " + Math.Round(s).ToString() + ", behind " + behind.ToString();
				wl += ", speed " + speed.ToString() + ", off " + off.ToString();
				vector ori = pb.GetOrientation();
				wl += ", yaw want " + Math.Round(yaw).ToString() + " has " + Math.Round(ori[0]).ToString();
				DL_Log.Detail(wl);
			}
		}
		vector lpos = DL_PathAt(front);
		lpos[1] = DL_RoadY(lpos[0], lpos[2]);
		DL_FollowCam(lpos, vector.Zero, front, dt, restart);
	}
}
