// Fill out your copyright notice in the Description page of Project Settings.


#include "TraversalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "MotionWarpingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UTraversalComponent::UTraversalComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

// Called when the game starts
void UTraversalComponent::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void UTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update sliding while traversal state is sliding
	if (TraversalState == ETraversalState::Sliding)
	{
		SlideUpdate();
	}

}

void UTraversalComponent::Initialize(ACharacter* Character)
{
	PlayerCharacter = Character;
	PlayerCharacterMovement = Character->GetCharacterMovement();
	PlayerCapsule = Character->GetCapsuleComponent();

	DefaultGravity = PlayerCharacterMovement->GravityScale;
	DefaultGroundFriction = PlayerCharacterMovement->GroundFriction;
	DefaultBrakingDeceleration = PlayerCharacterMovement->BrakingDecelerationWalking;
}

/* General */

FVector UTraversalComponent::GetCapsuleBaseLocation()
{
	return PlayerCapsule->GetComponentLocation() - (PlayerCapsule->GetUpVector() * PlayerCapsule->GetScaledCapsuleHalfHeight());
}

FVector UTraversalComponent::GetCapsuleLocationFromBaseLocation(FVector BaseLocation, float ZOffset)
{
	DrawDebugSphere(GetWorld(), BaseLocation + FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight() + ZOffset), 10.0f, 12, FColor::Blue, false, 1.0f, 0, 2.0f);
	return BaseLocation + FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight() + ZOffset);
}

bool UTraversalComponent::IsRoomForCapsule(FVector Location)
{
	FVector Start = Location + FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight_WithoutHemisphere());
	FVector End = Location - FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight_WithoutHemisphere());
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, PlayerCapsule->GetScaledCapsuleRadius(), DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	
	return !Hit.bBlockingHit && !Hit.bStartPenetrating;
}

void UTraversalComponent::OnMontageCompleted()
{
	PlayerCharacterMovement->SetMovementMode(MOVE_Walking);
	TraversalState = ETraversalState::None;
}

FIsObjectClimbableOut UTraversalComponent::IsObjectClimbable(float ReachDistance, float MinLedgeHeight, float MaxLedgeHeight)
{
	FVector Start = (GetCapsuleBaseLocation() + PlayerCharacter->GetLastMovementInputVector() * -15.0f) + FVector(0.0f, 0.0f, (MinLedgeHeight + MaxLedgeHeight) / 2);
	FVector End = Start + PlayerCharacter->GetLastMovementInputVector() * ReachDistance;
	float HalfHeight = (MaxLedgeHeight - MinLedgeHeight) / 2;
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, End, 5.0f, HalfHeight, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true);
	FIsObjectClimbableOut Out;
	if (bHit && !Hit.bStartPenetrating && !PlayerCharacterMovement->IsWalkable(Hit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("climbable"));
		Out.bIsNotWalkable = true;
		Out.InitialImpactPoint = Hit.ImpactPoint;
		Out.InitialImpactNormal = Hit.ImpactNormal;
		return Out;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("NOT climbable"));
		Out.bIsNotWalkable = false;
		Out.InitialImpactPoint = FVector(0.0f, 0.0f, 0.0f);
		Out.InitialImpactNormal = FVector(0.0f, 0.0f, 0.0f);
		return Out;
	}
}

FIsSurfaceWalkableOut UTraversalComponent::IsSurfaceWalkable(float MaxLedgeHeight, FVector InitialImpactPoint, FVector InitialImpactNormal)
{
	FVector End = PlayerCharacter->GetLastMovementInputVector() * 10.0f + FVector(InitialImpactPoint.X, InitialImpactPoint.Y, GetCapsuleBaseLocation().Z);
	FVector Start = End + FVector(0.0f, 0.0f, MaxLedgeHeight + 30.0f);
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, 5.0f, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	FIsSurfaceWalkableOut Out;
	if (bHit && PlayerCharacterMovement->IsWalkable(Hit))
	{
		Out.bIsWalkable = true;
		Out.WalkableImpactPoint = Hit.ImpactPoint;
		return Out;
	}
	else
	{
		Out.bIsWalkable = false;
		Out.WalkableImpactPoint = FVector(0.0f, 0.0f, 0.0f);
		return Out;
	}
}

