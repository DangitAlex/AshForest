// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestCreature.h"
#include "AshForestCharacter.h"

// Sets default values
AAshForestCreature::AAshForestCreature()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MaxHealth = 250.f;

	bAllowAttacking = true;

	AttackInterval_MIN = 1.f;
	AttackInterval_MAX = 4.f;
}

bool AAshForestCreature::CanBeTargeted_Implementation(const AActor* ByActor)
{
	return ByActor->IsA(AAshForestCharacter::StaticClass());
}

bool AAshForestCreature::CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return DamageCauser->IsA(AAshForestCharacter::StaticClass());
}

void AAshForestCreature::OnTargetableDeath_Implementation(const AActor* Murderer)
{
	if (Murderer && Murderer->IsA(AAshForestCharacter::StaticClass()))
		((AAshForestCharacter*)Murderer)->OnKilledEnemy(this);
}

bool AAshForestCreature::CanAttackTarget_Implementation(const AActor* ForTarget)
{
	return bAllowAttacking && AttackProjectileClass != NULL && ForTarget != NULL && GetWorld()->TimeSince(LastAttackTime) > CurrentAttackInterval;
}

FTransform AAshForestCreature::GetAttackOrigin_Implementation(const AActor* ForTarget)
{
	const FVector spawnLoc = GetActorLocation();
	const FRotator spawnRot = (ForTarget->GetActorLocation() - spawnLoc).GetSafeNormal().Rotation();

	return FTransform(spawnRot, spawnLoc, FVector(1.f));
}

void AAshForestCreature::AttackTarget(const AActor* ForTarget)
{
	if (AttackProjectileClass == NULL || ForTarget == NULL)
		return;

	LastAttackTime = GetWorld()->GetTimeSeconds();
	CurrentAttackInterval = FMath::RandRange(AttackInterval_MIN, AttackInterval_MAX);
	
	const FTransform spawnTrans = GetAttackOrigin(ForTarget);
	auto temp = GetWorld()->SpawnActorDeferred<AAshForestProjectile>(AttackProjectileClass.Get(), spawnTrans, (AActor*)this, (APawn*)this, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	temp->InitProjectile(this);
	temp->FinishSpawning(spawnTrans, true);

	if (temp->IsValidLowLevel())
	{
		LastFiredProjectile = temp;

		//AS: Reset To fix any directional errors caused by adjusting the spawn location
		const FTransform spawnTrans = GetAttackOrigin(ForTarget);
		LastFiredProjectile->SetActorTransform(spawnTrans);
	}
}