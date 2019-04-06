// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "AshForestCheckpoint.generated.h"

/**
 * 
 */
UCLASS()
class ASHFOREST_API AAshForestCheckpoint : public ATriggerBox
{
	GENERATED_BODY()

public:
	AAshForestCheckpoint();
	
protected:

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Checkpoint")
		AActor* RespawnPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Checkpoint")
		int32 CheckpointIndex;

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
		bool TryUpdatePlayerCheckpoint(class AActor* ForActor);

public:
	UFUNCTION(BlueprintCallable, Category = "Checkpoint") FORCEINLINE
		int32 GetCheckpointIndex() const { return CheckpointIndex; };

	UFUNCTION(BlueprintCallable, Category = "Checkpoint") FORCEINLINE
		FTransform GetRespawnTransform() const { return RespawnPoint ? RespawnPoint->GetActorTransform() : GetActorTransform(); };

	UFUNCTION()
		void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);

	UFUNCTION()
		void OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor);
};
