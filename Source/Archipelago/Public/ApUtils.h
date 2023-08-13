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
	static FText ApText(std::string inString);

	static FString ApString(std::string inString);
	
};
