// Copyright ProjectMF. All Rights Reserved.

#include "ProjectMFEditor.h"
#include "MFItemKeyCustomization.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"

void FProjectMFEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// "MFItemKey" = USTRUCT 名去 F 前缀。
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"MFItemKey",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMFItemKeyCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FProjectMFEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("MFItemKey");
	}
}

IMPLEMENT_MODULE(FProjectMFEditorModule, ProjectMFEditor)
