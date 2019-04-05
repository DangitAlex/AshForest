// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetableInterface.h"
#include "AshForestTrigger.generated.h"

UENUM(BlueprintType)
namespace EAshForestTriggerType
{
	enum Type
	{
		EAshTrigger_ON_DASH				UMETA(DisplayName = "Triggered By Dash"),
		EAshTrigger_ON_ACTOR_DESTROY	UMETA(DisplayName = "Triggered By Actor Destruction"),
		EAshTrigger_MAX					UMETA(Hidden)
	};
}

UCLASS()
class ASHFOREST_API AAshForestTrigger : public AActor, public ITargetableInterface
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Lock-On")
		USceneComponent* TargetableComp;

public:	
	// Sets default values for this actor's properties
	AAshForestTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lock On")
		FName TargetableComponentName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trigger")
		TEnumAsByte<EAshForestTriggerType::Type> TriggerType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trigger")
		bool bDamagedDisablesTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
		TArray<AActor*> TriggeredByActorsDestruction;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Trigger")
		bool bTriggerEnabled;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Trigger")
		TArray<AActor*> CurrentTriggeredByActors;

	UFUNCTION(BlueprintCallable, Category = "Trigger")
		void OnActorDestruction(AActor* DestroyedActor);

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
		TArray<AActor*> TriggeredActivatesActors;

	virtual bool GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps) override;
	virtual bool CanBeTargeted_Implementation(const AActor* ByActor) override;
	virtual bool CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual bool IgnoresCollisionWithDamager_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent) override;
	virtual void TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent) override;

};
