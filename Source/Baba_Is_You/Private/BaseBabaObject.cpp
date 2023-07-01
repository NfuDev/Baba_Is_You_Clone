// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseBabaObject.h"
#include "BabaPC.h"
#include "Kismet/GameplayStatics.h"
#include <Kismet/KismetMathLibrary.h>


// Sets default values
ABaseBabaObject::ABaseBabaObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BabaFlipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flip Book"));

	TraceDistance = 15.0f;
	GridSize = 24.0f;
	
}

ABaseBabaObject::~ABaseBabaObject()
{
	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Yellow, "Class Destroied");
	}

	for (auto& Tile : NearTiles)
	{
		if (Tile)
		{
			//Tile->GenerateVisualsByDrag(false); //AmDeleted(this);//when the class is deleted we tell the near by about it
		}
	}
}

// Called when the game starts or when spawned
void ABaseBabaObject::BeginPlay()
{
	Super::BeginPlay();

	DefaultID = ObjectID;
	
    inputListenerPawn = Cast<AInputsListenerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (inputListenerPawn)
	{
		inputListenerPawn->InputReceivedDelegate.AddDynamic(this, &ABaseBabaObject::Move);
		inputListenerPawn->RewindDelegate.AddDynamic(this, &ABaseBabaObject::RestoreObjectState);
		inputListenerPawn->RecordDelegate.AddDynamic(this, &ABaseBabaObject::RecordObjectState);
	}
		
		
	
	//getting the player controller reference	
	BabaPC = Cast<ABabaPC>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (ObjectType == EGameObjectType::activatorObject)
	{
		ReactToPush(FVector());//trying to activate all rules that are already aligned at begin play, using react to push in nutral direction since we dont even use it
	}

	ReactToMovementVisually();// this will give all tiles a reference to each other in begin play
}

// Called every frame
void ABaseBabaObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine)
	{

	//	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::FromInt(ObjectStateRecords.Num()));
	}
}


void ABaseBabaObject::SetupVisuals()
{
	
	if (WallType != EWallType::None && WallTypeVisuals.Num() > 0)
	{
		int32 _index = int(WallType) - 1;
		UPaperFlipbook* NewVisual = WallTypeVisuals.IsValidIndex(_index) ? WallTypeVisuals[_index] : nullptr;

		if(NewVisual)
		BabaFlipbook->SetFlipbook(NewVisual);
	}
}


bool ABaseBabaObject::CheckIfRuleApplied(EBabaRules RuleToCheck)
{
	bool found;
	TArray<EBabaRules>& AppliedRules = BabaPC->GetAppliedRulesList(ObjectID, found);
	
	return AppliedRules.Contains(RuleToCheck);
}


void ABaseBabaObject::Move(FVector Direction)
{
	//RecordObjectState();//when ever the user hit any inputs we record it's state and other game objects states 

	if (CheckIfRuleApplied(EBabaRules::you) && ObjectType == EGameObjectType::gameObject)
	{
		if (GEngine)
		{
			/*FString Mes = "Moved And ID is : ";
			Mes += ObjectID.ToString();*/
			//GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, Mes);
		}

		if (ObjectType == EGameObjectType::gameObject && WallType == EWallType::None)
		{
			SetupAnimation(Direction);
		}
		
		bool MovedSuccessfully = TryToMove(Direction);
		if (MovedSuccessfully)
		{
			inputListenerPawn->RecordDelegate.Broadcast();
		}
	
	}
	
}

