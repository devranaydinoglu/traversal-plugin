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

// Set references
void UTraversalComponent::Initialize(ACharacter* Character)
{
	PlayerCharacter = Character;
	PlayerCharacterMovement = Character->GetCharacterMovement();
	PlayerCapsule = Character->GetCapsuleComponent();

	DefaultGravity = PlayerCharacterMovement->GravityScale;
	DefaultGroundFriction = PlayerCharacterMovement->GroundFriction;
	DefaultBrakingDeceleration = PlayerCharacterMovement->BrakingDecelerationWalking;
}

// General
// Get capsule location on ground
FVector UTraversalComponent::GetCapsuleBaseLocation()
{
	return PlayerCapsule->GetComponentLocation() - (PlayerCapsule->GetUpVector() * PlayerCapsule->GetScaledCapsuleHalfHeight());
}

// Get capsule location on ground based on specified base location
FVector UTraversalComponent::GetCapsuleLocationFromBaseLocation(FVector BaseLocation)
{
	return BaseLocation + FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight() + 5.0f);
}

// Check if capsule doesn't hit anything at location
bool UTraversalComponent::IsRoomForCapsule(FVector Location)
{
	FVector Start = Location + FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight_WithoutHemisphere());
	FVector End = Location - FVector(0.0f, 0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight_WithoutHemisphere());
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, PlayerCapsule->GetScaledCapsuleRadius(), DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);
	
	return !Hit.bBlockingHit && !Hit.bStartPenetrating;
}

// Reset movement mode and traversal state
void UTraversalComponent::OnMontageCompleted()
{
	PlayerCharacterMovement->SetMovementMode(MOVE_Walking);
	TraversalState = ETraversalState::None;
}

// Trace forward to check if character can't step onto object
FCheckObjectHeightOut UTraversalComponent::CheckObjectHeight(float ReachDistance, float MinLedgeHeight, float MaxLedgeHeight)
{
	FVector Start = (GetCapsuleBaseLocation() + PlayerCharacter->GetLastMovementInputVector() * -15.0f) + FVector(0.0f, 0.0f, (MinLedgeHeight + MaxLedgeHeight) / 2);
	FVector End = Start + PlayerCharacter->GetLastMovementInputVector() * ReachDistance;
	float HalfHeight = (MaxLedgeHeight - MinLedgeHeight) / 2;
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, End, 5.0f, HalfHeight, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	FCheckObjectHeightOut ReturnValue;

	if (Hit.bBlockingHit && !Hit.bStartPenetrating && !PlayerCharacterMovement->IsWalkable(Hit))
	{
		ReturnValue = { true, Hit.ImpactPoint, Hit.ImpactNormal };
		return ReturnValue;
	}
	else
	{
		ReturnValue = { false, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f) };
		return ReturnValue;
	}
}

// Trace downward from the initial trace's impact point and determine if the hit location is walkable. If it is, set impact point as object start sync point
FIsWalkableSurfaceOut UTraversalComponent::IsWalkableSurface(float MaxLedgeHeight, FVector InitialImpactPoint, FVector InitialImpactNormal)
{
	FVector End = PlayerCharacter->GetLastMovementInputVector() * 10.0f + FVector(InitialImpactPoint.X, InitialImpactPoint.Y, GetCapsuleBaseLocation().Z);
	FVector Start = End + FVector(0.0f, 0.0f, MaxLedgeHeight + 30.0f);
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, 5.0f, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	if (Hit.bBlockingHit && PlayerCharacterMovement->IsWalkable(Hit))
	{
		FIsWalkableSurfaceOut ReturnValue = { true, Hit.ImpactPoint };
		return ReturnValue;
	}
	else
	{
		FIsWalkableSurfaceOut ReturnValue = { false, FVector(0.0f, 0.0f, 0.0f) };
		return ReturnValue;
	}
}

// Check if nothing is blocking the path
bool UTraversalComponent::CheckPath(float Height, FVector EndTargetLocation)
{
	FVector Start = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetActorUpVector() * Height;
	FVector End = GetCapsuleLocationFromBaseLocation(EndTargetLocation);
	const TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), Start, End, PlayerCapsule->GetScaledCapsuleRadius(), PlayerCapsule->GetScaledCapsuleHalfHeight(), DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	return !bHit;
}

// Determine correct vault/mantle animation properties based on height
// Adjust animation starting position for mantling while in air
FAnimationProperties UTraversalComponent::DetermineAnimationProperties(float Height, const TArray<FAnimationPropertySettings>& AnimationPropertySettings)
{
	FAnimationProperties ReturnValue;

	for (const FAnimationPropertySettings& PropertySetting : AnimationPropertySettings)
	{
		if (IsValid(PropertySetting.Animation) && UKismetMathLibrary::InRange_FloatFloat(Height, PropertySetting.AnimationMinHeight, PropertySetting.AnimationMaxHeight))
		{
			ReturnValue = { PropertySetting.Animation, PropertySetting.AnimationHeightOffset, UKismetMathLibrary::MapRangeClamped(Height, PropertySetting.InHeightA, PropertySetting.InHeightB, PropertySetting.StartingPositionA, PropertySetting.StartingPositionA) };
			return ReturnValue;
		}
	}

	ReturnValue = { nullptr, 0.0f, 0.0f };
	return ReturnValue;
}


