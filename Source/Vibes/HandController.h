// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"
#include "HandController.generated.h"

UCLASS()
class VIBES_API AHandController : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHandController();

	void SetHand(EControllerHand Hand) { MotionController->SetTrackingSource(Hand); }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void Grip();

	UFUNCTION()
		void Release();


	UFUNCTION()
		void PairController(AHandController* Controller);

private:
	//callbacks
	UFUNCTION()
		void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
		void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
		bool CanClimb() const;


	// Default Subobject 

	UPROPERTY(VisibleAnywhere)
		UMotionControllerComponent* MotionController = nullptr;

	//Parameters
	UPROPERTY(EditDefaultsOnly)
		UHapticFeedbackEffect_Base* HapticEffect = nullptr;

	//State
	bool bCanClimb = false;
	bool bIsClimbing = false;
	FVector ClimbStartLocation;

	AHandController* OtherController;
};
