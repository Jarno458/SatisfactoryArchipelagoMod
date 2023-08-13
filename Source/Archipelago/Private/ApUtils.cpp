#include "ApUtils.h"
#include <string>

FText UApUtils::FText(std::string inString) {
	return FText::FromString(FString(inString.c_str()));
}

FString UApUtils::FStr(std::string inString) {
	return FString(inString.c_str());
}

FString UApUtils::FStr(int64_t inInt) {
	return FString(std::to_string(inInt).c_str());
}
