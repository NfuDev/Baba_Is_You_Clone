// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputsListenerPawn.h"
#include "PaperFlipbookComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BaseBabaObject.generated.h"

class ABabaPC;

UENUM(BlueprintType)
enum class EGameObjectType : uint8
{
	gameObject,
	ruleObject,
	wordObject,
	activatorObject
};

UENUM(BlueprintType)
enum class EBabaRules : uint8//add more rules here if needed, the piorities are taken from the order of the enum entries
{
	you,//will make the object consume inputs for movements and also accept win and kill events 
	Stop,//makes the object stop the player from moving
	Push,//push will make the object moves if the player pushed it, this will override the stop if the two rules are active for the same object
	open,// this will destroy the object when it overlap with the player and it will override the push and stop
	kill,//should be used with all kill objetcs such as water sink or crap kills and so on
	win,// player will win if overlaped with an object that has this rule activated

};
//Deprecated enum , we are now using tags for easier way to add more objects types 
UENUM(BlueprintType)//only check vs rules , but objects should have name tag to distenguish them
enum class EBabaWord : uint8
{
	baba,
	wall,
	rock,
	water,
	lava
};

UENUM(BlueprintType)
enum class EWallType : uint8
{
	None,//for all none wall objects 
	FullPeice,
	UpperCornerRight,
	UpperCornerLeft,
	LowerCornerRight,
	LowerCornerLeft,
	Cross,
	CrossConnect,
	HMidNormal,
	VMidNormal,
	HMidConnectUp,
	HMidConnectDown,
	VMidConnectRight,
	VMidConnectLeft,
	HEndRight,
	HEndLeft,
	VEndUP,
	VEndDown

};

UENUM(BlueprintType)
enum class EAnimationType : uint8
{
	PX UMETA(DisplayName = "Positive X Direction"),
	NX UMETA(DisplayName = "Negative X Direction"),
	PY UMETA(DisplayName = "Positive Y Direction"),
	NY UMETA(DisplayName = "Negative Y Direction")
};

USTRUCT(BlueprintType)
struct FAnimationsList
{
	GENERATED_BODY();

public:
	UPROPERTY(EditAnywhere, Category = "Animations Struct")
		TArray<UPaperFlipbook*> Animations;
};

USTRUCT(BlueprintType)
struct FObjectState
{
	GENERATED_BODY();

public:
	UPROPERTY(EditAnywhere, Category = "State Struct")
		FVector Location;

	UPROPERTY(EditAnywhere, Category = "State Struct")
		UPaperFlipbook* Animation;

	UPROPERTY(EditAnywhere, Category = "State Struct")
		FVector Scale;//works as animation direction . scalling negativly will flip the animation 

	UPROPERTY(EditAnywhere, Category = "State Struct")
		FName ID;
};


USTRUCT(BlueprintType)
struct FActivationRecord
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, Category = "Activation Struct")
		FName ID;

	UPROPERTY(EditAnywhere, Category = "Activation Struct")
		EBabaRules Rule;

	UPROPERTY(EditAnywhere, Category = "Activation Struct")
		TArray<AActor*> WordRule;

//to prevent multiple calls for activation, we get the word and the rule from the activation so if they are found then dont activate next call
	bool CanActivate(FHitResult& wordhit, FHitResult& rulehit)
	{
		
		return !(WordRule.Contains(wordhit.GetActor()) && WordRule.Contains(rulehit.GetActor()));
	}

	bool IsActive() { return ID != FName(); }

	void Deactivate()
	{
		ID = FName();
		WordRule.Empty();
	}
};
UCLASS()
class BABA_IS_YOU_API ABaseBabaObject : public AActor
{
	GENERATED_BODY()

public:

	UFUNCTION(CallInEditor, Category = "Baba Settings")
		void SetupVisuals();

	/*the gameplay type of this object*/
	UPROPERTY(EditAnywhere, Category = "Baba Settings")
		EGameObjectType ObjectType;

	/*the effect of this object as a rule when get activated by an activator object*/
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ObjectType == EGameObjectType::ruleObject", EditConditionHides), Category = "Baba Settings")
		EBabaRules Rule;

	/*defines the wall flipbook*/
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ObjectType == EGameObjectType::gameObject", EditConditionHides), Category = "Baba Settings")
		EWallType WallType;


