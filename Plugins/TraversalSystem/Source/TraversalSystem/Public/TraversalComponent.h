// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TraversalComponent.generated.h"

class UCharacterMovementComponent;
class UCapsuleComponent;
class UAnimMontage;


UENUM(BlueprintType)
enum class ETraversalState : uint8
{
	None			UMETA(DisplayName = "None"),
	Vaulting		UMETA(DisplayName = "Vaulting"),
	Mantling		UMETA(DisplayName = "Mantling"),
	Sliding			UMETA(DisplayName = "Sliding"),
	WallClimbing	UMETA(DisplayName = "WallClimbing")
};

USTRUCT()
struct FCheckObjectHeightOut
{
	GENERATED_BODY()

	bool bIsNotWalkable;
	FVector InitialImpactPoint;
	FVector InitialImpactNormal;
};

USTRUCT()
struct FIsWalkableSurfaceOut
{
	GENERATED_BODY()

	bool bIsWalkable;
	FVector WalkableImpactPoint;
};

USTRUCT()
struct FCanVaultOverDepthOut
{
	GENERATED_BODY()

	bool bCanVaultOverDepth;
	FVector DepthImpactPoint;
};

USTRUCT()
struct FAnimationProperties
{
	GENERATED_BODY()

	UPROPERTY()
	UAnimMontage* Animation;

	float AnimationHeightOffset;
	double AnimationStartingPosition;
};

USTRUCT()
struct FAnimationPropertySettings
{
	GENERATED_BODY()

	// Animation to be played
	UPROPERTY(EditAnywhere)
	UAnimMontage* Animation;

	// Min height at which this animation should be played
	UPROPERTY(EditAnywhere)
	float AnimationMinHeight;

	// Max height at which this animation should be played
	UPROPERTY(EditAnywhere)
	float AnimationMaxHeight;

	// Vertical offset added to the start warp target
	UPROPERTY(EditAnywhere)
	float AnimationHeightOffset;

	// Lower bound used to pick a starting time (ratio from 0.0 to 1.0) for the mantle animation based on the mantle height. This will prevent the full animation from playing when the mantle distance is small
	UPROPERTY(EditAnywhere)
	float InHeightA;

	// Upper bound used to pick a starting time (ratio from 0.0 to 1.0) for the mantle animation based on the mantle height. This will prevent the full animation from playing when the mantle distance is small
	UPROPERTY(EditAnywhere)
	float InHeightB;

	// Starting time (ratio from 0.0 to 1.0) of the mantle animation used for the lower bound (InHeightA). Higher value means that a smaller portion of the mantle animation will be played
	UPROPERTY(EditAnywhere)
	float StartingPositionA;

	// Starting time (ratio from 0.0 to 1.0) of the mantle animation used for the upper bound (InHeightB). Lower value means that a bigger portion of the mantle animation will be played
	UPROPERTY(EditAnywhere)
	float StartingPositionB;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRAVERSALSYSTEM_API UTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTraversalComponent();

	// Current traversal state
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	ETraversalState TraversalState;

protected:
	// Owning character reference
	UPROPERTY()
	ACharacter* PlayerCharacter;

	// Owning character movement component reference
	UPROPERTY()
	UCharacterMovementComponent* PlayerCharacterMovement;

	// Owning character capsule component reference
	UPROPERTY()
	UCapsuleComponent* PlayerCapsule;

	// Trace channel used to detect objects
	UPROPERTY(EditAnywhere, Category = "Traversal")
	TEnumAsByte<ETraceTypeQuery> DetectionTraceChannel;

	// Default owning character gravity
	float DefaultGravity;

	// Default owning character ground friction
	float DefaultGroundFriction;

	// Default owning character braking deceleration
	float DefaultBrakingDeceleration;

	// Warp target placed at the start of the object
	FVector ObjectStartWarpTarget;

	// Warp target placed at the end of the object
	FVector ObjectEndWarpTarget;

	// Warp target placed behind the object
	FVector LandWarpTarget;

	
	/* Vault */

	// Max distance to object to initiate vault
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultReachDistance;

	// Min ledge height that can be vaulted over
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMinLedgeHeight;

	// Max ledge height that can be vaulted over
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxLedgeHeight;

	// Min obstacle depth that can be vaulted over
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMinDepth;

	// Max obstacle depth that can be vaulted over
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxDepth;

	// Max angle between the owning character's forward vector and obstacle's normal that allows vaulting
	UPROPERTY(EditAnywhere, Category = "Vault")
	int32 VaultMaxApproachAngle;

	// Distance behind obstacle that will be landed at
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultLandDistance;

	// Distance that will detect the ground behind the obstacle
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxLandVerticalDistance;

	// Animation properties that are used to adjust animation to conditions
	UPROPERTY(EditAnywhere, Category = "Vault|Animations")
	TArray<FAnimationPropertySettings> VaultAnimationPropertySettings;

	// Warp target name specified in the vault montage
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultObjectStartWarpTargetName;

	// Warp target name specified in the vault montage
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultObjectEndWarpTargetName;

	// Warp target name specified in the vault montage
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultLandWarpTargetName;


	/* Mantle */

	// Max distance to object to initiate mantle
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleReachDistance;

	// Min ledge height that can be mantled on
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleMinLedgeHeight;

	// Max ledge height that can be mantled on
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleMaxLedgeHeight;

	// Animation properties that are used to adjust animation to conditions
	UPROPERTY(EditAnywhere, Category = "Mantle|Animations")
	TArray<FAnimationPropertySettings> MantleAnimationPropertySettings;

