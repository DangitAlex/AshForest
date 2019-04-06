// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AshForestProjectile.generated.h"

class UCapsuleComponent;

UCLASS()
class ASHFOREST_API AAshForestProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(Category = Projectile, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* CollisionComp;

	UPROPERTY(Category = Projectile, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UParticleSystemComponent* ProjVFX;

	UPROPERTY(Category = Projectile, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UProjectileMovementComponent* ProjMoveComp;
	
public:	
	// Sets default values for this actor's properties
	AAshForestProjectile();

	UFUNCTION(BlueprintCallable, Category = Projectile)
		void InitProjectile(APawn* FromInstigator);

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		float ProjectileDamage;

	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		UParticleSystem* ExplosionVFX;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Projectile)
		bool IgnoreProjectileHit(const FHitResult & ForHit);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Projectile)
		void OnProjectileExplode(const FHitResult & ExplodeFromHit);

public:	

	UFUNCTION(BlueprintCallable, Category = Projectile) FORCEINLINE
		UCapsuleComponent* GetCollisionComponent() const { return CollisionComp; };

	UFUNCTION(BlueprintCallable, Category = Projectile) FORCEINLINE
		UProjectileMovementComponent* GetProjectileMovement() const { return ProjMoveComp; };

	UFUNCTION(BlueprintNativeEvent, Category = Projectile)
		void OnDeflected(AActor* DeflectedByActor, const FVector & DeflectedVelocity);

	UFUNCTION()
		void OnProjectileHit(UPrimitiveComponent * HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit);
};