bool UTraversalComponent::IsCapsulePathClear(float Height, FVector EndTargetLocation)
{
	FVector Start = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetActorUpVector() * Height;
	FVector End = GetCapsuleLocationFromBaseLocation(EndTargetLocation, 2.0f);
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;
	
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, End, PlayerCapsule->GetScaledCapsuleRadius(), PlayerCapsule->GetScaledCapsuleHalfHeight(), DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	return !bHit;
}

FAnimationProperties UTraversalComponent::DetermineAnimationProperties(float Height, const TArray<FAnimationPropertySettings>& AnimationPropertySettings)
{
	FAnimationProperties Out;

	for (const FAnimationPropertySettings& PropertySetting : AnimationPropertySettings)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::SanitizeFloat(Height));
		if (IsValid(PropertySetting.Animation) && UKismetMathLibrary::InRange_FloatFloat(Height, PropertySetting.AnimationMinHeight, PropertySetting.AnimationMaxHeight))
		{
			Out.Animation = PropertySetting.Animation;
			Out.AnimationHeightOffset = PropertySetting.AnimationHeightOffset;
			Out.AnimationStartingPosition = UKismetMathLibrary::MapRangeClamped(Height, PropertySetting.InHeightA, PropertySetting.InHeightB, PropertySetting.StartingPositionA, PropertySetting.StartingPositionA);
			Out.AnimationEndBlendTime = PropertySetting.AnimationEndBlendTime;
			return Out;
		}
	}

	Out.Animation = nullptr;
	Out.AnimationHeightOffset = 0.0f;
	Out.AnimationStartingPosition = 0.0f;
	Out.AnimationEndBlendTime = 0.0f;
	return Out;
}


// Vault

bool UTraversalComponent::VaultCheck()
{
	if (TraversalState != ETraversalState::None || PlayerCharacterMovement->IsFalling())
	{
		return false;
	}

	FVector InitialTraceImpactPoint;
	FVector InitialTraceImpactNormal;

	// Trace forward to check if character can't step onto object
	FIsObjectClimbableOut CheckObjectHeightReturnValue = IsObjectClimbable(VaultReachDistance, VaultMinLedgeHeight, VaultMaxLedgeHeight);

	if (CheckObjectHeightReturnValue.bIsNotWalkable)
	{
		InitialTraceImpactPoint = CheckObjectHeightReturnValue.InitialImpactPoint;
		InitialTraceImpactNormal = CheckObjectHeightReturnValue.InitialImpactNormal;
	}
	else
	{
		return false;
	}

	float ApproachAngleDotProduct = UKismetMathLibrary::Dot_VectorVector(InitialTraceImpactNormal, PlayerCharacter->GetActorForwardVector());
	int32 ApproachAngle = UKismetMathLibrary::Round(UKismetMathLibrary::Abs(ApproachAngleDotProduct) * 90.0f);

	// Check if it can start the vault with current approach angle
	if (ApproachAngle <= VaultMaxApproachAngle)
	{
		return false;
	}

	FVector WalkableImpactPoint;
	float VaultHeight;

	// Trace downward from the initial trace's impact point and determine if the hit location is walkable. If it is, set impact point as object start sync point
	FIsSurfaceWalkableOut IsWalkableSurfaceReturnValue = IsSurfaceWalkable(VaultMaxLedgeHeight, InitialTraceImpactPoint, InitialTraceImpactNormal);

	if (IsWalkableSurfaceReturnValue.bIsWalkable)
	{
		WalkableImpactPoint = IsWalkableSurfaceReturnValue.WalkableImpactPoint;
		ObjectStartWarpTarget = WalkableImpactPoint;
		FVector VaultHeightVec = GetCapsuleLocationFromBaseLocation(WalkableImpactPoint, 2.0f) - PlayerCharacter->GetActorLocation();
		VaultHeight = VaultHeightVec.Z;

		// Check if vault height isn't higher than the max vault ledge height
		if (VaultHeight > VaultMaxLedgeHeight)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Check vaulting actor depth and space behind actor.If true, set object end sync point to depth impact point.Find land sync point
	FCanVaultOverDepthOut CanVaultOverDepthOut = CanVaultOverDepth();
	FVector EndTargetLocation;

	if (CanVaultOverDepthOut.bCanVaultOverDepth)
	{
		ObjectEndWarpTarget = CanVaultOverDepthOut.DepthImpactPoint;
		LandWarpTarget = GetVaultLandPoint(ObjectEndWarpTarget);
		EndTargetLocation = FVector(LandWarpTarget.X, LandWarpTarget.Y, LandWarpTarget.Z + VaultHeight);
	}
	else
	{
		return false;
	}

	// Check if nothing is blocking the mantle path
	bool bIsPathClear = IsCapsulePathClear(VaultHeight, EndTargetLocation);

	if (!bIsPathClear)
	{
		return false;
	}

	// Determine correct vault animation properties based on vault height
	FAnimationProperties VaultAnimationProperties = DetermineAnimationProperties(VaultHeight, VaultAnimationPropertySettings);

	VaultStart(VaultAnimationProperties.Animation, VaultAnimationProperties.AnimationEndBlendTime);

	return true;
}

FCanVaultOverDepthOut UTraversalComponent::CanVaultOverDepth()
{
	FVector Start = PlayerCharacter->GetActorLocation();
	FVector End = Start + PlayerCharacter->GetActorForwardVector() * VaultReachDistance;
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	FCanVaultOverDepthOut Out;
	if (bHit)
	{
		FVector ReachImpactPoint = Hit.ImpactPoint;

		Start = ReachImpactPoint + PlayerCharacter->GetActorForwardVector() * VaultMaxDepth;
		End = ReachImpactPoint;

		bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

		if (bHit)
		{
			FVector DepthImpactPoint = Hit.ImpactPoint;
			bool bInRange = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::Vector_Distance(DepthImpactPoint, ReachImpactPoint), VaultMinDepth, VaultMaxDepth);
			FVector Location = DepthImpactPoint + PlayerCharacter->GetActorForwardVector() * (PlayerCapsule->GetScaledCapsuleRadius() + VaultLandDistance);
			bool bCanVaultOverDepth = Hit.Distance > 1 && bInRange && IsRoomForCapsule(Location);

			Out.bCanVaultOverDepth = bCanVaultOverDepth;
			Out.DepthImpactPoint = DepthImpactPoint;
			return Out;
		}
		else
		{
			Out.bCanVaultOverDepth = false;
			Out.DepthImpactPoint = FVector(0.0f, 0.0f, 0.0f);
			return Out;
		}
	}
	else
	{
		Out.bCanVaultOverDepth = false;
		Out.DepthImpactPoint = FVector(0.0f, 0.0f, 0.0f);
		return Out;
	}
}

