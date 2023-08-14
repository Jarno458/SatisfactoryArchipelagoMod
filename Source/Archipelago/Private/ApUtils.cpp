#include "ApUtils.h"

#include <string>
#include "Reflection/ClassGenerator.h"

DEFINE_LOG_CATEGORY(LogApUtils);

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
	// TODO ANY_PACKAGE deprecation, find a different object finder that works
	// the excessive variables are to allow for debugger inspection as it works, clean up later
	auto found = FindObject<UClass>(ANY_PACKAGE, ClassName, false);
	if (found) {
		UE_LOG(LogApUtils, Display, TEXT("Class %s already exists, returning that instead"), ClassName);
		const auto test = found;
		return test;
	} else {
		UE_LOG(LogApUtils, Display, TEXT("Creating class %s in package %s"), ClassName, PackageName);
		return FClassGenerator::GenerateSimpleClass(PackageName, ClassName, ParentClass);
	}
}
