// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseBabaObject.h"
#include "BabaPC.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FObjectRulePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Pair Struct")
		FName ObjectID;

	/*all rules that are activated for this certain object ID*/
	UPROPERTY(EditAnywhere, Category = "Pair Struct")
		TArray<EBabaRules> AppliedRules;

};

UCLASS()
class BABA_IS_YOU_API ABabaPC : public APlayerController
{
	GENERATED_BODY()

		ABabaPC();
public:

	/*this map will have pair of object id and the rule that should be applied on it*/
	UPROPERTY(EditAnywhere, Category = "Activated Rules")
		TArray<FObjectRulePair> ActivatedRules;


	void ActivateRule(FName ID, EBabaRules Rule);

	void DeactivateRule(FName ID, EBabaRules Rule);

	TArray<EBabaRules>& GetAppliedRulesList(FName ID, bool& found);
	

};