	// Warp target name specified in the mantle montage
	UPROPERTY(EditAnywhere, Category = "Mantle")
	FName MantleWarpTargetName;


	/* Slide */

	// Base slide power
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlidePower;

	// Determines how much influence the floor's normal has
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideFloorMultiplier;

	// Ground friction during slide
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideGroundFriction;

	// Braking power during slide
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideBrakingPower;

	// Min speed while sliding. Slide will be stopped if the owning character's speed gets below this value
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideMinSpeed;

	// Max speed while sliding. Used to clamp owning character's speed to this value while sliding
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideMaxSpeed;

	
	/* Wall climb */

	// Froward distance used to detect wall
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float WallDetectionDistance;

	// Speed while wall climbing
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float WallClimbSpeed;

	// Distance used to detect wall for inward turns
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	float InwardTurnDetectionDistance;

	// Max angle for inward turns
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	float MaxInwardTurnAngle;

	// Left inward turn animation
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	UAnimMontage* LeftInwardTurnAnimation;

	// Right inward turn animation
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	UAnimMontage* RightInwardTurnAnimation;

	// Distance used to detect wall at direction of input
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float DirectionalTraceDistance;
	
	// Distance used to detect wall for outward turns
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	float OutwardTurnDetectionDistance;

	// Max angle for outward turns
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	float MaxOutwardTurnAngle;

	// Left outward turn animation
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	UAnimMontage* LeftOutwardTurnAnimation;

	// Right outward turn animation
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	UAnimMontage* RightOutwardTurnAnimation;


public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Set references
	UFUNCTION(BlueprintCallable)
	void Initialize(ACharacter* Character);

	/* Vault */
	
	// Check if player can vault
	UFUNCTION(BlueprintCallable, Category = "Vault")
	bool VaultCheck();

	/* Mantle */
	
	// Check if player can mantle
	UFUNCTION(BlueprintCallable, Category = "Mantle")
	bool MantleCheck();

	/* Slide */
	
	// Check if player can slide
	UFUNCTION(BlueprintCallable, Category = "Slide")
	bool SlideCheck();

	/* Wall climb */

	// Check if player can wall climb
	UFUNCTION(BlueprintCallable, Category = "Wall Climb")
	bool WallClimbCheck();


protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	/* General */
	
	// Get capsule location on ground
	FVector GetCapsuleBaseLocation();

	// Get capsule location on ground based on specified base location
	FVector GetCapsuleLocationFromBaseLocation(FVector BaseLocation);

	// Check if capsule doesn't hit anything at location
	bool IsRoomForCapsule(FVector Location);

	// Reset movement mode and traversal state
	void OnMontageCompleted();

	// Trace forward to check if character can't step onto object
	FCheckObjectHeightOut CheckObjectHeight(float ReachDistance, float MinLedgeHeight, float MaxLedgeHeight);

	// Trace downward from the initial trace's impact point and determine if the hit location is walkable. If it is, set impact point as object start sync point
	FIsWalkableSurfaceOut IsWalkableSurface(float MaxLedgeHeight, FVector InitialImpactPoint, FVector InitialImpactNormal);

	// Check if nothing is blocking the path
	bool CheckPath(float Height, FVector EndTargetLocation);

	// Determine correct vault/mantle animation based on height
	FAnimationProperties DetermineAnimationProperties(float Height, const TArray<FAnimationPropertySettings>& AnimationPropertySettings);


	/* Vault */

	// Check if the depth of the actor can be vaulted over and if the player capsule fits after vault
	FCanVaultOverDepthOut CanVaultOverDepth();

	// Trace down to get land point
	FVector GetVaultLandPoint(FVector ObjectEndPoint);

	// Start vault
	void VaultStart(UAnimMontage* VaultAnimation);


	/* Mantle */

	// Add height offset to warp target point
	float ApplyMantleAnimationHeightOffset(float HeightOffset);

	// Start mantle
	void MantleStart(UAnimMontage* MantleAnimation, float HeightOffset, float StartingPosition);


	/* Slide */

	// Start slide
	void SlideStart();

	// Slide update
	void SlideUpdate();

	// Calculate the force applied while sliding
	FVector CalculateSlideForce(FVector FloorNormal);

	// Slide stop
	void SlideStop();


	/* Wall climb */

	// Shoot forward trace from offset
	FHitResult ForwardTrace(FVector Offset);

	// Start wall climb
	void WallClimbStart(FHitResult& ForwardTraceHit);

	// Check if wall can be climbed onto based on angle between the current wall and target wall
	bool IsTurnAngleClimbable(FVector CurrentWallNormal, FVector TargetWallNormal, float MaxTurnAngle);

	// Perform trace to detect inward wall/turn based on input direction
	void WallClimbInwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal);

	// Perform inward turn
	void WallClimbInwardTurn(float AxisValue);

	// Perform forward trace from input direction to check if there is space to climb to
	FHitResult WallClimbDirectionalTrace(FVector Direction, float AxisValue);

	// Handle movement during wall climb
	UFUNCTION(BlueprintCallable)
	void WallClimbMovement(FVector Direction, float AxisValue);

	// Perform trace to detect outward wall/turn
	void WallClimbOutwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal, FVector DirectionalTraceEnd);

	// Perform outward turn
	void WallClimbOutwardTurn(float AxisValue);

	// Stop wall climb
	UFUNCTION(BlueprintCallable)
	void WallClimbStop();
};
