// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestCreature.h"

// Sets default values
AAshForestCreature::AAshForestCreature()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAshForestCreature::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAshForestCreature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
//void AAshForestCreature::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//}

