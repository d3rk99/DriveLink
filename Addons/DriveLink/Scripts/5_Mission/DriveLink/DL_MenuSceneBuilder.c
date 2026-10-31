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
			return true;
		}
		return false;
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
		m_DL_CamUp = cam[1] - DL_RoadY(cam[0], cam[2]);
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
		dBodyDynamic(m_DL_Cars[i], false);
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
		if (rec >= 0)
			want = Math.Max(rec * DL_REC_SPEED_X, 3);
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
			if (m_DL_Glide.Get(i))
			{
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
				aheadS = gs;
				continue;
			}
			vector pos = c.GetPosition();
			float s = DL_TrackS(pos, m_DL_CarS[i]);
			m_DL_CarS[i] = s;
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
				DL_Log.Detail("menu scene: " + c.GetType() + " stuck at " + Math.Round(s).ToString() + " m, back on the road further on");
				m_DL_StuckFor[i] = 0;
				DL_PutOnPath(i, Math.Min(s + 8, len - DL_RUN_END));
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
			float throttle = Math.Clamp(err * 0.35, 0, 1);
			// a gentle foot: the pedal only rises gradually, and a heavy vehicle gets at most 60 % in first gear
			// (full throttle from standstill made a V3S hop on its stiff rear suspension until it rolled over)
			throttle = Math.Min(throttle, m_DL_Asked.Get(i) + dt * 0.6);
			if (dBodyGetMass(c) > 3000 && DL_V.Gear(c) <= DL_V.NeutralGear(c) + 1)
				throttle = Math.Min(throttle, 0.6);
			m_DL_Asked.Set(i, throttle);
			DL_V.Drive(c, throttle, steer, brake);
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
				if (gear < first || (speed < 3 && gear > first + 1))
				{
					DL_V.ShiftTo(c, first);
					shifted = "to first";
				}
				else if (rev > 0.8 && gear < DL_V.GearCount(c) - 1 && (wasUp || since > 1500))
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
		DL_FollowCam(m_DL_Cars[0].GetPosition(), m_DL_CarS[0], dt, restart);
	}

	// the camera at its place beside and ahead of the lead (at lpos, leadS along the path), turning with the road
	protected void DL_FollowCam(vector lpos, float leadS, float dt, bool restart)
	{
		m_DL_Follow = m_DL_Follow + (lpos - m_DL_Follow) * Math.Min(1, dt * 4);
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
		cam[1] = Math.Max(DL_RoadY(cam[0], cam[2]), lp[1]) + m_DL_CamUp;
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
		DL_FollowCam(lpos, front, dt, restart);
	}
}
