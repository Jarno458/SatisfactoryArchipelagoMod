#pragma once

#include "CoreMinimal.h"
#include "Misc/Variant.h"
#include "FGUserSetting.h"

#include "UserSettingStringValueSelector.generated.h"

UCLASS(Blueprintable, DefaultToInstanced, EditInlineNew, meta=(DisplayName="Text Input"))
class ARCHIPELAGO_API UUserSettingFNameValueSelector : public UFGUserSetting_ValueSelector
{
	GENERATED_BODY()
	
public:
	virtual EOptionType GetOptionType() const override { return EOptionType::OT_Custom; }

	virtual FVariant GetDefaultValue() const override { return FName(defaultText.ToString()); }

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Value")
	FText defaultText;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Value")
	bool isPassword;
};
