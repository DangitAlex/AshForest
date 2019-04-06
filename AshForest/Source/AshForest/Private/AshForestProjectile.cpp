// Fill out your copyright notice in the Description page of Project Settings.

#include "AshForestProjectile.h"
#include "Components/CapsuleComponent.h"
#include "TargetableInterface.h"
#include "Kismet/GameplayStatics.h"
#include "AshForestCharacter.h"

// Sets default values
AAshForestProjectile::AAshForestProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>("CollisionComp");
	if (CollisionComp)
	{
		RootComponent = CollisionComp;
		CollisionComp->OnComponentHit.AddDynamic(this, &AAshForestProjectile::OnProjectileHit);
	}

	ProjVFX = CreateDefaultSubobject<UParticleSystemComponent>("ProjParticleComp");
	if (ProjVFX) ProjVFX->SetupAttachment(GetRootComponent());

	ProjMoveComp = CreateDefaultSubobject<UProjectileMovementComponent>("ProjMovementComp");

	ProjectileDamage = 20.f;
}

// Called when the game starts or when spawned
void AAshForestProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAshForestProjectile::InitProjectile(APawn* FromInstigator)
{
	check(FromInstigator);

	Instigator = FromInstigator;
}

bool AAshForestProjectile::IgnoreProjectileHit_Implementation(const FHitResult & ForHit)
{
	return ForHit.Actor == NULL || ForHit.Actor == this || ForHit.Actor == Instigator;
}

void AAshForestProjectile::OnProjectileExplode_Implementation(const FHitResult & ExplodeFromHit)
{
	if (ExplosionVFX) UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionVFX, GetActorLocation(), GetActorRotation(), true);
	
	Destroy();
}

void AAshForestProjectile::OnProjectileHit(UPrimitiveComponent * HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	if (auto hitPlayer = Cast<AAshForestCharacter>(OtherActor))
	{
		if (hitPlayer->GetCurrentAshMoveState() == EAshCustomMoveState::EAshMove_DASHING)
		{
			hitPlayer->DeflectProjectile(this);
			return;
		}
	}

	if (IgnoreProjectileHit(Hit))
		return;

	if (OtherActor->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass()))
		ITargetableInterface::Execute_TakeDamage(OtherActor, this, ProjectileDamage, Hit);

	OnProjectileExplode(Hit);
}