#include "Subsystem/ApExplorationSubsystem.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogApExplorationSubsystem);

FApExplorationData::FApExplorationData() {
}

// TODO there should be 109 of these according to SCIM?
TMap<int64, FVector_NetQuantize> AApExplorationSubsystem::IdToDropPodLocation = {
	// Regenerate via /Script/Engine.Blueprint'/Archipelago/Debug/CC_BuildDropPodLocations.CC_BuildDropPodLocations'
	{1, FVector_NetQuantize(-29068, -22640, 17384)},
	{2, FVector_NetQuantize(-33340, 5176, 23519)},
	{3, FVector_NetQuantize(8680, -41777, 13053)},
	{4, FVector_NetQuantize(35082, 16211, 22759)},
	{5, FVector_NetQuantize(-3511, 62314, 22109)},
	{6, FVector_NetQuantize(66652, -13642, 13420)},
	{7, FVector_NetQuantize(55247, -51316, 14363)},
	{8, FVector_NetQuantize(-4706, -76301, 13618)},
	{9, FVector_NetQuantize(-40194, 62956, 26261)},
	{10, FVector_NetQuantize(80980, -44100, 8303)},
	{11, FVector_NetQuantize(-56144, -72864, 27668)},
	{12, FVector_NetQuantize(-95228, 6970, 25142)},
	{13, FVector_NetQuantize(-89284, -50630, 16019)},
	{14, FVector_NetQuantize(-94708, 40337, 19832)},
	{15, FVector_NetQuantize(94267, 47237, 9435)},
	{16, FVector_NetQuantize(87739, -62975, 13444)},
	{17, FVector_NetQuantize(12249, 114177, 26721)},
	{18, FVector_NetQuantize(115978, 21424, 15519)},
	{19, FVector_NetQuantize(-78236, 90857, 20305)},
	{20, FVector_NetQuantize(-35359, 116594, 21827)},
	{21, FVector_NetQuantize(111479, -54515, 17081)},
	{22, FVector_NetQuantize(121061, 45324, 17373)},
	{23, FVector_NetQuantize(125497, -34949, 8220)},
	{24, FVector_NetQuantize(-26327, -129047, 7780)},
	{25, FVector_NetQuantize(21373, 132336, 2510)},
	{26, FVector_NetQuantize(17807, -136922, 13621)},
	{27, FVector_NetQuantize(-118480, 74929, 16995)},
	{28, FVector_NetQuantize(94940, 105482, 9860)},
	{29, FVector_NetQuantize(-129115, 60165, 4800)},
	{30, FVector_NetQuantize(-142000, 23970, 32660)},
	{31, FVector_NetQuantize(46048, 141933, 13064)},
	{32, FVector_NetQuantize(144456, 36294, 17301)},
	{33, FVector_NetQuantize(-43144, 145820, 7472)},
	{34, FVector_NetQuantize(-108774, 107811, 10154)},
	{35, FVector_NetQuantize(-56987, -144603, 2072)},
	{36, FVector_NetQuantize(-152676, 33864, 19283)},
	{37, FVector_NetQuantize(90313, 129583, 9112)},
	{38, FVector_NetQuantize(111212, -113040, 12036)},
	{39, FVector_NetQuantize(-157077, -6312, 25128)},
	{40, FVector_NetQuantize(157249, -40206, 13694)},
	{41, FVector_NetQuantize(-151842, 72468, 9945)},
	{42, FVector_NetQuantize(64696, 156038, 14067)},
	{43, FVector_NetQuantize(-157080, -67028, 11766)},
	{44, FVector_NetQuantize(170057, -10579, 18823)},
	{45, FVector_NetQuantize(143671, 92573, 24990)},
	{46, FVector_NetQuantize(127215, -116866, -1397)},
	{47, FVector_NetQuantize(163999, 61333, 21481)},
	{48, FVector_NetQuantize(98306, -149781, 2552)},
	{49, FVector_NetQuantize(5302, -187090, -1608)},
	{50, FVector_NetQuantize(188304, 17059, 12949)},
	{51, FVector_NetQuantize(84256, -171122, -290)},
	{52, FVector_NetQuantize(191366, 37694, 5676)},
	{53, FVector_NetQuantize(28695, 193441, 17459)},
	{54, FVector_NetQuantize(-146044, -137047, 2357)},
	{55, FVector_NetQuantize(-200203, -17766, 12193)},
	{56, FVector_NetQuantize(47834, 195703, 2943)},
	{57, FVector_NetQuantize(198418, -41186, 13786)},
	{58, FVector_NetQuantize(-195756, -59210, -84)},
	{59, FVector_NetQuantize(-121994, 166916, -49)},
	{60, FVector_NetQuantize(88323, 188913, 1420)},
	{61, FVector_NetQuantize(-123677, -167107, 29710)},
	{62, FVector_NetQuantize(150633, 146698, 7727)},
	{63, FVector_NetQuantize(-55111, -204857, 7844)},
	{64, FVector_NetQuantize(216096, -268, -1592)},
	{65, FVector_NetQuantize(159088, -145116, 23164)},
	{66, FVector_NetQuantize(207683, -68352, 3927)},
	{67, FVector_NetQuantize(-189258, 116331, -1764)},
	{68, FVector_NetQuantize(46951, 221859, 5917)},
	{69, FVector_NetQuantize(-9988, 227625, -1017)},
	{70, FVector_NetQuantize(232515, -20519, 8979)},
	{71, FVector_NetQuantize(232138, 27191, -1629)},
	{72, FVector_NetQuantize(-135, -237257, -1760)},
	{73, FVector_NetQuantize(-232498, -51432, -386)},
	{74, FVector_NetQuantize(-238333, 17321, 19741)},
	{75, FVector_NetQuantize(200510, 131912, 6341)},
	{76, FVector_NetQuantize(-108812, 214051, 3200)},
	{77, FVector_NetQuantize(232255, 79925, -1275)},
	{78, FVector_NetQuantize(226418, 98109, 7339)},
	{79, FVector_NetQuantize(156569, 191767, -9312)},
	{80, FVector_NetQuantize(44579, -244343, -874)},
	{81, FVector_NetQuantize(118349, 221905, -7063)},
	{82, FVector_NetQuantize(249919, 59534, 2430)},
	{83, FVector_NetQuantize(188233, 177201, 9608)},
	{84, FVector_NetQuantize(-174494, -197134, -1538)},
	{85, FVector_NetQuantize(-50655, -259272, -1667)},
	{86, FVector_NetQuantize(30383, 266975, -987)},
	{87, FVector_NetQuantize(272715, 28087, -1586)},
	{88, FVector_NetQuantize(-152279, 229520, 1052)},
	{89, FVector_NetQuantize(241532, 131343, 17157)},
	{90, FVector_NetQuantize(-259577, 105048, -1548)},
	{91, FVector_NetQuantize(275070, -52585, 5980)},
	{92, FVector_NetQuantize(-247303, -142348, 4524)},
	{93, FVector_NetQuantize(261797, 124616, -2597)},
	{94, FVector_NetQuantize(187056, 223656, -3215)},
	{95, FVector_NetQuantize(293299, 51, 522)},
	{96, FVector_NetQuantize(219146, -199880, 6503)},
	{97, FVector_NetQuantize(176423, 243273, -9780)},
	{98, FVector_NetQuantize(291821, 74782, -1574)},
	{99, FVector_NetQuantize(-78884, 292640, -4763)},
	{100, FVector_NetQuantize(174948, -276436, 21151)},
	{101, FVector_NetQuantize(295166, -173139, 8083)},
	{102, FVector_NetQuantize(349295, -38831, -1485)},
	{103, FVector_NetQuantize(360114, -106614, 11815)},
	{104, FVector_NetQuantize(303169, -246169, 5487)},
	{105, FVector_NetQuantize(236508, -312236, 9971)},
	{106, FVector_NetQuantize(360285, -217558, 3900)},
	{107, FVector_NetQuantize(366637, -303548, -7288)},
};

