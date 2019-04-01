// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestCreature.h"

// Sets default values
AAshForestCreature::AAshForestCreature()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TargetableComponentName = "TargetableComp";

	TargetableComp = CreateOptionalDefaultSubobject<USceneComponent>(AAshForestCreature::TargetableComponentName);
	if (TargetableComp)
	{
		TargetableComp->SetupAttachment(GetRootComponent());
	}

	MaxHealth = 100.f;
}

// Called when the game starts or when spawned
void AAshForestCreature::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = MaxHealth;
}

// Called every frame
void AAshForestCreature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AAshForestCreature::GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps)
{
	TargetableComps.Empty();

	if (TargetableComp != NULL)
	{
		TargetableComps.Add(TargetableComp);
		return true;
	}

	return false;
}

bool AAshForestCreature::CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return true;
}

void AAshForestCreature::TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount)
{
	if (!DamageCauser)
		return;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
		Die(DamageCauser);
}

void AAshForestCreature::Die(const AActor* Murderer)
{
	Destroy();
}