bool ABaseBabaObject::ApplyMovement(FVector Direction, const TArray<EBabaRules> ObstacleAppliedRules, FHitResult ObstacleFound)
{
	if (ObstacleFound.bBlockingHit)//has obstacle
	{
		ABaseBabaObject* Obstacle = Cast<ABaseBabaObject>(ObstacleFound.GetActor());

		bool ForceMove = Obstacle->ObjectType != EGameObjectType::gameObject;

		if (!ObstacleAppliedRules.Contains(EBabaRules::Stop) && !ObstacleAppliedRules.Contains(EBabaRules::you) || ForceMove)
		{
			if (ObstacleAppliedRules.Contains(EBabaRules::Push) || ForceMove)//i need this so i can seperate the push and overlap
			{
				
				if (Obstacle->TryToMove(Direction))
				{
					FVector NextGrid = GetActorLocation() + (Direction * GridSize);
					SetActorLocation(NextGrid);
					
					return true;

				}

			}

			else
			{
				
				FVector NextGrid = GetActorLocation() + (Direction * GridSize);
				SetActorLocation(NextGrid);
				ProcessOverLap();//if not push and also not stop then we will overlap with is so we process it and see the results

				if (GEngine)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, "Processing OverLap");
				}
				return true;
				
			}
			
		}

		else
		{
			if (ObstacleAppliedRules.Contains(EBabaRules::you))
			{
				FVector NextGrid = GetActorLocation() + (Direction * GridSize);
				SetActorLocation(NextGrid);

				return true;
			}

			return false;
		}

		
	}

	else//has no obstacle then move normally
	{
		
		FVector NextGrid = GetActorLocation() + (Direction * GridSize);
		SetActorLocation(NextGrid);
		return true;
	}

	return false;
}


bool ABaseBabaObject::TraceInDirection(FVector Direction, FHitResult& TraceResults)
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.AddUnique(this);

	FVector Start = GetActorLocation();
	FVector End = (Direction * TraceDistance) + Start;
	
	ETraceTypeQuery TraceQuery = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	
	bool hit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, 5.0f, TraceQuery, true, ActorToIgnore, DebugType,
		TraceResults, true, FColor::Red, FColor::Green, 2.0f);

	
	return hit;
}

bool ABaseBabaObject::TraceInDirection(FVector Location, FVector Direction, FHitResult& TraceResults)//overloaded for level desing use only
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.AddUnique(this);//ignore self

	ActorToIgnore.AddUnique(GetOverlappedTile());//ignore overlapped objects

	AddNoneWallObjects(ActorToIgnore);//ignore all none wall objects
	
	FVector Start = GetActorLocation();
	FVector End = (Direction * TraceDistance) + Start;

	ETraceTypeQuery TraceQuery = UEngineTypes::ConvertToTraceType(ECC_Visibility);

	bool hit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, 5.0f, TraceQuery, true, ActorToIgnore, DebugType,
		TraceResults, true, FColor::Red, FColor::Green, 2.0f);

	ABaseBabaObject* BabaObject = Cast<ABaseBabaObject>(TraceResults.GetActor());

	return hit && BabaObject->ObjectType == EGameObjectType::gameObject && BabaObject->WallType != EWallType::None;
}

void ABaseBabaObject::SetupAnimation(FVector Direction)
{
	TArray<FString> MappedEntries = { "PX", "NX", "PY", "NY" };

	FString Str = FMath::Abs(Direction.Y) > FMath::Abs(Direction.X) ? "Y" : "X";
	float value = FMath::Abs(Direction.Y) > FMath::Abs(Direction.X) ? Direction.Y : Direction.X;
	FString Subfix = value > 0 ? "P" : "N";

	Subfix += Str;//string represnts the enum entry

	EAnimationType MapKey = EAnimationType(MappedEntries.Find(Subfix));
	
	if (Animations.Num() > 0)
	{
		FAnimationsList& AnimationsList = *Animations.Find(MapKey);

		BabaFlipbook->SetFlipbook(AnimationsList.Animations[animationIndex]);
		SetActorScale3D(FVector(value, 1, 1));

		animationIndex++;
		if (animationIndex > 2)
			animationIndex = 0;
	}

	//debug code
	
	//if (GEngine)
	//{
	//	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("direction in %s and value is %f"), *Str, value));
	//	GEngine->AddOnScreenDebugMessage(-1, 0.3f, FColor::Yellow, *Subfix);
	//} 

}

