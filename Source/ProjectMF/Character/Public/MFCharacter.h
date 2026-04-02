// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MFCharacter.generated.h"

class UPaperFlipbookComponent;
class UPaperFlipbook;
class UInputAction;
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

	// --- Input Actions (在蓝图或数据资产中配置) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	void HandleMove(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();

	// --- Flipbook ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> RunFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> JumpFlipbook;
};
