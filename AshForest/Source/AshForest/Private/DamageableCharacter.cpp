// Fill out your copyright notice in the Description page of Project Settings.

#include "DamageableCharacter.h"

// Sets default values
ADamageableCharacter::ADamageableCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TargetableComponentName = "TargetableComp";

	TargetableComp = CreateOptionalDefaultSubobject<USceneComponent>(ADamageableCharacter::TargetableComponentName);
	if (TargetableComp)
		TargetableComp->SetupAttachment(GetRootComponent());

	MaxHealth = 100.f;
}

// Called when the game starts or when spawned
void ADamageableCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = MaxHealth;
}

void ADamageableCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	TargetableDie(NULL);
}

bool ADamageableCharacter::GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps)
{
	TargetableComps.Empty();

	if (TargetableComp != NULL)
	{
		TargetableComps.Add(TargetableComp);
		return true;
	}

	return false;
}

bool ADamageableCharacter::CanBeTargeted_Implementation(const AActor* ByActor)
{
	return true;
}

bool ADamageableCharacter::CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return true;
}


bool ADamageableCharacter::IgnoresCollisionWithDamager_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return false;
}

void ADamageableCharacter::TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent)
{
	if (!DamageCauser || IsPendingKill())
		return;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
		TargetableDie(DamageCauser);
}

void ADamageableCharacter::TargetableDie(const AActor* Murderer)
{
	ITargetableInterface::Execute_OnTargetableDeath(this, Murderer);
	Destroy();
}
