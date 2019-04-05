// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActivateableInterface.h"
#include "AshForestActivateableActor.generated.h"

UCLASS()
class ASHFOREST_API AAshForestActivateableActor : public AActor, public IActivateableInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* RootComp;
	
public:	
	// Sets default values for this actor's properties
	AAshForestActivateableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
		bool AllowsActivationState_Implementation(const AActor* ByActivator, const bool NewActivationState);
		void Activate_Implementation(const AActor* Activator);
		void Deactivate_Implementation(const AActor* Deactivator);
		void ToggleActive_Implementation(const AActor* Activator);
};
