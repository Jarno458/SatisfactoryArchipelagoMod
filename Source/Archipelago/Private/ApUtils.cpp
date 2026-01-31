#include "ApUtils.h"

#include <string>

#include "StructuredLog.h"
#include "Reflection/ClassGenerator.h"
#include "Module/GameInstanceModuleManager.h"
#include "Module/ApGameInstanceModule.h"

DEFINE_LOG_CATEGORY(LogApUtils);

DEFINE_LOG_CATEGORY(LogArchipelagoCpp);

FText UApUtils::FText(std::string inString) {
	return FText::FromString(FString(inString.c_str()));
}

FString UApUtils::FStr(std::string inString) {
	return FString(inString.c_str());
}

FString UApUtils::FStr(int64 inInt) {
	return FString(std::to_string(inInt).c_str());
}

TTuple<bool, UClass*> UApUtils::FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass) {
	if (auto found = FindObject<UClass>(FindPackage(nullptr, packageName), className, false)) {
		UE_LOG(LogApUtils, Verbose, TEXT("Class %s already exists, returning that instead"), className);
		return TTuple<bool, UClass*>(true, found);
	} else {
		UE_LOG(LogApUtils, Verbose, TEXT("Creating class %s in package %s"), className, packageName);
		return TTuple<bool, UClass*>(false, FClassGenerator::GenerateSimpleClass(packageName, className, parentClass));
	}
}

FString UApUtils::GetImagePathForItem(UFGItemDescriptor* item) {
	return item->GetBigIconFromInstance()->GetPathName();
}

UApGameInstanceModule* UApUtils::GetGameInstanceModule(UObject* worldContext) {
	if (const auto world = worldContext->GetWorld()) {
		const auto moduleManager = world->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>();
		const auto module = moduleManager->FindModule("Archipelago");
		return Cast<UApGameInstanceModule>(module);
	}
	return nullptr;
}

FString UApUtils::YankParseValueString(AP_SetReply& setReply) {
	//example 54737402054455566

	// this breaks on really long values due to internal conversion to double
	//std::string valueCstr = (*(std::string*)setReply.value);

	FString intergerValueString = TEXT("0");

	FString rawPacket = UApUtils::FStr(*setReply.raw);

	//destroy opperators
	int32 opperatorStart = rawPacket.Find(TEXT("\"operations\":[{"));
	if (opperatorStart == INDEX_NONE) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() opperator not found in packet: {0}", rawPacket);
		return TEXT("0");
	}
	int32 opperatorEnd = rawPacket.Find(TEXT("}],"), ESearchCase::IgnoreCase, ESearchDir::FromStart, opperatorStart);
	if (opperatorEnd == INDEX_NONE) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() opperator end not found in packet: {0}", rawPacket);
		return TEXT("0");
	}

	FString reducedRawPacket = rawPacket.Left(opperatorStart) + rawPacket.Right(rawPacket.Len() - opperatorEnd);

	//find value
	FString valueKey = FString(TEXT("\"value\":"));
	int32 valueStart = reducedRawPacket.Find(valueKey);
	if (valueStart == INDEX_NONE) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() value not found in packet: {0}", reducedRawPacket);
		return TEXT("0");
	}
	valueStart += valueKey.Len();

	int32 valueEnd = reducedRawPacket.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, valueStart);
	if (valueEnd == INDEX_NONE)
		valueEnd = reducedRawPacket.Find(TEXT("}"), ESearchCase::IgnoreCase, ESearchDir::FromStart, valueStart);
	if (valueEnd == INDEX_NONE) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() value end not found packet: {0}", reducedRawPacket);
		return TEXT("0");
	}

	FString rawValue = reducedRawPacket.Mid(valueStart, valueEnd - valueStart).TrimStartAndEnd();

	if (rawValue.IsEmpty()) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() raw value not found");
		return TEXT("0");
	} 
	
	UE_LOGFMT(LogApUtils, Verbose, "UApUtils::YankParseValueString() found rawvalue {0}", rawValue);

	//cut off decimals, should not happen, but we cant trust other games
	if (rawValue.Contains(".")) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() received non numeric input {0}", rawValue);
		intergerValueString = rawValue.Left(rawValue.Find(".", ESearchCase::IgnoreCase, ESearchDir::FromEnd));
	}
	else {
		intergerValueString = rawValue;
	}

	if (!intergerValueString.IsNumeric()) {
		UE_LOGFMT(LogApUtils, Error, "UApUtils::YankParseValueString() received non numeric input {0}", intergerValueString);
		return TEXT("0");
	}

	return intergerValueString;
}

int256 UApUtils::Int256FromBigIntString(FString bigInt) {
	bool negative = false;
	if (bigInt.StartsWith(TEXT("-"))) {
		negative = true;
		bigInt = bigInt.RightChop(1);
	}

	FString reverse = bigInt.Reverse();

	int256 total = 0;
	int multiplier = 0;

	for (TCHAR c : reverse) {
		if (!FChar::IsDigit(c)) {
			UE_LOGFMT(LogApUtils, Error, "UApUtils::Int256FromBigIntString() received non numeric input {0} in string {1}", c, bigInt);
			total = 0;
		}

		int256 multiplierValue = int256::One;
		for (int i = 0; i < multiplier; i++) {
			multiplierValue *= 10;
		}

		total += (multiplierValue * FChar::ConvertCharDigitToInt(c));
		multiplier++;
	}

	if (negative)
		total *= -1;

	UE_LOGFMT(LogApUtils, VeryVerbose, "UApUtils::Int256FromBigIntString() validation, input: {0} > hex: {1} > validation: {2}", bigInt, total.ToString(), FParse::HexNumber(*total.ToString()));

	return total;
}

UApBlueprintDataBridge* UApUtils::GetBlueprintDataBridge(UObject* worldContext) {
	if (const auto modModule = UApUtils::GetGameInstanceModule(worldContext)) {
		return modModule->BlueprintData;
	}
	return nullptr;
}

void UApUtils::WriteStringToFile(FString Path, FString text, bool relative) {
#if WITH_EDITOR 
	FFileHelper::SaveStringToFile(text, relative ? *(FPaths::ProjectDir() + Path) : *Path);
#else
	const FString AbsoluteRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString AbsolutePath = AbsoluteRootPath + TEXT("Mods/") + Path;
	if (!AbsolutePath.Contains(TEXT(".."))) {
		FFileHelper::SaveStringToFile(text, *AbsolutePath);
	}
	else {
		UE_LOG(LogApUtils, Error, TEXT("Absolute or escaping Paths are not allowed in Runtime"));
	}
#endif
}

bool UApUtils::IsApPlayerValid(FApPlayer player) {
	return player.IsValid();
}
