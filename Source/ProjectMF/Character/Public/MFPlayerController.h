// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MFPlayerController.generated.h"

class UMFPlayerConfig;
class UMFMainHUDWidget;
class UMFCommandComponent;

UCLASS()
class PROJECTMF_API AMFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMFPlayerController();

public:
	/**
	 * 调试控制台命令：让离玩家最近的宠物移动到 (X,Y,Z)。转调 CommandComp。
	 * 控制台输入：MFDebugPetMove 1200 800 100
	 * （Exec 放在 PlayerController 上——PC 一定在 exec 路由链里；组件 exec 不保证被命中。）
	 */
	UFUNCTION(Exec)
	void MFDebugPetMove(float X, float Y, float Z);

	/**
	 * 调试控制台命令：让离玩家最近的宠物释放指定 tag 的技能。转调 CommandComp。
	 * 控制台输入：MFDebugPetSkill MF.Ability.Pet.Move.Jump
	 */
	UFUNCTION(Exec)
	void MFDebugPetSkill(const FString& SkillTag);

	/** 玩家指令组件访问器（供 Pawn 在 SetupPlayerInputComponent 绑定命令输入）。 */
	UMFCommandComponent* GetCommandComponent() const { return CommandComp; }

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

	/**
	 * 玩家指令组件（指令系统 M0 起）：管理选中/林克时间/下令。
	 * M0 仅提供调试入口；选中/拖拽/林克时间在 M3/M4 填充。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Command", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMFCommandComponent> CommandComp;
};
