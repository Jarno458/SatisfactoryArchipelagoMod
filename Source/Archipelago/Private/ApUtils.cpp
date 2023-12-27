#include "ApUtils.h"

#include <string>
#include "Reflection/ClassGenerator.h"
#include "Module/GameInstanceModuleManager.h"
#include "Module/ApGameInstanceModule.h"

//TODO REMOVE
#pragma optimize("", off)

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

UClass* UApUtils::FindOrCreateClass(const TCHAR* packageName, const TCHAR* className, UClass* parentClass) {
	if (auto found = FindObject<UClass>(FindPackage(nullptr, packageName), className, false)) {
		UE_LOG(LogApUtils, Verbose, TEXT("Class %s already exists, returning that instead"), className);
		return found;
	} else {
		UE_LOG(LogApUtils, Verbose, TEXT("Creating class %s in package %s"), className, packageName);
		return FClassGenerator::GenerateSimpleClass(packageName, className, parentClass);
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

#pragma optimize("", on)