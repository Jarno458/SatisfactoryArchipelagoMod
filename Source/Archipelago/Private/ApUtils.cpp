#include "ApUtils.h"

#include <string>
#include "Reflection/ClassGenerator.h"

DEFINE_LOG_CATEGORY(LogApUtils);

DEFINE_LOG_CATEGORY(LogArchipelagoCpp);

FText UApUtils::FText(std::string inString) {
	return FText::FromString(FString(inString.c_str()));
}

FString UApUtils::FStr(std::string inString) {
	return FString(inString.c_str());
}

FString UApUtils::FStr(int64_t inInt) {
	return FString(std::to_string(inInt).c_str());
}

UClass* UApUtils::FindOrCreateClass(const TCHAR* PackageName, const TCHAR* ClassName, UClass* ParentClass) {
	if (auto found = FindObject<UClass>(FindPackage(nullptr, PackageName), ClassName, false)) {
		UE_LOG(LogApUtils, Verbose, TEXT("Class %s already exists, returning that instead"), ClassName);
		return found;
	} else {
		UE_LOG(LogApUtils, Verbose, TEXT("Creating class %s in package %s"), ClassName, PackageName);
		return FClassGenerator::GenerateSimpleClass(PackageName, ClassName, ParentClass);
	}
}

FString UApUtils::GetImagePathForItem(UFGItemDescriptor* item) {
	return item->GetBigIconFromInstance()->GetPathName();
}
