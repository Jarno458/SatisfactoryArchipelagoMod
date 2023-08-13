#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ApUtils.generated.h"

/**
 * 
 */
UCLASS()
class ARCHIPELAGO_API UApUtils : public UObject
{
	GENERATED_BODY()

public:
	static FText FText(std::string inString);

	static FString FStr(std::string inString);

	static FString FStr(int64_t inInt);
	
};
