// Copyright 2023 devran. All Rights Reserved.

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
struct FIsObjectClimbableOut
{
	GENERATED_BODY()

	bool bIsNotWalkable = false;
	FVector InitialImpactPoint = FVector::ZeroVector;
	FVector InitialImpactNormal = FVector::ZeroVector;
};

USTRUCT()
struct FIsSurfaceWalkableOut
{
	GENERATED_BODY()

	bool bIsWalkable = false;
	FVector WalkableImpactPoint = FVector::ZeroVector;
};

USTRUCT()
struct FCanVaultOverDepthOut
{
	GENERATED_BODY()

	bool bCanVaultOverDepth = false;
	FVector DepthImpactPoint = FVector::ZeroVector;
};

USTRUCT()
struct FAnimationProperties
{
	GENERATED_BODY()

	UPROPERTY()
	UAnimMontage* Animation = nullptr;

	float AnimationHeightOffset = 0.0f;
	double AnimationStartingPosition = 0.0;
	float AnimationEndBlendTime = 0.0f;
};

/**
* Animation properties that are used to adjust animation to conditions. Can be used to play different vault animation for different heights.
*/
USTRUCT()
struct FAnimationPropertySettings
{
	GENERATED_BODY()

	// Animation to be played.
	UPROPERTY(EditAnywhere)
	UAnimMontage* Animation = nullptr;

	// Min height at which this animation should be played.
	UPROPERTY(EditAnywhere)
	float AnimationMinHeight = 0.0f;

	// Max height at which this animation should be played.
	UPROPERTY(EditAnywhere)
	float AnimationMaxHeight = 0.0f;

	// Offset added to the start warp target's Z axis. Can be used to tweak the target height so the character's hand position in the animation is perfectly aligned with the ledge.
	UPROPERTY(EditAnywhere)
	float AnimationHeightOffset = 0.0f;

	// Lower bound used to pick a starting time for the mantle animation based on the mantle height. This will prevent the full animation from playing when the mantle distance is small.
	UPROPERTY(EditAnywhere)
	float InHeightA = 0.0f;

	// Upper bound used to pick a starting time for the mantle animation based on the mantle height. This will prevent the full animation from playing when the mantle distance is small.
	UPROPERTY(EditAnywhere)
	float InHeightB = 0.0f;

	// Starting time (ratio from 0.0 to 1.0) of the mantle animation used for the height's lower bound (InHeightA). Higher value means that a smaller portion of the mantle animation will be played.
	UPROPERTY(EditAnywhere)
	float StartingPositionA = 0.0f;

	// Starting time (ratio from 0.0 to 1.0) of the mantle animation used for the height's upper bound (InHeightB). Lower value means that a bigger portion of the mantle animation will be played.
	UPROPERTY(EditAnywhere)
	float StartingPositionB = 0.0f;

	// The time in seconds that will be cut off from the end of the animation to allow for better blending.
	UPROPERTY(EditAnywhere)
	float AnimationEndBlendTime = 0.0f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRAVERSALSYSTEM_API UTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	// Owning character reference.
	UPROPERTY()
	ACharacter* PlayerCharacter;

	// Owning character movement component reference.
	UPROPERTY()
	UCharacterMovementComponent* PlayerCharacterMovement;

	// Owning character capsule component reference.
	UPROPERTY()
	UCapsuleComponent* PlayerCapsule;

	// Current traversal state.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Traversal")
	ETraversalState TraversalState;

	// Trace channel used to detect objects.
	UPROPERTY(EditAnywhere, Category = "Traversal")
	TEnumAsByte<ETraceTypeQuery> DetectionTraceChannel;

	// Default owning character gravity.
	float DefaultGravity;

	// Default owning character ground friction.
	float DefaultGroundFriction;

	// Default owning character braking deceleration.
	float DefaultBrakingDeceleration;

	// Warp target placed at the start of the object.
	FVector ObjectStartWarpTarget;

	// Warp target placed at the end of the object.
	FVector ObjectEndWarpTarget;

	// Warp target placed behind the object.
	FVector LandWarpTarget;



	// Z offset added to the capsule's starting position. Used for fine tuning when taking the height of the capsule component into account.
	UPROPERTY(EditAnywhere, Category = "General")
	float GlobalHeightOffsetZ;



	// Max distance to object to initiate vault.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultReachDistance;