AApExplorationSubsystem* AApExplorationSubsystem::Get(UObject* WorldContext) {
	if (!WorldContext) {
		return nullptr;
	}
	if (WorldContext->GetWorld()) {
		TArray<AActor*> arr;
		// Relies on the fact that nothing has spawned the C++ version before
		// The blueprint one descends from this so it is found instead
		// This would break if a C++ version was spawned or persisted via save game
		UGameplayStatics::GetAllActorsOfClass(WorldContext->GetWorld(), AApExplorationSubsystem::StaticClass(), arr);
		if (arr.IsValidIndex(0)) {
			return Cast<AApExplorationSubsystem>(arr[0]);
		} else {
			return nullptr;
		}
	} else {
		return nullptr;
	}
}

void AApExplorationSubsystem::GetDropPodLocationMap(TMap<int, FVector_NetQuantize> in_idToLocation) {
	for (auto& entry : IdToDropPodLocation) {
		in_idToLocation.Add((int) entry.Key, entry.Value);
	}
}

void AApExplorationSubsystem::PopulateIdToDropPod() {
	UE_LOG(LogApExplorationSubsystem, Verbose, TEXT("AApExplorationSubsystem::PopulateIdToDropPod"));

	TArray<AActor*> arr;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGDropPod::StaticClass(), arr);
	if (arr.IsEmpty()) {
		UE_LOG(LogApExplorationSubsystem, Error, TEXT("No drop pods found? Called too early?"));
		return;
	}

	for (const auto& dropPod : arr) {
		auto location = FVector_NetQuantize(dropPod->GetActorLocation());
		auto truncLocation = FVector_NetQuantize((int)location.X, (int)location.Y, (int)location.Z);

		if (const auto id = IdToDropPodLocation.FindKey(truncLocation)) {
			UE_LOG(LogApExplorationSubsystem, Display, TEXT("ID %d matched drop pod at location %s"), *id, *truncLocation.ToCompactString());
			IdToDropPod.Add(*id, Cast<AFGDropPod>(dropPod));
		} else {
			UE_LOG(LogApExplorationSubsystem, Warning, TEXT("Found a drop pod in the world we that don't have data for? Skipping. At location %s"), *truncLocation.ToCompactString());
		}
	}
}
