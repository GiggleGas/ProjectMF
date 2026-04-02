// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MFCharacterTypes.h"
#include "MFCharacter.generated.h"

class UPaperFlipbookComponent;
class UPaperFlipbook;
class UInputAction;
class UMFAnimationConfig;
struct FInputActionValue;

UCLASS()
class PROJECTMF_API AMFCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMFCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Flipbook")
	void SetFlipbook(UPaperFlipbook* NewFlipbook);

	UFUNCTION(BlueprintPure, Category = "Flipbook")
	UPaperFlipbook* GetCurrentFlipbook() const;

protected:
	virtual void BeginPlay() override;

	// --- Input Actions ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PickAction;

	// --- Character State ---

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FMFCharacterState CharacterState;

	// --- Animation ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UMFAnimationConfig> AnimationConfig;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandlePickStarted();
	void HandlePickCompleted();

	void UpdateCharacterAction();
	void UpdateAnimation();

	static UPaperFlipbook* GetFlipbookForDirection(const FMFDirectionalFlipbooks& Set, EMFFacingDirection Direction);
};