FVector UTraversalComponent::GetVaultLandPoint(FVector ObjectEndPoint)
{
	FVector Start = ObjectEndPoint + PlayerCharacter->GetActorForwardVector() * VaultLandDistance;
	FVector End = Start - FVector(0.0f, 0.0f, VaultMaxLandVerticalDistance);
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	if (Hit.bBlockingHit)
	{
		return Hit.ImpactPoint;
	}
	else
	{
		return FVector(0.0f, 0.0f, 0.0f);
	}
}

void UTraversalComponent::VaultStart(UAnimMontage* VaultAnimation, float AnimationEndBlendTime)
{
	TraversalState = ETraversalState::Vaulting;

	// Set player movement mode and add warp targets
	PlayerCharacterMovement->SetMovementMode(MOVE_Flying);
	UMotionWarpingComponent* PlayerMotionWarpingComponent = PlayerCharacter->FindComponentByClass<UMotionWarpingComponent>();
	if (IsValid(PlayerMotionWarpingComponent))
	{
		PlayerMotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultObjectStartWarpTargetName, ObjectStartWarpTarget, PlayerCharacter->GetActorRotation());
		PlayerMotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultObjectEndWarpTargetName, ObjectEndWarpTarget, PlayerCharacter->GetActorRotation());
		PlayerMotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultLandWarpTargetName, LandWarpTarget, PlayerCharacter->GetActorRotation());

		float MontageDuration = PlayerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(VaultAnimation, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);
		FTimerHandle MontageCompletedTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(MontageCompletedTimerHandle, this, &UTraversalComponent::OnMontageCompleted, MontageDuration - AnimationEndBlendTime, false);
	}
}


// Mantle

