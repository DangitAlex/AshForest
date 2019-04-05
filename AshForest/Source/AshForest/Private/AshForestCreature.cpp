// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestCreature.h"
#include "AshForestCharacter.h"

// Sets default values
AAshForestCreature::AAshForestCreature()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TargetableComponentName = "TargetableComp";

	TargetableComp = CreateOptionalDefaultSubobject<USceneComponent>(AAshForestCreature::TargetableComponentName);
	if (TargetableComp)
		TargetableComp->SetupAttachment(GetRootComponent());

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

bool AAshForestCreature::CanBeTargeted_Implementation(const AActor* ByActor)
{
	return ByActor->IsA(AAshForestCharacter::StaticClass());
}

bool AAshForestCreature::CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return DamageCauser->IsA(AAshForestCharacter::StaticClass());
}


bool AAshForestCreature::IgnoresCollisionWithDamager_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return DamageCauser->IsA(AAshForestCharacter::StaticClass());
}

void AAshForestCreature::TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent)
{
	if (!DamageCauser || IsPendingKill())
		return;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
		Die(DamageCauser);
}

void AAshForestCreature::Die(const AActor* Murderer)
{
	OnDeath(Murderer);
	Destroy();
}

void AAshForestCreature::OnDeath_Implementation(const AActor* Murderer)
{
	if (Murderer->IsA(AAshForestCharacter::StaticClass()))
		((AAshForestCharacter*)Murderer)->OnKilledEnemy(this);
}