// Vault

// Check if player can vault
bool UTraversalComponent::VaultCheck()
{
	if (TraversalState == ETraversalState::None && !PlayerCharacterMovement->IsFalling())
	{
		FVector InitialTraceImpactPoint;
		FVector InitialTraceImpactNormal;

		// Trace forward to check if character can't step onto object
		FCheckObjectHeightOut CheckObjectHeightReturnValue = CheckObjectHeight(VaultReachDistance, VaultMinLedgeHeight, VaultMaxLedgeHeight);

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
		FIsWalkableSurfaceOut IsWalkableSurfaceReturnValue = IsWalkableSurface(VaultMaxLedgeHeight, InitialTraceImpactPoint, InitialTraceImpactNormal);

		if (IsWalkableSurfaceReturnValue.bIsWalkable)
		{
			WalkableImpactPoint = IsWalkableSurfaceReturnValue.WalkableImpactPoint;
			ObjectStartWarpTarget = WalkableImpactPoint;
			FVector VaultHeightVec = GetCapsuleLocationFromBaseLocation(WalkableImpactPoint) - PlayerCharacter->GetActorLocation();
			VaultHeight = VaultHeightVec.Z;

			// Check if vault height isn't higher than the max ledge height
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
		FCanVaultOverDepthOut CanVaultOverDepthReturnValue = CanVaultOverDepth();
		FVector EndTargetLocation;

		if (CanVaultOverDepthReturnValue.bCanVaultOverDepth)
		{
			ObjectEndWarpTarget = CanVaultOverDepthReturnValue.DepthImpactPoint;
			LandWarpTarget = GetVaultLandPoint(ObjectEndWarpTarget);
			EndTargetLocation = FVector(LandWarpTarget.X, LandWarpTarget.Y, LandWarpTarget.Z + VaultHeight);
		}
		else
		{
			return false;
		}

		// Check if nothing is blocking the mantle path
		bool bIsPathClear = CheckPath(VaultHeight, EndTargetLocation);

		if (!bIsPathClear)
		{
			return false;
		}

		// Determine correct vault animation properties based on vault height
		FAnimationProperties VaultAnimationProperties = DetermineAnimationProperties(VaultHeight, VaultAnimationPropertySettings);


		VaultStart(VaultAnimationProperties.Animation);
		return true;
	}
	else
	{
		return false;
	}
}

// Check if the depth of the actor can be vaulted over and if the player capsule fits after vault
FCanVaultOverDepthOut UTraversalComponent::CanVaultOverDepth()
{
	FVector Start = PlayerCharacter->GetActorLocation();
	FVector End = Start + PlayerCharacter->GetActorForwardVector() * VaultReachDistance;
	TArray<AActor*> ActorsToIgnore;
	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

	if (bHit)
	{
		FVector InitialImpactPoint = Hit.ImpactPoint;

		Start = InitialImpactPoint + PlayerCharacter->GetActorForwardVector() * VaultMaxDepth;
		End = InitialImpactPoint;

		bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, DetectionTraceChannel, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

		if (bHit)
		{
			FVector DepthImpactPoint = Hit.ImpactPoint;
			bool bInRange = UKismetMathLibrary::InRange_FloatFloat(UKismetMathLibrary::Vector_Distance(DepthImpactPoint, InitialImpactPoint), VaultMinDepth, VaultMaxDepth);
			FVector Location = DepthImpactPoint + PlayerCharacter->GetActorForwardVector() * (PlayerCapsule->GetScaledCapsuleRadius() + VaultLandDistance);
			bool bCanVaultOverDepth = Hit.Distance > 1 && bInRange && IsRoomForCapsule(Location);

			return { bCanVaultOverDepth, DepthImpactPoint };
		}
		else
		{
			return { false, FVector(0.0f, 0.0f, 0.0f) };
		}
	}
	else
	{
		return { false, FVector(0.0f, 0.0f, 0.0f)};
	}
}

// Trace down to get vault land point
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

// Start vault
void UTraversalComponent::VaultStart(UAnimMontage* VaultAnimation)
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
		GetWorld()->GetTimerManager().SetTimer(MontageCompletedTimerHandle, this, &UTraversalComponent::OnMontageCompleted, MontageDuration, false);
	}
}


// Mantle