	/*List of walls Visuals*/
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ObjectType == EGameObjectType::gameObject", EditConditionHides), Category = "Baba Settings")
		TArray<UPaperFlipbook*> WallTypeVisuals;

	/*animations list if the object used for main player*/
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ObjectType == EGameObjectType::gameObject", EditConditionHides), Category = "Baba Settings")
	    TMap<EAnimationType, FAnimationsList> Animations;

	/*the baba object that this word represents*/
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ObjectType == EGameObjectType::wordObject", EditConditionHides), Category = "Baba Settings")
		TSubclassOf<ABaseBabaObject> RepresentedObject;

	/*this represents the object name like baba and wall and other stuff, only taken in consideration if the object is word type
	then it can be used with rules , other wise it will only be used to find the object to affet it with the rule that currently
	got activated*/
	UPROPERTY(EditAnywhere, Category = "Baba Settings")
		FName ObjectID;

	FName DefaultID;

	/*how far we trace for near by objects in grid*/
	UPROPERTY(EditAnywhere, Category = "Baba Settings")
	    float TraceDistance;

	UPROPERTY(EditAnywhere, Category = "Baba Settings")
		float GridSize;

	UPROPERTY(EditAnywhere, Category = "Baba Settings")
		TEnumAsByte<EDrawDebugTrace::Type> DebugType;


	UPROPERTY(VisibleAnywhere, Category = "Baba Visuals")
		UPaperFlipbookComponent* BabaFlipbook;

	ABabaPC* BabaPC;

	AInputsListenerPawn* inputListenerPawn;

	int32 animationIndex;
	
	TArray<FObjectState> ObjectStateRecords;

	FActivationRecord HLastActivatedMap;

	FActivationRecord VLastActivatedMap;
	
public:	
	// Sets default values for this actor's properties
	ABaseBabaObject();
	~ABaseBabaObject();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void Move(FVector Direction);

	bool ApplyMovement(FVector Direction, const TArray<EBabaRules> ObstacleAppliedRules, FHitResult ObstacleFound);

	bool TraceInDirection(FVector Direction, FHitResult& TraceResults);

	bool TraceInDirection(FVector Location, FVector Direction, FHitResult& TraceResults);

	bool CheckIfRuleApplied(EBabaRules RuleToCheck);

	void SetupAnimation(FVector Direction);

	bool TryToMove(FVector Direction);

	void ProcessOverLap();

	UFUNCTION()
	void RecordObjectState();
	
	UFUNCTION()
	void RestoreObjectState();

	void ReactToPush(FVector Direction);

	//used if the object was an activator only
	void TryToActivate(FHitResult RightHit, FHitResult LeftHit, FHitResult UPHit, FHitResult DownHit);

	bool TryToActivateAxis(FHitResult WordHit, FHitResult RuleHit, FActivationRecord& LastActivatedMap);


	UPaperFlipbook* Get_ID_Visuals(FName ID);

	void SetupWord_Rule_Activators(TArray<FHitResult> AllDirectionResluts);
	TArray<ABaseBabaObject*> MyActivators;//this is for rules and words only use , when they pushed they tell the activator that they are not
	//available now


	//###### WALLS CONSTRCUTION BY DRAG LOGICS ####################

	UFUNCTION(BlueprintCallable, Category = "Level Construction Logics")
		void GenerateVisualsByDrag(bool isDragged);

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (CommutativeAssociativeBinaryOperator = "false"), Category = "Level Construction Logics")
		static FORCEINLINE	bool TestingPins(bool A, bool B) { return A && B; };

	UFUNCTION(BlueprintCallable, Category = "Level Construction Logics")
		void OnObjectDragged();

	TArray<ABaseBabaObject*> ConstructWallBySurrounding();

	void TriggerNearTiles(ABaseBabaObject* Instegator);

	ABaseBabaObject* GetOverlappedTile();

	TArray<ABaseBabaObject*> GetSurroundingTiles(TArray<bool>& SidesStates);

	TArray<ABaseBabaObject*> NearTiles;
	
	bool bProcessedVisuals;

	void AmDeleted(ABaseBabaObject* DeletedTile);

	void ReactToMovementVisually();

	void AddNoneWallObjects(TArray<AActor*>& inList);
};
