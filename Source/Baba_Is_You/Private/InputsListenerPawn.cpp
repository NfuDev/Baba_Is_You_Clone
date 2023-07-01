// Fill out your copyright notice in the Description page of Project Settings.


#include "InputsListenerPawn.h"
#include "Engine/Classes/Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

// Sets default values
AInputsListenerPawn::AInputsListenerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	SetRootComponent(CameraRoot);
	Camera->SetupAttachment(CameraRoot);

}

// Called when the game starts or when spawned
void AInputsListenerPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AInputsListenerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AInputsListenerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Right", IE_Pressed, this , &AInputsListenerPawn::MoveRight);
	PlayerInputComponent->BindAction("Left", IE_Pressed, this , &AInputsListenerPawn::MoveLeft);
	PlayerInputComponent->BindAction("UP", IE_Pressed, this , &AInputsListenerPawn::MoveUP);
	PlayerInputComponent->BindAction("Down", IE_Pressed, this , &AInputsListenerPawn::MoveDown);
	PlayerInputComponent->BindAction("Rewind", IE_Pressed, this, &AInputsListenerPawn::RewindMoves);
}

void AInputsListenerPawn::MoveRight()
{

	//InputReceivedDelegate.Execute(GetActorRightVector());
	InputReceivedDelegate.Broadcast(GetActorRightVector());
}

void AInputsListenerPawn::MoveLeft()
{
	//InputReceivedDelegate.Execute(GetActorRightVector() * -1);
	InputReceivedDelegate.Broadcast(GetActorRightVector() * -1);

}

void AInputsListenerPawn::MoveUP()
{
	//InputReceivedDelegate.Execute(GetActorForwardVector());
	InputReceivedDelegate.Broadcast(GetActorForwardVector());
}

void AInputsListenerPawn::MoveDown()
{
	//InputReceivedDelegate.Execute(GetActorForwardVector() * -1);
	InputReceivedDelegate.Broadcast(GetActorForwardVector() * -1);
}

void AInputsListenerPawn::RewindMoves()
{
	RewindDelegate.Broadcast();
}

