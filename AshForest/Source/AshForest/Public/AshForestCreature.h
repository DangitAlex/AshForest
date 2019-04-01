// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TargetableInterface.h"
#include "AIController.h"
#include "AshForestCreature.generated.h"

UCLASS()
class ASHFOREST_API AAshForestCreature : public ACharacter, public ITargetableInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Lock-On")
		USceneComponent* TargetableComp;

public:
	// Sets default values for this character's properties
	AAshForestCreature();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Lock On")
		FName TargetableComponentName;

	UPROPERTY(EditAnywhere, Category = "Health")
		float MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
		float CurrentHealth;

	UFUNCTION(BlueprintCallable, Category = "Health")
		void Die(const AActor* Murderer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual bool GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps) override;
	virtual bool CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual void TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount) override;
};