	// Min ledge height that can be vaulted over.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMinLedgeHeight;

	// Max ledge height that can be vaulted over.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxLedgeHeight;

	// Min obstacle depth that can be vaulted over.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMinDepth;

	// Max obstacle depth that can be vaulted over.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxDepth;

	// Max angle between the owning character's forward vector and obstacle's normal that allows vaulting.
	UPROPERTY(EditAnywhere, Category = "Vault")
	int32 VaultMaxApproachAngle;

	// Distance behind obstacle that will be landed at.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultLandDistance;

	// Distance that will detect the ground behind the obstacle.
	UPROPERTY(EditAnywhere, Category = "Vault")
	float VaultMaxLandVerticalDistance;

	// Animation properties that are used to adjust animation to conditions. Can be used to play different vault animation for different heights.
	UPROPERTY(EditAnywhere, Category = "Vault|Animations")
	TArray<FAnimationPropertySettings> VaultAnimationPropertySettings;

	// Warp target name specified in the vault montage.
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultObjectStartWarpTargetName;

	// Warp target name specified in the vault montage.
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultObjectEndWarpTargetName;

	// Warp target name specified in the vault montage.
	UPROPERTY(EditAnywhere, Category = "Vault")
	FName VaultLandWarpTargetName;

	// Calculated height of the object to vault over. Will only be set after an attemp to vault.
	UPROPERTY(BlueprintReadOnly, Category = "Vault")
	float VaultHeight;



	// Max distance to object to initiate mantle.
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleReachDistance;

	// Min ledge height that can be mantled on.
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleMinLedgeHeight;

	// Max ledge height that can be mantled on.
	UPROPERTY(EditAnywhere, Category = "Mantle")
	float MantleMaxLedgeHeight;

	// Animation properties that are used to adjust animation to conditions.
	UPROPERTY(EditAnywhere, Category = "Mantle|Animations")
	TArray<FAnimationPropertySettings> MantleAnimationPropertySettings;

	// Warp target name specified in the mantle montage.
	UPROPERTY(EditAnywhere, Category = "Mantle")
	FName MantleWarpTargetName;

	// Calculated height of the object to mantle on. Will only be set after an attempt to mantle.
	UPROPERTY(BlueprintReadOnly, Category = "Mantle")
	float MantleHeight;



	// Base slide power.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlidePower;

	// Determines how much influence the floor's normal has.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideFloorMultiplier;

	// Ground friction during slide.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideGroundFriction;

	// Braking power during slide.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideBrakingPower;

	// Min speed while sliding. Slide will be stopped if the owning character's speed gets below this value.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideMinSpeed;

	// Max speed while sliding. Used to clamp owning character's speed to this value while sliding.
	UPROPERTY(EditAnywhere, Category = "Slide")
	float SlideMaxSpeed;

	

	// Froward distance used to detect wall.
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float WallDetectionDistance;

	// Speed while wall climbing.
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float WallClimbSpeed;

	// Distance used to detect wall for inward turns.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	float InwardTurnDetectionDistance;

	// Max angle for inward turns.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	float MaxInwardTurnAngle;

	// Left inward turn animation.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	UAnimMontage* LeftInwardTurnAnimation;

	// Right inward turn animation.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Inward Turn")
	UAnimMontage* RightInwardTurnAnimation;

	// Distance used to detect wall at direction of input.
	UPROPERTY(EditAnywhere, Category = "Wall Climb")
	float DirectionalTraceDistance;
	
	// Distance used to detect wall for outward turns.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	float OutwardTurnDetectionDistance;

	// Max angle for outward turns.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	float MaxOutwardTurnAngle;

	// Left outward turn animation.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	UAnimMontage* LeftOutwardTurnAnimation;

	// Right outward turn animation.
	UPROPERTY(EditAnywhere, Category = "Wall Climb|Outward Turn")
	UAnimMontage* RightOutwardTurnAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "Wall Climb")
	float WallClimbHorizontalInput;

	UPROPERTY(BlueprintReadOnly, Category = "Wall Climb")
	float WallClimbVerticalInput;

	bool bWallClimbIsTurning = false;

	FTimerHandle WallClimbTurnMontageCompletedHandle;

