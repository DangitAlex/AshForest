// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestTrigger.h"
#include "ActivateableInterface.h"

// Sets default values
AAshForestTrigger::AAshForestTrigger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateOptionalDefaultSubobject<USceneComponent>("RootComponent");
	if (RootComp) RootComponent = RootComp;

	TargetableComponentName = "TargetableComp";

	TargetableComp = CreateOptionalDefaultSubobject<USceneComponent>(AAshForestTrigger::TargetableComponentName);
	if (TargetableComp)
		TargetableComp->SetupAttachment(GetRootComponent());

	bDamagedDisablesTrigger = true;
	TriggerType = EAshForestTriggerType::EAshTrigger_ON_DASH;
}

// Called when the game starts or when spawned
void AAshForestTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	bTriggerEnabled = true;

	if (TriggeredActivatesActors.Num() <= 0)
		return;

	if (TriggeredByActorsDestruction.Num())
	{
		CurrentTriggeredByActors = TriggeredByActorsDestruction;

		for (auto currActor : TriggeredByActorsDestruction)
		{
			currActor->OnDestroyed.AddDynamic(this, &AAshForestTrigger::OnActorDestruction);
		}
	}
}

void AAshForestTrigger::OnActorDestruction(AActor* DestroyedActor)
{
	if (!DestroyedActor || TriggeredActivatesActors.Num() <= 0)
		return;

	CurrentTriggeredByActors.Remove(DestroyedActor);

	if (CurrentTriggeredByActors.Num() > 0)
		return;

	for (auto currActor : TriggeredActivatesActors)
	{
		if (currActor && currActor->GetClass()->ImplementsInterface(UActivateableInterface::StaticClass()) && IActivateableInterface::Execute_AllowsActivationState(currActor, this, true))
			IActivateableInterface::Execute_Activate(currActor, this);
	}
}

bool AAshForestTrigger::GetTargetableComponents_Implementation(TArray<USceneComponent*> & TargetableComps)
{
	TargetableComps.Empty();

	if (TargetableComp != NULL)
	{
		TargetableComps.Add(TargetableComp);
		return true;
	}

	return false;
}

bool AAshForestTrigger::CanBeTargeted_Implementation(const AActor* ByActor)
{
	return bTriggerEnabled && TriggerType == EAshForestTriggerType::EAshTrigger_ON_DASH;
}

bool AAshForestTrigger::CanBeDamaged_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return bTriggerEnabled && TriggerType == EAshForestTriggerType::EAshTrigger_ON_DASH;
}

bool AAshForestTrigger::IgnoresCollisionWithDamager_Implementation(const AActor* DamageCauser, const FHitResult & DamageHitEvent)
{
	return false;
}

void AAshForestTrigger::TakeDamage_Implementation(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent)
{
	if (!DamageCauser || TriggerType != EAshForestTriggerType::EAshTrigger_ON_DASH)
		return;

	for (auto currActor : TriggeredActivatesActors)
	{
		if (!currActor || !currActor->GetClass()->ImplementsInterface(UActivateableInterface::StaticClass()) || !IActivateableInterface::Execute_AllowsActivationState(currActor, this, true))
			continue;

		IActivateableInterface::Execute_Activate(currActor, this);
	}

	if (bDamagedDisablesTrigger)
		bTriggerEnabled = false;
}
