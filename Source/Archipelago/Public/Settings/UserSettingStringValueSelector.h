#pragma once

#include "CoreMinimal.h"
#include "Misc/Variant.h"
#include "FGOptionInterface.h"
#include "FGUserSetting.h"

#include "UserSettingStringValueSelector.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUserSettingStringValueSelector, Log, All);

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, meta=(DisplayName = "String"))
class ARCHIPELAGO_API  UUserSettingStringValueSelector : public UFGUserSetting_ValueSelector
{
	GENERATED_BODY()
	
public:
	virtual EOptionType GetOptionType() const override { return EOptionType::OT_Custom; }

	virtual FVariant GetDefaultValue() const override { return FName(defaultText); }

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Value")
	FString defaultText;
};