bool ABaseBabaObject::TryToMove(FVector Direction)
{

	TArray<EBabaRules> TempAppliedRules;
	FHitResult HitResults;
	bool found;

	TraceInDirection(Direction, HitResults);//returns the hit if found 

	FName TargetID = HitResults.bBlockingHit? Cast<ABaseBabaObject>(HitResults.GetActor())->ObjectID : "None";
	TempAppliedRules = HitResults.bBlockingHit ? BabaPC->GetAppliedRulesList(TargetID, found) : TempAppliedRules;

	bool MovementResults =  ApplyMovement(Direction, TempAppliedRules, HitResults);//start processing inputs to see if this object can move

	if (MovementResults && ObjectType != EGameObjectType::gameObject)//this code only runs for words and ruls and activator objects
	{
		if (GEngine)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, "is push");
		}

		ReactToPush(Direction);
	}

	if (MovementResults)
	{
		ReactToMovementVisually();
	}

	return MovementResults;

}

void ABaseBabaObject::ProcessOverLap()
{
	FHitResult HitResults;
	TraceInDirection(FVector(1,0,0), HitResults);
	bool found;

	ABaseBabaObject* OverlappedObject = Cast<ABaseBabaObject>(HitResults.GetActor());

	TArray<EBabaRules> AppliedRules;

	if (OverlappedObject)
	{
		FName OverLappedID = OverlappedObject->ObjectID;

		AppliedRules = BabaPC->GetAppliedRulesList(OverLappedID, found);
	}
	

	//if found then we may have a behaviour when overlapped mostly kill or open or win
	if (found)
	{
		if (AppliedRules.Contains(EBabaRules::kill))
		{

		}
		else if (AppliedRules.Contains(EBabaRules::win))
		{

		}
		else if (AppliedRules.Contains(EBabaRules::open))
		{

		}
	}
}

void ABaseBabaObject::RecordObjectState()
{
	
	FObjectState NewObjectState;
	NewObjectState.Location = GetActorLocation();
	NewObjectState.Animation = BabaFlipbook->GetFlipbook();
	NewObjectState.Scale = GetActorScale3D();
	NewObjectState.ID = ObjectID;
	//.. if more 

	ObjectStateRecords.Add(NewObjectState);
}

void ABaseBabaObject::RestoreObjectState()
{
	

	if (ObjectStateRecords.Num() > 0)
	{
		int32 LastIndex = ObjectStateRecords.Num() - 1;

		FObjectState RestoredObjectState = ObjectStateRecords[LastIndex];

		SetActorLocation(RestoredObjectState.Location);
		BabaFlipbook->SetFlipbook(RestoredObjectState.Animation);
		SetActorScale3D(RestoredObjectState.Scale);
		ObjectID = RestoredObjectState.ID;
		ObjectStateRecords.RemoveAt(LastIndex);
	}
	
}

