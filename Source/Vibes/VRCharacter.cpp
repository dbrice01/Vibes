// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "HandController.h"
#include "Components/SplineComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		CameraManager = PlayerController->PlayerCameraManager;
	}
	if (PostProcessComponent != nullptr && BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
	}

	if (HandControllerClass == nullptr) return;
	LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftController != nullptr)
	{
		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->SetOwner(this);
		LeftController->SetHand(EControllerHand::Left);
	}

	RightController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightController != nullptr)
	{
		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->SetOwner(this);
		RightController->SetHand(EControllerHand::Right);
	}
	LeftController->PairController(RightController);
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	MoveDestinationMarker();
	UpdateBlinkers();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (PlayerInputComponent == nullptr) return;
	PlayerInputComponent->BindAxis("MoveForward", this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Rotate", this, &AVRCharacter::RotateAround);
	PlayerInputComponent->BindAction("Teleport", IE_Pressed, this, &AVRCharacter::BeginTeleport);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookRight", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAction("GripLeft", IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction("GripLeft", IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction("GripRight", IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction("GripRight", IE_Released, this, &AVRCharacter::ReleaseRight);

}

void AVRCharacter::MoveForward(float InputValue)
{
	AddMovementInput(Camera->GetForwardVector(), InputValue);
}

void AVRCharacter::MoveRight(float InputValue)
{
	AddMovementInput(Camera->GetRightVector(), InputValue);
}

void AVRCharacter::RotateAround(float InputValue)
{
	AddControllerYawInput(InputValue);
}

void AVRCharacter::BeginTeleport()
{
	if (DestinationMarker == nullptr || CameraManager == nullptr) return;
	CameraManager->StartCameraFade(0, 1, TeleportDelay, FColor::Black, false, true);
	GetWorld()->GetTimerManager().SetTimer(TeleportDelayTimer, this, &AVRCharacter::FinishTeleport, TeleportDelay);
}

void AVRCharacter::FinishTeleport()
{
	if (DestinationMarker == nullptr || CameraManager == nullptr) return;
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);
	CameraManager->StartCameraFade(1, 0, TeleportDelay, FColor::Black, false, true);
}

void AVRCharacter::MoveDestinationMarker()
{
	FVector NavLocation;
	TArray <FVector> Path;
	if (FindTeleportTarget(Path, NavLocation))
	{
		DestinationMarker->SetWorldLocation(NavLocation);
		DrawTeleportPath(Path);
		if (!DestinationMarker->IsVisible())DestinationMarker->SetVisibility(true);
	}
	else if (DestinationMarker->IsVisible())
	{
		DestinationMarker->SetVisibility(false);
		TArray <FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);
	}
}

bool AVRCharacter::FindTeleportTarget(TArray<FVector>& OutPath, FVector& OutLocation)
{
	FVector Aim = LeftController->GetActorForwardVector().RotateAngleAxis(30, LeftController->GetActorRightVector());
	FPredictProjectilePathResult ProjectileResult;
	FVector ForwardVelocity = LeftController->GetActorForwardVector() * TeleportParabolaStrength;
	FPredictProjectilePathParams ProjectileParams = FPredictProjectilePathParams(TeleportParabolaRadius, LeftController->GetActorLocation(), ForwardVelocity, 2., ECollisionChannel::ECC_Visibility, this);
	ProjectileParams.bTraceComplex = true;
	bool bLineTraceSuccess = UGameplayStatics::PredictProjectilePath(this, ProjectileParams, ProjectileResult);

	for (FPredictProjectilePathPointData DataPoint : ProjectileResult.PathData)
	{
		OutPath.Add(DataPoint.Location);
	}

	if (!bLineTraceSuccess) return false;

	FNavLocation NavLocation;
	bool bNavMeshTraceSuccess = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(ProjectileResult.HitResult.Location, NavLocation, TeleportProjectionExtent);
	if (!bNavMeshTraceSuccess) return false;
	OutLocation = NavLocation.Location;
	return true;
}

void AVRCharacter::UpdateSpline(const TArray<FVector> Path)
{
	TeleportPath->ClearSplinePoints(false);
	for (int32 i = 0; i < Path.Num(); i++)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);

	}
	TeleportPath->UpdateSpline();
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector> Path)
{
	UpdateSpline(Path);

	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; i++)
	{

		if (i >= TeleportPathMeshPool.Num())
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArcMesh);
			SplineMesh->SetMaterial(0, TeleportArcMaterial);
			SplineMesh->RegisterComponent();
			TeleportPathMeshPool.Add(SplineMesh);
		}

		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);
		FVector StartPos, EndPos, StartTangent, EndTangent;

		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
	}
}

void AVRCharacter::UpdateBlinkers()
{
	if (PostProcessComponent != nullptr && BlinkerMaterialBase != nullptr)
	{

		BlinkerMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Radius"), RadiusVsVelocity->GetFloatValue(GetVelocity().Size()));
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstanceDynamic);
		FVector2D Centre = GetBlinkerCentre();
		BlinkerMaterialInstanceDynamic->SetVectorParameterValue(TEXT("Centre"), FLinearColor(Centre.X, Centre.Y, 0, 0));
	}
}

FVector2D AVRCharacter::GetBlinkerCentre()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero()) return FVector2D(0.5, 0.5);
	FVector WorldLocation;
	if (FVector::DotProduct(MovementDirection, Camera->GetForwardVector()) > 0)
	{
		WorldLocation = Camera->GetComponentLocation() + MovementDirection * 100;
	}
	else
	{
		WorldLocation = Camera->GetComponentLocation() - MovementDirection * 100;
	}
	FVector2D ScreenLocation;
	if (PlayerController == nullptr) return FVector2D(0.5, 0.5);
	if (!PlayerController->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation)) return FVector2D(0.5, 0.5);
	FIntPoint ScreenSize;
	PlayerController->GetViewportSize(ScreenSize.X, ScreenSize.Y);
	ScreenLocation.X /= ScreenSize.X;
	ScreenLocation.Y /= ScreenSize.Y;
	//UE_LOG(LogTemp, Warning, TEXT("Screen location is: %s"), *ScreenLocation.ToString());
	return ScreenLocation;
}

