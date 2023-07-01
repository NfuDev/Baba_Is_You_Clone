// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "C:\Program Files\Epic Games\UE_5.1\Engine\Source\Runtime\Core\Public\Delegates\Delegate.h"
#include "InputsListenerPawn.generated.h"

class UCameraComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputReceived, FVector, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecord);

UCLASS()
class BABA_IS_YOU_API AInputsListenerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AInputsListenerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	FOnInputReceived InputReceivedDelegate;
	FOnRewind RewindDelegate;
	FOnRecord RecordDelegate;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* Camera;

	USceneComponent* CameraRoot;

	bool bGameFinished;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	void MoveRight();
	void MoveLeft();
	void MoveUP();
	void MoveDown();
	void RewindMoves();
};