void ABaseBabaObject::ReactToPush(FVector Direction)
{
	//the two directions used by the word or the rule to check for activator to poke it to activate a rule
	FVector V_DirToActivator;
	FVector H_DirToActivator;

	FHitResult RightCheckResults;
	FHitResult LeftheckResults;
	FHitResult UpCheckResults;
	FHitResult DownCheckResults;

	

	//AInputsListenerPawn* inputListenerPawn = Cast<AInputsListenerPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	TraceInDirection(inputListenerPawn->GetActorForwardVector(), UpCheckResults);
	TraceInDirection(inputListenerPawn->GetActorForwardVector() * -1, DownCheckResults);

	TraceInDirection(inputListenerPawn->GetActorRightVector(), RightCheckResults);
	TraceInDirection(inputListenerPawn->GetActorRightVector() * -1, LeftheckResults);

	TArray<FHitResult> ResultsInAllDirections;
	ResultsInAllDirections.Add(UpCheckResults);
	ResultsInAllDirections.Add(DownCheckResults);
	ResultsInAllDirections.Add(RightCheckResults);
	ResultsInAllDirections.Add(LeftheckResults);


	if (ObjectType != EGameObjectType::activatorObject)//search for activator and tell it to try to activate
	{

		for (int32 index = 0; index < 4; index++)//four directions
		{
			auto& DirectionResult = ResultsInAllDirections[index];

			if (DirectionResult.bBlockingHit)
			{
				ABaseBabaObject* HitObject = Cast<ABaseBabaObject>(DirectionResult.GetActor());

				if (HitObject->ObjectType == EGameObjectType::activatorObject)
				{
				
					HitObject->ReactToPush(Direction);//used ract to push since i want to check all directions in the activator again
					MyActivators.AddUnique(HitObject);
					if (GEngine)
					{
					//	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "rule or word Trying To Activate");
					}
				}
			}

			else//if there are no hits in specific direction we try to find if there were an activator so we tell it that we moved
			{

				if (MyActivators.IsValidIndex(index))
				{
				   
				   MyActivators[index]->ReactToPush(Direction);
				   MyActivators.RemoveAt(index);
				   if (GEngine)
				   {
					 //  GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "Sus Code");
				   }
				}
				
			}
		}
	}

	else 
	{
		//if the pushed was activator then directly try to activate
		TryToActivate(RightCheckResults, LeftheckResults, UpCheckResults, DownCheckResults);
		SetupWord_Rule_Activators(ResultsInAllDirections);//set activator reference in the rule and object 
		if (GEngine)
		{
		//	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "Activator Trying To Activate");
		}

	}
	
}

void ABaseBabaObject::TryToActivate(FHitResult RightHit, FHitResult LeftHit, FHitResult UPHit, FHitResult DownHit)
{
	if (GEngine && UPHit.bBlockingHit && DownHit.bBlockingHit)
	{
		/*ABaseBabaObject& wordRef = *Cast<ABaseBabaObject>(UPHit.GetActor());
		ABaseBabaObject& RuleRef = *Cast<ABaseBabaObject>(DownHit.GetActor());

		FString Mes = "Poked and word / rule are : ";
		Mes += FString(wordRef.ObjectID.ToString()) + "/" + FString(UEnum::GetValueAsString(RuleRef.Rule));*/
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Mes);
	}

	bool bWordRuleAlignedH;
	bool bWordRuleAlignedV;

	//try and return the results if not found the deactivate last activated rule
	bWordRuleAlignedH = HLastActivatedMap.CanActivate(LeftHit, RightHit)? TryToActivateAxis(LeftHit, RightHit, HLastActivatedMap) : true;//horezintal activation
	bWordRuleAlignedV = VLastActivatedMap.CanActivate(UPHit, DownHit)? TryToActivateAxis(UPHit, DownHit, VLastActivatedMap) : true;// vertical activation

	//deactivate if found previous activation and faild to activate new
	if (!bWordRuleAlignedH && HLastActivatedMap.IsActive())
	{
		if (GEngine)
		{
			FString Mes = "Rule Deactivated : " + FString(UEnum::GetValueAsString(HLastActivatedMap.Rule)) + " For ID : " + FString(HLastActivatedMap.ID.ToString());
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, Mes);
		}
		BabaPC->DeactivateRule(HLastActivatedMap.ID, HLastActivatedMap.Rule);
		HLastActivatedMap.Deactivate();
	}

	if (!bWordRuleAlignedV && VLastActivatedMap.IsActive())
	{
		BabaPC->DeactivateRule(VLastActivatedMap.ID, VLastActivatedMap.Rule);
		VLastActivatedMap.Deactivate();
	}

}