bool UTraversalComponent::MantleCheck()
{
	if (TraversalState != ETraversalState::None)
	{
		return false;
	}

	FVector InitialTraceImpactPoint;
	FVector InitialTraceImpactNormal;

	// Trace forward to check if character can't step onto object
	FIsObjectClimbableOut IsObjectClimbableReturnValue = IsObjectClimbable(MantleReachDistance, MantleMinLedgeHeight, MantleMaxLedgeHeight);

	if (IsObjectClimbableReturnValue.bIsNotWalkable)
	{
		InitialTraceImpactPoint = IsObjectClimbableReturnValue.InitialImpactPoint;
		InitialTraceImpactNormal = IsObjectClimbableReturnValue.InitialImpactNormal;
	}
	else
	{
		return false;
	}

	FVector WalkableImpactPoint;
	float MantleHeight;

	// Trace downward from the initial trace's impact point and determine if the hit location is walkable. If it is, set impact point as object start sync point
	FIsSurfaceWalkableOut IsWalkableSurfaceReturnValue = IsSurfaceWalkable(MantleMaxLedgeHeight, InitialTraceImpactPoint, InitialTraceImpactNormal);

	if (IsWalkableSurfaceReturnValue.bIsWalkable)
	{
		WalkableImpactPoint = IsWalkableSurfaceReturnValue.WalkableImpactPoint;
		ObjectStartWarpTarget = WalkableImpactPoint;
		FVector MantleHeightVec = GetCapsuleLocationFromBaseLocation(WalkableImpactPoint, 2.0f) - PlayerCharacter->GetActorLocation();
		MantleHeight = MantleHeightVec.Z - 2.0f;
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::SanitizeFloat(MantleHeight));

		// Check if mantle height isn't higher than the max mantle ledge height
		if (MantleHeight > MantleMaxLedgeHeight)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Check if nothing is blocking the mantle path
	bool bIsPathClear = IsCapsulePathClear(MantleHeight, WalkableImpactPoint);

	if (!bIsPathClear)
	{
		return false;
	}

	/*UAnimMontage* MantleAnimation;
	float AnimationHeightOffset;
	float AnimationStartingPosition;*/

	// Determine correct mantle animation based on mantle height
	FAnimationProperties MantleAnimationProperties = DetermineAnimationProperties(MantleHeight, MantleAnimationPropertySettings);

	/*MantleAnimation = MantleAnimationProperties.Animation;
	AnimationHeightOffset = MantleAnimationProperties.AnimationHeightOffset;
	AnimationStartingPosition = MantleAnimationProperties.AnimationStartingPosition;*/

	// Start mantle
	MantleStart(MantleAnimationProperties);

	return true;
}

float UTraversalComponent::ApplyMantleHeightOffset(float HeightOffset)
{
	return PlayerCapsule->GetScaledCapsuleHalfHeight() * 2 - HeightOffset;
}

void UTraversalComponent::MantleStart(const FAnimationProperties& AnimationProperties)
{
	TraversalState = ETraversalState::Mantling;

	// Set player movement mode and add warp target
	PlayerCharacterMovement->SetMovementMode(MOVE_Flying);
	UMotionWarpingComponent* PlayerMotionWarpingComponent = PlayerCharacter->FindComponentByClass<UMotionWarpingComponent>();
	if (IsValid(PlayerMotionWarpingComponent))
	{
		FVector TargetLocation = ObjectStartWarpTarget - FVector(0.0f, 0.0f, ApplyMantleHeightOffset(AnimationProperties.AnimationHeightOffset));
		PlayerMotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MantleWarpTargetName, TargetLocation, PlayerCharacter->GetActorRotation());

		float MontageDuration = PlayerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(AnimationProperties.Animation, 1.0f, EMontagePlayReturnType::Duration, AnimationProperties.AnimationStartingPosition, true);
		FTimerHandle MontageCompletedTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(MontageCompletedTimerHandle, this, &UTraversalComponent::OnMontageCompleted, MontageDuration - AnimationProperties.AnimationStartingPosition - AnimationProperties.AnimationEndBlendTime, false);
	}
}


// Slide
//bool UTraversalComponent::CanSlide()
//{
//	return TraversalState == ETraversalState::None && !PlayerCharacterMovement->IsFalling();
//}

// Check if player can slide
bool UTraversalComponent::SlideCheck()
{
	if (TraversalState == ETraversalState::None && !PlayerCharacterMovement->IsFalling())
	{
		SlideStart();
		return true;
	}
	else
	{
		return false;
	}
}

// Start slide
void UTraversalComponent::SlideStart()
{
	TraversalState = ETraversalState::Sliding;
	PlayerCharacterMovement->GroundFriction = SlideGroundFriction;
	PlayerCharacterMovement->BrakingDecelerationWalking = SlideBrakingPower;
}

