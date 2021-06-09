// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "Components/SceneComponent.h"
#include "HandController.h"
//#include "Components/StaticMeshComponent.h"
#include "VRCharacter.generated.h"

UCLASS()
class VIBES_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere)
		class UCameraComponent* Camera = nullptr;

	UPROPERTY()
		class AHandController* RightController = nullptr;

	UPROPERTY()
		class AHandController* LeftController = nullptr;

	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AHandController> HandControllerClass = nullptr;

	UPROPERTY(VisibleAnywhere)
		class USceneComponent* VRRoot = nullptr;

	UPROPERTY()
		TArray<class USplineMeshComponent*> TeleportPathMeshPool;

private:
	UFUNCTION()
		void MoveForward(float InputValue);

	UFUNCTION()
		void MoveRight(float InputValue);

	UFUNCTION()
		void GripLeft() { LeftController->Grip(); }

	UFUNCTION()
		void ReleaseLeft() { LeftController->Release(); }

	UFUNCTION()
		void GripRight() { RightController->Grip(); }

	UFUNCTION()
		void ReleaseRight() { RightController->Release(); }

	UFUNCTION()
		void RotateAround(float InputValue);

	UFUNCTION()
		void BeginTeleport();

	UFUNCTION()
		void FinishTeleport();

	UFUNCTION()
		void MoveDestinationMarker();

	UFUNCTION()
		bool FindTeleportTarget(TArray<FVector>& OutPath, FVector& OutLocation);

	UFUNCTION()
		void UpdateSpline(const TArray<FVector> Path);

	UFUNCTION()
		void DrawTeleportPath(const TArray<FVector> Path);

	UFUNCTION()
		void UpdateBlinkers();

	UFUNCTION()
		FVector2D GetBlinkerCentre();

	UPROPERTY(EditAnywhere)
		float MaxTeleportRange = 1000;

	UPROPERTY(EditAnywhere)
		float TeleportDelay = 0.75;

	UPROPERTY(EditAnywhere)
		float TeleportParabolaRadius = 10;

	UPROPERTY(EditAnywhere)
		float TeleportParabolaStrength = 500;

	UPROPERTY(EditAnywhere)
		FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY()
		FTimerHandle TeleportDelayTimer;

	UPROPERTY()
		APlayerController* PlayerController;

	UPROPERTY()
		class USplineComponent* TeleportPath = nullptr;

	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker = nullptr;

	UPROPERTY()
		APlayerCameraManager* CameraManager = nullptr;

	UPROPERTY()
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
		class UMaterialInstanceDynamic* BlinkerMaterialInstanceDynamic;

	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportArcMesh;

	UPROPERTY(EditDefaultsOnly)
		class UMaterialInterface* TeleportArcMaterial;

};
