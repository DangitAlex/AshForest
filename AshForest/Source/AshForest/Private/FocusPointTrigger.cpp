// Fill out your copyright notice in the Description page of Project Settings.

#include "FocusPointTrigger.h"
#include "AshForestCharacter.h"

AFocusPointTrigger::AFocusPointTrigger()
{
	OnActorBeginOverlap.AddDynamic(this, &AFocusPointTrigger::OnOverlapBegin);
	OnActorEndOverlap.AddDynamic(this, &AFocusPointTrigger::OnOverlapEnd);

	FocusPointActor = nullptr;

	ControlRotation_InterpSpeed = -1.f;
	FocusTargetCameraArmLength = -1.f;
	FocusTargetCameraArmLength_InterpSpeed = -1.f;
	FocusTargetCameraSocketOffset = FVector::ZeroVector;
	FocusTargetCameraSocketOffset_InterpSpeed = -1.f;
	FocusTargetFOV = -1.f;
	FocusTargetFOV_InterpSpeed = -1.f;
}

void AFocusPointTrigger::OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor)
{
	if (OtherActor && OtherActor != this && OtherActor->IsA(AAshForestCharacter::StaticClass()))
		((AAshForestCharacter*)OtherActor)->SetFocusPointTrigger(this);
}

void AFocusPointTrigger::OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor)
{
	if (bOnlyAllowFocusingWithinTrigger && OtherActor && OtherActor != this && OtherActor->IsA(AAshForestCharacter::StaticClass()))
		((AAshForestCharacter*)OtherActor)->SetFocusPointTrigger(nullptr);
}