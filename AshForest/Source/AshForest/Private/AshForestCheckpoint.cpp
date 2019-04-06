// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestCheckpoint.h"
#include "AshForestCharacter.h"

AAshForestCheckpoint::AAshForestCheckpoint()
{
	OnActorBeginOverlap.AddDynamic(this, &AAshForestCheckpoint::OnOverlapBegin);
	OnActorEndOverlap.AddDynamic(this, &AAshForestCheckpoint::OnOverlapEnd);

	CheckpointIndex = -1;
}

bool AAshForestCheckpoint::TryUpdatePlayerCheckpoint(class AActor* ForActor)
{
	if (ForActor->IsA(AAshForestCharacter::StaticClass()))
	{
		((AAshForestCharacter*)ForActor)->OnTouchCheckpoint(this);
		return true;
	}

	return false;
}

void AAshForestCheckpoint::OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor)
{
	if (OtherActor && OtherActor != this)
		TryUpdatePlayerCheckpoint(OtherActor);
}

void AAshForestCheckpoint::OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor)
{
	if (OtherActor && OtherActor != this)
		TryUpdatePlayerCheckpoint(OtherActor);
}