//try to activate horizental or vertical 
bool ABaseBabaObject::TryToActivateAxis(FHitResult WordHit, FHitResult RuleHit, FActivationRecord& LastActivatedMap)
{
	if (RuleHit.bBlockingHit && WordHit.bBlockingHit)
	{
		
		ABaseBabaObject* RuleObject = Cast<ABaseBabaObject>(RuleHit.GetActor());
		ABaseBabaObject* WordObject = Cast<ABaseBabaObject>(WordHit.GetActor());

		if (RuleObject->ObjectType == EGameObjectType::ruleObject)
		{
			
			BabaPC->ActivateRule(WordObject->ObjectID, RuleObject->Rule);
			LastActivatedMap.ID = WordObject->ObjectID;
			LastActivatedMap.Rule = RuleObject->Rule;

			LastActivatedMap.WordRule.Add(WordObject);
			LastActivatedMap.WordRule.Add(RuleObject);

			if (GEngine)
			{
				//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, "rule activated");
			}
		}

		else if (RuleObject->ObjectType == EGameObjectType::wordObject)
		{
			TArray<AActor*> AllActors;

			UPaperFlipbook* ID_Visuals = Get_ID_Visuals(RuleObject->ObjectID);//save the visuals of a certain id before we mess with them

			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBabaObject::StaticClass(), AllActors);

			for (auto& itr : AllActors)//filters only baba objects with ID
			{
				ABaseBabaObject* BabaObject = Cast<ABaseBabaObject>(itr);
				
				if (BabaObject->ObjectID == WordObject->ObjectID && BabaObject->ObjectType == EGameObjectType::gameObject)
				{
					FString IDBefore = BabaObject->ObjectID.ToString();

					BabaObject->ObjectID = RuleObject->ObjectID;//transfair the rule object ID means all the rules applied on this id will
					//be available for this object, it is safe since we saved the default ID
					BabaObject->WallType = EWallType::None;
					BabaObject->BabaFlipbook->SetFlipbook(ID_Visuals);

					if (GEngine)
					{/*
						FString Mes = "ID found on Object Named: ";
						Mes += FString(BabaObject->GetName()) + " and ID After / Before : ";
						Mes += FString(BabaObject->ObjectID.ToString()) + " / " + IDBefore;*/
						//GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Red, Mes);
					}
				}

			}//end for

			if (GEngine)
			{
				/*FString Mes = "Word To Word Activation, Transfaired ID is : ";
				Mes += FString(RuleObject->ObjectID.ToString()) + " Actors found : ";
				Mes += FString::FromInt(AllActors.Num());*/
				//GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Red, Mes);
			}

		}//end else

		if (GEngine)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Green, "Hit Results Are Valid");
		}
		return true;
	}//end firt if

	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Red, "Hit Results Are NOT Valid");
	}

	return false;
}

UPaperFlipbook* ABaseBabaObject::Get_ID_Visuals(FName ID)
{
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBabaObject::StaticClass(), AllActors);

	for (auto& itr : AllActors)
	{
		ABaseBabaObject* BabaObject = Cast<ABaseBabaObject>(itr);

		if (BabaObject->ObjectID == ID && BabaObject->ObjectType == EGameObjectType::gameObject)
		{
			return BabaObject->BabaFlipbook->GetFlipbook();
		}
	}
	return nullptr;
}

//called in the activator , but to be applied on the rules and words objects 
void ABaseBabaObject::SetupWord_Rule_Activators(TArray<FHitResult> AllDirectionResluts)
{
	if (AllDirectionResluts.Num() > 0)
	{
		for (int32 index = 0; index < 4; index++)//four directions
		{
			auto& DirectionResult = AllDirectionResluts[index];

			if (DirectionResult.bBlockingHit)
			{
				ABaseBabaObject* HitObject = Cast<ABaseBabaObject>(DirectionResult.GetActor());

				if (HitObject->ObjectType == EGameObjectType::wordObject || HitObject->ObjectType == EGameObjectType::ruleObject)
				{

					HitObject->MyActivators.AddUnique(this);
				}
			}

		}

	}

	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "PASSED EMPTY REFERENCE!!");
		}
	}
}

