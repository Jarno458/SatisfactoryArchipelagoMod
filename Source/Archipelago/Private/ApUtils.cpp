#include "ApUtils.h"

FText UApUtils::ApText(std::string inString) {
	return FText::FromString(FString(inString.c_str()));
}

FString UApUtils::ApString(std::string inString) {
	return FString(inString.c_str());
}