// Slide update
void UTraversalComponent::SlideUpdate()
{
	FFindFloorResult FloorHit = PlayerCharacterMovement->CurrentFloor;

	if (FloorHit.bBlockingHit)
	{
		FVector Force = CalculateSlideForce(FloorHit.HitResult.ImpactNormal);

		PlayerCharacterMovement->AddForce(Force);

		// Clamp velocity to prevent extreme player speed while sliding
		PlayerCharacterMovement->Velocity = UKismetMathLibrary::ClampVectorSize(PlayerCharacterMovement->Velocity, 0.0, SlideMaxSpeed);
		
		// If player speed gets too low while sliding, stop sliding
		if (PlayerCharacterMovement->Velocity.Size() < SlideMinSpeed)
		{
			SlideStop();
		}
	}
	else
	{
		SlideStop();
	}
}

// Calculate the force applied while sliding
FVector UTraversalComponent::CalculateSlideForce(FVector FloorNormal)
{
	FVector ForwardForce = PlayerCharacter->GetActorForwardVector() * SlidePower;

	FVector FloorCross = UKismetMathLibrary::Cross_VectorVector(FloorNormal, UKismetMathLibrary::Cross_VectorVector(FloorNormal, UKismetMathLibrary::Vector_Up())).GetSafeNormal();
	float FloorDot = UKismetMathLibrary::Dot_VectorVector(PlayerCharacter->GetActorForwardVector(), FloorCross);
	FVector FloorForce;

	if (FloorDot == 0.0f)
	{
		FloorForce = FloorCross * (FloorDot * SlideFloorMultiplier);
	}
	else if (FloorDot < 0.0f)
	{
		FloorForce = FloorCross * (((1.0f + FloorDot) * 2.0f) * SlideFloorMultiplier);
	}
	else if (FloorDot > 0.0f && FloorDot <= 0.85f)
	{
		FloorForce = FloorCross * (((1.0f - FloorDot) * 2.0f) * SlideFloorMultiplier);
	}
	else
	{
		FloorForce = FloorCross * (((1.0f - FloorDot) * 5.0f) * SlideFloorMultiplier);
	}
	
	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::SanitizeFloat(FloorDot));
	return ForwardForce + FloorForce;
}

// Slide stop
void UTraversalComponent::SlideStop()
{
	TraversalState = ETraversalState::None;
	PlayerCharacterMovement->GroundFriction = DefaultGroundFriction;
	PlayerCharacterMovement->BrakingDecelerationWalking = DefaultBrakingDeceleration;
}


// Wall climb

FHitResult UTraversalComponent::ForwardTrace(FVector Offset)
{
	FVector Start = PlayerCharacter->GetActorLocation() + Offset;
	FVector End = Start + PlayerCharacter->GetActorForwardVector() * WallDetectionDistance;
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 1.0f);
	
	return Hit;
}

bool UTraversalComponent::WallClimbCheck()
{
	if (TraversalState != ETraversalState::None)
	{
		return false;
	}

	FHitResult TraceResult = ForwardTrace(FVector(0.0f, 0.0f, 0.0f));
	if (TraceResult.bBlockingHit)
	{
		WallClimbStart(TraceResult);
		return true;
	}
	else
	{
		return false;
	}
}

void UTraversalComponent::WallClimbStart(FHitResult& ForwardTraceHit)
{
	TraversalState = ETraversalState::WallClimbing;
	PlayerCharacterMovement->SetMovementMode(MOVE_Flying);
	PlayerCharacterMovement->bOrientRotationToMovement = false;
	PlayerCharacterMovement->MaxFlySpeed = WallClimbSpeed;
	PlayerCharacterMovement->BrakingDecelerationFlying = 2048;
	PlayerCharacterMovement->StopMovementImmediately();

	// Place player against the wall
	FVector TargetLocation = ForwardTraceHit.Location + ForwardTraceHit.Normal * PlayerCapsule->GetScaledCapsuleRadius();
	FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(ForwardTraceHit.Normal * -1.0f);
	PlayerCharacter->SetActorLocationAndRotation(TargetLocation, TargetRotation);
}

bool UTraversalComponent::IsTurnAngleClimbable(FVector CurrentWallNormal, FVector TargetWallNormal, float MaxTurnAngle)
{
	float Dot = FVector::DotProduct(CurrentWallNormal, TargetWallNormal);
	float Angle = UKismetMathLibrary::Acos(Dot);

	return Angle <= MaxTurnAngle;
}

void UTraversalComponent::WallClimbInwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal)
{
	FVector Start = PlayerCharacter->GetActorLocation();
	FVector End = Start + Direction * (AxisValue * InwardTurnDetectionDistance);
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	if (Hit.bBlockingHit)
	{
		// Check if turn angle of wall can be climbed onto
		bool bCanTurn = IsTurnAngleClimbable(CurrentWallNormal, Hit.Normal, MaxInwardTurnAngle);
		if (bCanTurn)
		{
			WallClimbInwardTurn(AxisValue);
		}
	}
}