public:
	// Sets default values for this component's properties.
	UTraversalComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Initialize the component and set default values.
	* 
	* @param Character Owning character.
	*/
	UFUNCTION(BlueprintCallable)
	void Initialize(ACharacter* Character);

	/**
	* Check if the character meets the requirements to vault.
	*/
	UFUNCTION(BlueprintCallable, Category = "Vault")
	bool VaultCheck();

	/**
	* Check if the character meets the requirements to mantle.
	*/
	UFUNCTION(BlueprintCallable, Category = "Mantle")
	bool MantleCheck();

	/**
	* Check if the character meets the requirements to slide.
	*/
	UFUNCTION(BlueprintCallable, Category = "Slide")
	bool SlideCheck();

	/**
	* Check if the character meets the requirements to wall climb.
	*/
	UFUNCTION(BlueprintCallable, Category = "Wall Climb")
	bool WallClimbCheck();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/**
	* Get the most bottom point of the capsule component.
	* 
	* @return Most bottom point of the capsule component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCapsuleBaseLocation();

	/**
	* Place capsule collision on top of a given point.
	* 
	* @param BaseLocation Location to place capsule collision on.
	* @return Most bottom point of the capsule component.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCapsuleLocationFromBaseLocation(FVector BaseLocation);

	/**
	* Trace a sphere to check whether the capsule will collide with anything at the given location.
	* 
	* @param Location Location to check.
	* @returns Room for capsule.
	*/
	bool IsRoomForCapsule(FVector Location);

	/**
	* Check if the object is within reach.
	* Check if the object's height is between the min and max ledge height.
	* Check if there is room for the capsule component.
	* 
	* @param ReachDistance Distance from the character within which the object needs to be.
	* @param MinLedgeHeight Min height of the ledge.
	* @param MaxLedgeHeight Max height of the ledge.
	* @return Whether the top of the object is walkable, the impact location on top of the object, and the impact normal of the top of the object.
	*/
	FIsObjectClimbableOut IsObjectClimbable(float ReachDistance, float MinLedgeHeight, float MaxLedgeHeight);

	/**
	* Trace downward from the initial trace's impact point and determine if the hit location is walkable.
	* If it is, set the impact point of this trace as object start sync point.
	* 
	* @param MaxLedgeHeight Max height of the ledge.
	* @param InitialImpactPoint Impact point of the initial trace.
	* @param InitialImpactNormal Impact normal of the initial trace.
	* @return Whether the top of the object is walkable and the impact point of the trace. 
	*/
	FIsSurfaceWalkableOut IsSurfaceWalkable(float MaxLedgeHeight, FVector InitialImpactPoint, FVector InitialImpactNormal);

	/**
	* Check if nothing is blocking the path by sweeping a capsule along the path.
	* 
	* @param Height Height of the ledge.
	* @param EndTargetLocation Target location of the vault or target.
	* @return Path is clear.
	*/
	bool IsCapsulePathClear(float Height, FVector EndTargetLocation);

	/**
	* Determine the correct vault/mantle animation based on the ledge height in FAnimationPropertySettings.
	* Adjust the starting position for the mantle montage.
	* 
	* @param Height Ledge height.
	* @param AnimationPropertySettings Property settings of each vault and mantle animation.
	* @return Animation properties to be used for the action.
	*/
	FAnimationProperties DetermineAnimationProperties(float Height, const TArray<FAnimationPropertySettings>& AnimationPropertySettings);



	/**
	* Check if the depth of the actor can be vaulted over and if the character capsule fits after vault.
	* 
	* @return Whether object is in range and there is room for the capsule component and the impact point of the object depth check.
	*/
	FCanVaultOverDepthOut CanVaultOverDepth();

	/**
	* Trace down from the object end point + the specified vault land distance to get the target landing point.
	* 
	* @param ObjectEndPoint End point of the object to be vaulted over.
	* @return Target location to land on.
	*/
	FVector GetVaultLandPoint(FVector ObjectEndPoint);

	/**
	* Prepare character and motion warping component for the vault.
	* 
	* @param VaultAnimation Vault animation to play.
	*/
	void VaultStart(UAnimMontage* VaultAnimation, float AnimationEndBlendTime);

	/**
	* Reset movement mode to walking and traversal state to none.
	*/
	void OnVaultMontageCompleted(float AnimationEndBlendTime);


	/**
	* Add height offset to warp target location.
	* The height offset allows for tweaking so the character's hands line up with the ledge.
	* 
	* @param HeightOffset Offset added to the warp target location's Z axis.
	* @return Offset to be added to the warp target location's Z axis.
	*/
	float ApplyMantleHeightOffset(float HeightOffset);

	/**
	* Prepare character and motion warping component for the mantle.
	* 
	* @param MantleAnimation Mantle animation to play.
	* @param HeightOffset Offset added to the warp target location's Z axis.
	* @param StartingPosition Starting position/time of the mantle animation.
	*/
	void MantleStart(const FAnimationProperties& AnimationProperties);

	/**
	* Reset movement mode to walking and traversal state to none.
	*/
	void OnMantleMontageCompleted(float AnimationEndBlendTime);


	// Start slide
	void SlideStart();

	// Slide update
	void SlideUpdate();

	// Calculate the force applied while sliding
	FVector CalculateSlideForce(FVector FloorNormal);

	// Slide stop
	void SlideStop();



	/**
	* Shoot a trace forward from the character's location taking into account the offset.
	* 
	* @param Offset Offset to be added to the trace's start location.
	* @return The hit result of the trace.
	*/
	FHitResult ForwardTrace(FVector Offset);

	/**
	* Prepare the character for the wall climb and move and rotate the character against the wall.
	* 
	* @param ForwardTraceHit Hit result of the forward trace.
	*/
	void WallClimbStart(FHitResult& ForwardTraceHit);

	/**
	* Reset the traversal state and character movement component to walking state.
	*/
	UFUNCTION(BlueprintCallable)
	void WallClimbStop();

	/**
	* Check in each direction whether there is enough room to hold onto the wall by using the player capsule component's size.
	*
	* @returns Whether there is room on each side of the character to start the wall climb.
	*/
	bool IsRoomToStartWallClimb();

	/**
	* Check if the target wall can be climbed onto based on the angle between the current wall and the target wall.
	* 
	* @param CurrentWallNormal Normal of the wall the character is currently on.
	* @param TargetWallNormal Normal of the wall the character wants to climb onto.
	* @param MaxTurnAngle The max angle between the current wall and the target wall onto which the character can climb.
	* @return Target wall can be climbed onto.
	*/
	bool IsTurnAngleClimbable(FVector CurrentWallNormal, FVector TargetWallNormal, float MaxTurnAngle);

	/**
	* Shoot a trace towards the input movement direction to detect a wall for an inward trace.
	* 
	* @param Direction Input movement direction of the character.
	* @param AxisValue Input movement value of the character.
	* @param CurrentWallNormal Normal of the wall the character is currently on.
	*/
	void WallClimbInwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal);

	/**
	* Play the correct inward turn animation.
	* 
	* @param AxisValue Input movement value of the character. Used to check the turn direction.
	*/
	void WallClimbInwardTurn(float AxisValue);

	/**
	* Shoot a forward trace from the top, right, bottom, or left of the character to check if there is space on the wall so the character can move in that direction.
	* 
	* @param Direction Input movement direction of the character.
	* @param AxisValue Input movement value of the character.
	* @return Hit result of the forward trace.
	*/
	FHitResult WallClimbDirectionalTrace(FVector Direction, float AxisValue);

	/**
	* Shoot a trace inward to the left or right of the character depending on the input direction to detect walls for an outward turn.
	* Gets called if the wall movement trace didn't hit anything.
	*
	* @param Direction Input movement direction of the character. Used to determine the traces' directions.
	* @param AxisValue Input movement value of the character. Used to determine the traces' directions.
	* @param CurrentWallNormal Normal of the wall the character is currently on.
	* @param DirectionalTraceEnd The end of the wall movement trace. Used as the starting point of this trace.
	*/
	void WallClimbOutwardTurnTrace(FVector Direction, float AxisValue, FVector CurrentWallNormal, FVector DirectionalTraceEnd);

	/**
	* Play the correct outward turn animation.
	*
	* @param AxisValue Input movement value of the character. Used to check the turn direction.
	*/
	void WallClimbOutwardTurn(float AxisValue);

	/**
	* Handle movement during wall climb. Move the character towards the input direction.
	* 
	* @param Direction Input movement direction of the character. Used to determine the movement direction.
	* @param AxisValue Input movement value of the character. Used to determine the movement direction.
	*/
	UFUNCTION(BlueprintCallable)
	void WallClimbMovement(FVector2D Direction);

	void SetWallClimbAnimationMovementDirections(FVector Direction);

	void OnWallClimbTurnMontageCompleted();
};