// Check if player can mantle
bool UTraversalComponent::MantleCheck()
{
	if (TraversalState == ETraversalState::None)
	{
		FVector InitialTraceImpactPoint;
		FVector InitialTraceImpactNormal;

		// Trace forward to check if character can't step onto object
		FCheckObjectHeightOut CheckObjectHeightReturnValue = CheckObjectHeight(MantleReachDistance, MantleMinLedgeHeight, MantleMaxLedgeHeight);

		if (CheckObjectHeightReturnValue.bIsNotWalkable)
		{
			InitialTraceImpactPoint = CheckObjectHeightReturnValue.InitialImpactPoint;
			InitialTraceImpactNormal = CheckObjectHeightReturnValue.InitialImpactNormal;
		}
		else
		{
			return false;
		}

		FVector WalkableImpactPoint;
		float MantleHeight;

		// Trace downward from the initial trace's impact point and determine if the hit location is walkable. If it is, set impact point as object start sync point
		FIsWalkableSurfaceOut IsWalkableSurfaceReturnValue = IsWalkableSurface(MantleMaxLedgeHeight, InitialTraceImpactPoint, InitialTraceImpactNormal);

		if (IsWalkableSurfaceReturnValue.bIsWalkable)
		{
			WalkableImpactPoint = IsWalkableSurfaceReturnValue.WalkableImpactPoint;
			ObjectStartWarpTarget = WalkableImpactPoint;
			FVector MantleHeightVec = GetCapsuleLocationFromBaseLocation(WalkableImpactPoint) - PlayerCharacter->GetActorLocation();
			MantleHeight = MantleHeightVec.Z;
		}
		else
		{
			return false;
		}

		// Check if nothing is blocking the mantle path
		bool bIsPathClear = CheckPath(MantleHeight, WalkableImpactPoint);

		if (!bIsPathClear)
		{
			return false;
		}

		UAnimMontage* MantleAnimation;
		float AnimationHeightOffset;
		float AnimationStartingPosition;

		// Determine correct mantle animation based on mantle height
		FAnimationProperties MantleAnimationProperties = DetermineAnimationProperties(MantleHeight, MantleAnimationPropertySettings);

		MantleAnimation = MantleAnimationProperties.Animation;
		AnimationHeightOffset = MantleAnimationProperties.AnimationHeightOffset;
		AnimationStartingPosition = MantleAnimationProperties.AnimationStartingPosition;

		// Start mantle
		MantleStart(MantleAnimation, AnimationHeightOffset, AnimationStartingPosition);

		return true;
	}
	else
	{
		return false;
	}
}

// Add height offset to warp target point
float UTraversalComponent::ApplyMantleAnimationHeightOffset(float HeightOffset)
{
	return PlayerCapsule->GetScaledCapsuleHalfHeight() * 2 - HeightOffset;
}

// Start mantle
void UTraversalComponent::MantleStart(UAnimMontage* MantleAnimation, float HeightOffset, float StartingPosition)
{
	TraversalState = ETraversalState::Mantling;

	// Set player movement mode and add warp target
	PlayerCharacterMovement->SetMovementMode(MOVE_Flying);
	UMotionWarpingComponent* PlayerMotionWarpingComponent = PlayerCharacter->FindComponentByClass<UMotionWarpingComponent>();
	if (IsValid(PlayerMotionWarpingComponent))
	{
		FVector TargetLocation = ObjectStartWarpTarget - FVector(0.0f, 0.0f, ApplyMantleAnimationHeightOffset(HeightOffset));
		DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 12, FColor::Red, false, 2.0f, false, 1.0f);
		PlayerMotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(MantleWarpTargetName, TargetLocation, PlayerCharacter->GetActorRotation());

		float MontageDuration = PlayerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(MantleAnimation, 1.0f, EMontagePlayReturnType::Duration, StartingPosition, true);
		FTimerHandle MontageCompletedTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(MontageCompletedTimerHandle, this, &UTraversalComponent::OnMontageCompleted, MontageDuration - StartingPosition, false);
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
// Shoot forward trace from offset
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

// Check if player can wall climb
bool UTraversalComponent::WallClimbCheck()
{
	if (TraversalState == ETraversalState::None)
	{
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
	else
	{
		return false;
	}
}

// Start wall climb
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

// Check if wall can be climbed onto based on angle between the current wall and target wall
bool UTraversalComponent::IsTurnAngleClimbable(FVector CurrentWallNormal, FVector TargetWallNormal, float MaxTurnAngle)
{
	float Dot = FVector::DotProduct(CurrentWallNormal, TargetWallNormal);
	float Angle = UKismetMathLibrary::Acos(Dot);

	return Angle <= MaxTurnAngle;
}

// Perform trace to detect inward wall/turn on either side of the player
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

// Perform inward turn
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

// Perform forward trace from input direction to check if there is space to climb to
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

// Handle movement during wall climb
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

// Perform trace to detect outward wall/turn
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

// Perform outward turn
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

// Stop wall climb
void UTraversalComponent::WallClimbStop()
{
	TraversalState = ETraversalState::None;
	PlayerCharacterMovement->SetMovementMode(MOVE_Walking);
	PlayerCharacterMovement->bOrientRotationToMovement = true;
	PlayerCharacterMovement->StopMovementImmediately();
}
