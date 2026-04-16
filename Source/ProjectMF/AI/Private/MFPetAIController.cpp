// Copyright ProjectMF. All Rights Reserved.

#include "MFPetAIController.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"
#include "MFLog.h"

// ============================================================
// 构造
// ============================================================

AMFPetAIController::AMFPetAIController()
{
	// 在 CDO 上创建组件；StateTree 资产由 RunStateTree() 在运行时赋值。
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
}

// ============================================================
// Possess / UnPossess
// ============================================================

void AMFPetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// UStateTreeAIComponent 会在 OnPossess 后自动感知受控 Pawn，
	// 无需额外操作；等待 RunStateTree() 触发启动。
}

void AMFPetAIController::OnUnPossess()
{
	// Possess 丢失时停止 StateTree，防止逻辑在无 Pawn 时空转。
	if (StateTreeComp && StateTreeComp->IsRunning())
	{
		StateTreeComp->StopLogic(TEXT("Controller unpossessed"));
	}
	Super::OnUnPossess();
}

// ============================================================
// StateTree 控制
// ============================================================

void AMFPetAIController::RunStateTree(UStateTree* InStateTree)
{
	if (InStateTree == nullptr)
	{
		// 传 nullptr 视为"仅停止"请求
		if (StateTreeComp && StateTreeComp->IsRunning())
		{
			StateTreeComp->StopLogic(TEXT("StateTree cleared"));
		}
		return;
	}

	if (!StateTreeComp)
	{
		MF_LOG_ERROR(LogMFSpawnAI, TEXT("%s: StateTreeComp is null — constructor error."), *GetName());
		return;
	}

	// 若已有逻辑在跑，先停止（支持运行时切换 StateTree）
	if (StateTreeComp->IsRunning())
	{
		StateTreeComp->StopLogic(TEXT("Switching to new StateTree"));
	}

	// UStateTreeAIComponent 继承自 UStateTreeComponent，StateTree 属性在父类上。
	// 通过父类指针赋值以绕过派生类的成员可见性问题。
	if (UStateTreeComponent* BaseComp = Cast<UStateTreeComponent>(StateTreeComp))
	{
		BaseComp->SetStateTree(InStateTree);
	}
	StateTreeComp->StartLogic();

	MF_LOG(LogMFSpawnAI, TEXT("%s: StateTree [%s] started."), *GetName(), *InStateTree->GetName());
}

bool AMFPetAIController::IsStateTreeRunning() const
{
	return StateTreeComp && StateTreeComp->IsRunning();
}