//Deprecated
void ABaseBabaObject::GenerateVisualsByDrag(bool isDragged)
{

	FHitResult RightCheckResults;
	FHitResult LeftheckResults;
	FHitResult UpCheckResults;
	FHitResult DownCheckResults;

	

	bool HasUp = TraceInDirection(FVector(), FVector(0.0f, -1.0f, 0.0f), UpCheckResults);
	bool HasDown = TraceInDirection(FVector(), FVector(0.0f, 1.0f, 0.0f), DownCheckResults);

	bool HasRight = TraceInDirection(FVector(), FVector(1.0f, 0.0f, 0.0f), RightCheckResults);
	bool HasLeft = TraceInDirection(FVector(), FVector(-1.0f, 0.0f, 0.0f), LeftheckResults);

	bool Cross = HasUp && HasDown && HasRight && HasLeft;

	bool FullPeice = !HasUp && !HasDown && !HasRight && !HasLeft;
	bool UpperCornerRight = HasLeft && HasDown && !HasUp && !HasRight;

	bool UpperCornerLeft = HasRight && HasDown && !HasLeft && !HasUp;
	bool LowerCornerRight = HasUp && HasLeft && !HasRight && !HasDown;

	bool LowerCornerLeft = HasRight && HasUp && !HasDown && !HasLeft;

	bool CrossConnect = HasRight && HasLeft && HasUp && !HasDown;
	bool HMidNormal = HasRight && HasLeft && !HasUp && !HasDown;

	bool VMidNormal = HasUp && HasDown && !HasRight && !HasLeft;
	bool HMidConnectUP = HasLeft && HasRight && HasUp && !HasDown;

	bool HMidConnectDown = HasLeft && HasRight&& !HasUp && HasDown;
	bool VMidConnectRight = HasUp && HasDown && HasLeft && !HasRight;

	bool VMidConnectLeft = HasUp && HasDown && HasRight && !HasLeft;
	bool HEndRight = HasLeft && !HasRight && !HasUp && !HasDown;

	bool HEndLeft = HasRight && !HasLeft && !HasUp && !HasDown;
	bool VEndUP = HasDown && !HasUp && !HasLeft && !HasRight;

	bool VEndDown = HasUp && !HasDown && !HasLeft && !HasRight;

	TArray<bool> SidesStates;

	TArray<FHitResult> SideCheckResults;

	SideCheckResults.Add(UpCheckResults);
	SideCheckResults.Add(DownCheckResults);
	SideCheckResults.Add(RightCheckResults);
	SideCheckResults.Add(LeftheckResults);

	SidesStates.Add(FullPeice);
	SidesStates.Add(UpperCornerRight);
	SidesStates.Add(UpperCornerLeft);
	SidesStates.Add(LowerCornerRight);
	SidesStates.Add(LowerCornerLeft);
	SidesStates.Add(Cross);
	SidesStates.Add(CrossConnect);
	SidesStates.Add(HMidNormal);
	SidesStates.Add(VMidNormal);
	SidesStates.Add(HMidConnectUP);
	SidesStates.Add(HMidConnectDown);
	SidesStates.Add(VMidConnectRight);
	SidesStates.Add(VMidConnectLeft);
	SidesStates.Add(HEndRight);
	SidesStates.Add(HEndLeft);
	SidesStates.Add(VEndUP);
	SidesStates.Add(VEndDown);

//change my visuals after drag
	for (auto& State : SidesStates)
	{
		if (State)
		{
			int32 _Index = SidesStates.Find(State);
			WallType = EWallType(_Index + 1);
			SetupVisuals();
			break;
		}
	}

// tell side tiles about my drag so they can react properly
	if (isDragged == true)
	{
		for (int32 index = 0; index < 4; index++)
		{
			auto& itr = SideCheckResults[index];

			ABaseBabaObject* SideTile = Cast<ABaseBabaObject>(itr.GetActor());
			if (SideTile)
			{
				SideTile->TriggerNearTiles(this);
			}
			else
			{

			}

		}
	}
	
	
}

//bool ABaseBabaObject::TestingPins(bool A, bool B)
//{
//	return A && B;
//}

