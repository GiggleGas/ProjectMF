// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MFPlayerController.generated.h"

class UMFPlayerConfig;
class UMFMainHUDWidget;

UCLASS()
class PROJECTMF_API AMFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMFPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/**
	 * 玩家专属配置资产。与 BP_MFCharacter 引用同一个实例。
	 * Controller 从中读取 DefaultMappingContext/Priority 和 MainHUDClass。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UMFPlayerConfig> PlayerConfig;

private:
	void AddInputMappingContext();
	void RemoveInputMappingContext();

	UPROPERTY()
	TObjectPtr<UMFMainHUDWidget> MainHUDInstance;
};
