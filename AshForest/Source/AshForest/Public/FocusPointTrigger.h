#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "FocusPointTrigger.generated.h"

UCLASS()
class ASHFOREST_API AFocusPointTrigger : public ATriggerBox
{
	GENERATED_BODY()

public:
	AFocusPointTrigger();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	AActor* FocusPointActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float ControlRotation_InterpSpeed;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float FocusTargetCameraArmLength;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float FocusTargetCameraArmLength_InterpSpeed;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	FVector FocusTargetCameraSocketOffset;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float FocusTargetCameraSocketOffset_InterpSpeed;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float FocusTargetFOV;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	float FocusTargetFOV_InterpSpeed;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Focusable")
	bool bOnlyAllowFocusingWithinTrigger;

	UFUNCTION()
	void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);

	UFUNCTION()
	void OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor);
};