void ABaseBabaObject::OnObjectDragged()
{

	TArray<ABaseBabaObject*> SurroundingTiles = ConstructWallBySurrounding();
	
	for (auto& Tile : SurroundingTiles)
	{
		int32 _index = SurroundingTiles.Find(Tile);

		if (Tile && Tile->WallType != EWallType::None)
		{
			Tile->ConstructWallBySurrounding();
		}

		else
		{
			if (NearTiles.Num() > 0)
			{
				for (auto& itr : NearTiles)
				{
					if (itr && itr->WallType != EWallType::None) itr->ConstructWallBySurrounding();
				}
			}
		}
		
	}
	NearTiles = SurroundingTiles;
}

TArray<ABaseBabaObject*> ABaseBabaObject::ConstructWallBySurrounding()
{

	TArray<bool> SidesStates;
	TArray<ABaseBabaObject*> SurroundingTiles = GetSurroundingTiles(SidesStates);

	for (auto& State : SidesStates)
	{
		int32 _Index = SidesStates.Find(State);
		if (State)
		{
			
			WallType = EWallType(_Index + 1);
			SetupVisuals();
			break;
		}
		
	}
	Cast
	return SurroundingTiles;
}


void ABaseBabaObject::TriggerNearTiles(ABaseBabaObject* Instegator)
{
	// set my visuals by checking near tiles as if i was dragged but the drag is false so it wont give stack over flow
	GenerateVisualsByDrag(false); 

	/*FHitResult RightCheckResults;
	FHitResult LeftheckResults;
	FHitResult UpCheckResults;
	FHitResult DownCheckResults;

	TraceInDirection(FVector(), FVector(0.0f, -1.0f, 0.0f), UpCheckResults);
	TraceInDirection(FVector(), FVector(0.0f, 1.0f, 0.0f), DownCheckResults);

	TraceInDirection(FVector(), FVector(1.0f, 0.0f, 0.0f), RightCheckResults);
	TraceInDirection(FVector(), FVector(-1.0f, 0.0f, 0.0f), LeftheckResults);

	TArray<FHitResult> SideCheckResults;

	SideCheckResults.Add(UpCheckResults);
	SideCheckResults.Add(DownCheckResults);
	SideCheckResults.Add(RightCheckResults);
	SideCheckResults.Add(LeftheckResults);

	for (int32 index = 0; index < 4; index++)
	{
		auto& itr = SideCheckResults[index];

		ABaseBabaObject* SideTile = Cast<ABaseBabaObject>(itr.GetActor());
		if (SideTile)
		{
			if(SideTile != Instegator)
			   SideTile->TriggerNearTiles(this);
			

			if (NearTiles.IsValidIndex(index) && SideTile != Instegator)
			{
				if (SideTile != NearTiles[index])
				{

					NearTiles[index]->TriggerNearTiles(this);
					NearTiles[index] = SideTile;

				}

			}

			else
			{
				NearTiles.AddUnique(SideTile);
			}
		}

		else
		{
			if (NearTiles.IsValidIndex(index) && SideTile != Instegator)
			{
				NearTiles[index]->TriggerNearTiles(this);
				NearTiles.RemoveAt(index);
			}
		}
	}*/
}


