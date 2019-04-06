// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TargetableInterface.h"
#include "DamageableCharacter.generated.h"

UCLASS()
class ASHFOREST_API ADamageableCharacter : public ACharacter, public ITargetableInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Lock-On")
		USceneComponent* TargetableComp;

public:
	// Sets default values for this character's properties
	ADamageableCharacter();

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lock On")
		FName TargetableComponentName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health")
		float MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
		float CurrentHealth;

public:	

//AS: ITargetable Interface START ======================================================================================================================
	virtual bool GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps) override;
	virtual bool CanBeTargeted_Implementation(const AActor* ByActor) override;
	virtual bool CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual bool IgnoresCollisionWithDamager_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual void TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent) override;
	virtual void TargetableDie(const AActor* Murderer) override;
//AS: ITargetable Interface END ========================================================================================================================
};
