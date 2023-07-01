// Fill out your copyright notice in the Description page of Project Settings.


#include "BabaPC.h"
ABabaPC::ABabaPC()
{
	FObjectRulePair Temp;
	Temp.AppliedRules.Add(EBabaRules::you);
	Temp.ObjectID = "Baba";
	ActivatedRules.Add(Temp);

	Temp.AppliedRules.Add(EBabaRules::Push);
	Temp.ObjectID = "Wall";
	ActivatedRules.Add(Temp);
}

void ABabaPC::ActivateRule(FName ID, EBabaRules Rule)
{
	bool found;
	TArray<EBabaRules>& FoundList = GetAppliedRulesList(ID, found);

	if (found)
	{
		if (GEngine)
		{
			FString Mes = GetOuter()->GetFName().ToString();
			FString BabaLog = "PC Activate from " + Mes;
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, BabaLog);
		}

		FoundList.Add(Rule);
	}

	else
	{
		FObjectRulePair Temp;
		Temp.AppliedRules.Add(Rule);
		Temp.ObjectID = ID;
		ActivatedRules.Add(Temp);
	}
}

void ABabaPC::DeactivateRule(FName ID, EBabaRules Rule)
{
	bool found;
	TArray<EBabaRules>& FoundList = GetAppliedRulesList(ID, found);

	if (found)
	{
		for (int32 i = 0; i <= FoundList.Num() - 1; i++)
		{
			if (Rule == FoundList[i])
			{
				FoundList.RemoveAt(i);
				break; // only remove one instance of a rule if many of the same rule where applied so that they dont lost effect
			}
		}
	}
}

TArray<EBabaRules>& ABabaPC::GetAppliedRulesList(FName ID, bool& found)
{
	TArray<EBabaRules> EmptyList;

	for (auto& Element : ActivatedRules)
	{
		if (ID == Element.ObjectID)
		{
			found = true;
			return Element.AppliedRules;
		}
	}

	found = false;
	return EmptyList;
}