TArray<ABaseBabaObject*> ABaseBabaObject::GetSurroundingTiles(TArray<bool>& SidesStates)
{
	//ForceNearToReConstrcut();
	SidesStates.Empty();

	FHitResult RightCheckResults;
	FHitResult LeftheckResults;
	FHitResult UpCheckResults;
	FHitResult DownCheckResults;

	bool HasUp = TraceInDirection(FVector(), FVector(0.0f, -1.0f, 0.0f), UpCheckResults);
	bool HasDown = TraceInDirection(FVector(), FVector(0.0f, 1.0f, 0.0f), DownCheckResults);

	bool HasRight = TraceInDirection(FVector(), FVector(1.0f, 0.0f, 0.0f), RightCheckResults);
	bool HasLeft = TraceInDirection(FVector(), FVector(-1.0f, 0.0f, 0.0f), LeftheckResults);

	bool Cross = HasUp && HasDown && HasRight && HasLeft;

	bool FullPeice = !HasUp && !HasDown && !HasRight && !HasLeft;
	bool UpperCornerRight = HasLeft && HasDown && !HasUp && !HasRight;

	bool UpperCornerLeft = HasRight && HasDown && !HasLeft && !HasUp;
	bool LowerCornerRight = HasUp && HasLeft && !HasRight && !HasDown;

	bool LowerCornerLeft = HasRight && HasUp && !HasDown && !HasLeft;

	bool CrossConnect = HasRight && HasLeft && HasUp && !HasDown;
	bool HMidNormal = HasRight && HasLeft && !HasUp && !HasDown;

	bool VMidNormal = HasUp && HasDown && !HasRight && !HasLeft;
	bool HMidConnectUP = HasLeft && HasRight && HasUp && !HasDown;

	bool HMidConnectDown = HasLeft && HasRight && !HasUp && HasDown;
	bool VMidConnectRight = HasUp && HasDown && HasLeft && !HasRight;

	bool VMidConnectLeft = HasUp && HasDown && HasRight && !HasLeft;
	bool HEndRight = HasLeft && !HasRight && !HasUp && !HasDown;

	bool HEndLeft = HasRight && !HasLeft && !HasUp && !HasDown;
	bool VEndUP = HasDown && !HasUp && !HasLeft && !HasRight;

	bool VEndDown = HasUp && !HasDown && !HasLeft && !HasRight;

	
	TArray<FHitResult> SideCheckResults;

	SideCheckResults.Add(UpCheckResults);
	SideCheckResults.Add(DownCheckResults);
	SideCheckResults.Add(RightCheckResults);
	SideCheckResults.Add(LeftheckResults);

	SidesStates.Add(FullPeice);
	SidesStates.Add(UpperCornerRight);
	SidesStates.Add(UpperCornerLeft);
	SidesStates.Add(LowerCornerRight);
	SidesStates.Add(LowerCornerLeft);
	SidesStates.Add(Cross);
	SidesStates.Add(CrossConnect);
	SidesStates.Add(HMidNormal);
	SidesStates.Add(VMidNormal);
	SidesStates.Add(HMidConnectUP);
	SidesStates.Add(HMidConnectDown);
	SidesStates.Add(VMidConnectRight);
	SidesStates.Add(VMidConnectLeft);
	SidesStates.Add(HEndRight);
	SidesStates.Add(HEndLeft);
	SidesStates.Add(VEndUP);
	SidesStates.Add(VEndDown);

	TArray<ABaseBabaObject*> TempSideTiles;
	for (auto& itr : SideCheckResults)
	{

		   TempSideTiles.Add(Cast<ABaseBabaObject>(itr.GetActor()));
		
	}

	return TempSideTiles;
}


void ABaseBabaObject::AmDeleted(ABaseBabaObject* DeletedTile)
{
	GenerateVisualsByDrag(false);
	NearTiles.Remove(DeletedTile);
}

void ABaseBabaObject::ReactToMovementVisually()
{
	if (WallType != EWallType::None)
	{
		OnObjectDragged();
	}
}

void ABaseBabaObject::AddNoneWallObjects(TArray<AActor*>& inList)
{

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBabaObject::StaticClass(), AllActors);

	for (auto& _Object : AllActors)
	{
		ABaseBabaObject* BabaObject = Cast<ABaseBabaObject>(_Object);

		if (BabaObject->WallType == EWallType::None)
		{
			inList.Add(BabaObject);
		}
	}

}


ABaseBabaObject* ABaseBabaObject::GetOverlappedTile()
{
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.AddUnique(this);

	FVector Start = GetActorLocation();
	FVector End =  Start;

	FHitResult HitResults;

	ETraceTypeQuery TraceQuery = UEngineTypes::ConvertToTraceType(ECC_Visibility);

	bool hit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, 5.0f, TraceQuery, true, ActorToIgnore, DebugType,
		HitResults, true, FColor::Red, FColor::Green, 2.0f);

	ABaseBabaObject* OverlappedObject = Cast<ABaseBabaObject>(HitResults.GetActor());

	return OverlappedObject;
}