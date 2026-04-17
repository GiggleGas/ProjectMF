// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MFPetAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/**
 * AMFPetAIController — 宠物 AI 的 Controller 基类。
 *
 * 职责：
 *   - 持有 UStateTreeAIComponent，负责启动 / 停止 StateTree。
 *   - UStateTreeAIComponent 会自动将 AIController 和受控 Pawn 注入为
 *     StateTree 的 Context 对象，无需手动绑定。
 *
 * 使用方式：
 *   AMFSpawnAIManager 在 Spawn + Possess 完成后调用 RunStateTree(Asset)。
 *   如需自定义行为，继承本类并在蓝图或 C++ 中扩展。
 *
 * StateTree 资产配置（编辑器侧）：
 *   - Context Data 中添加 AIController（类型 AMFPetAIController）
 *   - Context Data 中添加 ControlledPawn（类型 AMFPetBase）
 */
UCLASS(Blueprintable)
class PROJECTMF_API AMFPetAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMFPetAIController();

	/**
	 * 绑定 StateTree 资产并启动逻辑。
	 * 若已有 StateTree 在运行，先停止再重启（支持运行时切换行为）。
	 * 由 AMFSpawnAIManager::SpawnSinglePet 在 Possess 完成后调用。
	 *
	 * @param InStateTree  要运行的 StateTree 资产；传 nullptr 仅停止当前逻辑。
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void RunStateTree(UStateTree* InStateTree);

	/** 返回 StateTree 是否正在运行。 */
	UFUNCTION(BlueprintPure, Category = "AI|StateTree")
	bool IsStateTreeRunning() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/**
	 * StateTree 运行时组件。
	 * UStateTreeAIComponent 继承自 UStateTreeComponent，并自动提供
	 * AIController + ControlledPawn 两个 Context 对象。
	 */
	UPROPERTY(VisibleAnywhere, Category = "AI|StateTree")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;
};
