// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * ProjectMF 编辑器模块。放各类 PropertyCustomization / 编辑器工具。
 * 当前：注册 FMFItemKey 的下拉选择控件（从 DT_Item 列物品，点选而非手输数字 ItemID）。
 */
class FProjectMFEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
