// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AshForestCreature.generated.h"

UCLASS()
class ASHFOREST_API AAshForestCreature : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(Category = "Lock-On", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* TargetableComp;

public:
	// Sets default values for this character's properties
	AAshForestCreature();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
		FName TargetableComponentName;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	FORCEINLINE class USceneComponent* GetTargetableComponent() const { return TargetableComp; }

	//AS: Should be in a targetable interface so you can target things other than enemies
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lock-On")
		bool GetTargetableComponents(TArray<USceneComponent*> & TargetableComps);

	virtual bool GetTargetableComponentsInternal(TArray<USceneComponent*> & TargetableComps);
};
