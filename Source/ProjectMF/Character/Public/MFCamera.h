// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MFCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class PROJECTMF_API AMFCamera : public AActor
{
	GENERATED_BODY()

public:
	AMFCamera();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetFollowTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Camera")
	AActor* GetFollowTarget() const { return FollowTarget; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<AActor> FollowTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FollowSpeed = 5.0f;
};