void UTraversalComponent::WallClimbInwardTurn(float AxisValue)
{
	if (AxisValue < 0.0f)
	{
		PlayerCharacter->PlayAnimMontage(LeftInwardTurnAnimation);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Left Inward Turn"));
	}
	else
	{
		PlayerCharacter->PlayAnimMontage(RightInwardTurnAnimation);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Right Inward Turn"));
	}
}

FHitResult UTraversalComponent::WallClimbDirectionalTrace(FVector Direction, float AxisValue)
{
	FVector Start = PlayerCharacter->GetActorLocation() + Direction * (AxisValue * DirectionalTraceDistance);
	FVector End = Start + PlayerCharacter->GetActorForwardVector() * WallDetectionDistance;
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 1.0f);

	return Hit;
}

void UTraversalComponent::WallClimbMovement(FVector Direction, float AxisValue)
{
	FHitResult ForwardTraceHit = ForwardTrace(FVector(0.0f, 0.0f, 0.0f));
	//FHitResult ClimbTraceHit;

	FVector CurrentWallNormal = ForwardTraceHit.Normal;
	FVector TargetLocation;
	FVector TargetNormal;

	// Check if there is axis input
	if (AxisValue != 0.0f)
	{	
		WallClimbInwardTurnTrace(Direction, AxisValue, CurrentWallNormal);

		FHitResult DirectionalTraceHit = WallClimbDirectionalTrace(Direction, AxisValue);

		if (DirectionalTraceHit.bBlockingHit)
		{
			TargetLocation = DirectionalTraceHit.Location;
			TargetNormal = DirectionalTraceHit.Normal;

			FVector ClimbUnitDirection = UKismetMathLibrary::GetDirectionUnitVector(PlayerCharacter->GetActorLocation(), TargetLocation + (TargetNormal * PlayerCapsule->GetScaledCapsuleRadius()));
			FVector ClimbDirection;
			if (AxisValue > 0.0f)
			{
				ClimbDirection = ClimbUnitDirection * 1.0f;
			}
			else
			{
				ClimbDirection = ClimbUnitDirection * -1.0f;
			}

			PlayerCharacter->AddMovementInput(ClimbDirection, AxisValue);

			// Adjust player rotation to wall
			FRotator NewRotation = UKismetMathLibrary::RInterpTo(PlayerCharacter->GetActorRotation(), (TargetNormal * -1.0f).Rotation(), UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), 5.0f);
			PlayerCharacter->SetActorRotation(NewRotation);
		}
		else
		{
			WallClimbOutwardTurnTrace(Direction, AxisValue, CurrentWallNormal, DirectionalTraceHit.TraceEnd);
		}
	}
}

void UTraversalComponent::WallClimbOutwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal, FVector DirectionalTraceEnd)
{
	if (Direction == PlayerCharacter->GetActorRightVector() || Direction == PlayerCharacter->GetActorRightVector() * -1)
	{
		FVector Start = DirectionalTraceEnd;
		FVector End = Start + Direction * (AxisValue * -1.0f * OutwardTurnDetectionDistance);
		TArray<AActor*> ActorsToIgnore;
		FHitResult Hit;

		UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 1.0f);

		if (Hit.bBlockingHit)
		{
			// Check if turn angle of wall can be climbed onto
			bool bCanTurn = IsTurnAngleClimbable(CurrentWallNormal, Hit.Normal, MaxOutwardTurnAngle);

			if (bCanTurn)
			{
				WallClimbOutwardTurn(AxisValue);
			}
		}
	}
}

void UTraversalComponent::WallClimbOutwardTurn(float AxisValue)
{
	if (AxisValue < 0.0f)
	{
		PlayerCharacter->PlayAnimMontage(LeftOutwardTurnAnimation);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Left Outward Turn"));
	}
	else
	{
		PlayerCharacter->PlayAnimMontage(RightOutwardTurnAnimation);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Right Outward Turn"));
	}
}

void UTraversalComponent::WallClimbStop()
{
	TraversalState = ETraversalState::None;
	PlayerCharacterMovement->SetMovementMode(MOVE_Walking);
	PlayerCharacterMovement->bOrientRotationToMovement = true;
	PlayerCharacterMovement->StopMovementImmediately();
}
