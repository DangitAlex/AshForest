// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageableCharacter.h"
#include "AIController.h"
#include "AshForestProjectile.h"
#include "AshForestCreature.generated.h"

UCLASS()
class ASHFOREST_API AAshForestCreature : public ADamageableCharacter
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
		TSubclassOf<AAshForestProjectile> AttackProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
		float AttackInterval_MIN;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
		float AttackInterval_MAX;

protected:

	UPROPERTY(EditAnywhere, Category = "Combat")
		bool bAllowAttacking;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
		float CurrentAttackInterval;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Combat")
		float LastAttackTime;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
		AAshForestProjectile* LastFiredProjectile;

public:
	// Sets default values for this character's properties
	AAshForestCreature();

protected:
	// Called when the game starts or when spawned

public:	

	virtual bool CanBeTargeted_Implementation(const AActor* ByActor) override;
	virtual bool CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual void OnTargetableDeath_Implementation(const AActor* Murderer) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
		bool CanAttackTarget(const AActor* ForTarget);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
		FTransform GetAttackOrigin(const AActor* ForTarget);

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void AttackTarget(const AActor* ForTarget);
};
