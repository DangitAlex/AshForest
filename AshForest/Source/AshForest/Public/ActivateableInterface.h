// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ActivateableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UActivateableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class ASHFOREST_API IActivateableInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
		bool AllowsActivationState(const AActor* ByActivator, const bool NewActivationState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
		void Activate(const AActor* Activator);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
		void Deactivate(const AActor* Deactivator);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Activation")
		void ToggleActive(const AActor* Activator);
};
