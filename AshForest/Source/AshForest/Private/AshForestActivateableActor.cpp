// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestActivateableActor.h"

// Sets default values
AAshForestActivateableActor::AAshForestActivateableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateOptionalDefaultSubobject<USceneComponent>("RootComponent");
	if (RootComp) RootComponent = RootComp;
}

// Called when the game starts or when spawned
void AAshForestActivateableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

bool AAshForestActivateableActor::AllowsActivationState_Implementation(const AActor* ByActivator, const bool NewActivationState)
{
	return ByActivator != NULL;
}

void AAshForestActivateableActor::Activate_Implementation(const AActor* Activator)
{

}

void AAshForestActivateableActor::Deactivate_Implementation(const AActor* Deactivator)
{

}

void AAshForestActivateableActor::ToggleActive_Implementation(const AActor* Activator)
{

}

