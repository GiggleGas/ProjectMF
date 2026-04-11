// Copyright ProjectMF. All Rights Reserved.

#include "MFLog.h"
#include "Engine/Engine.h"

// ============================================================
// Log Category 定义
// ============================================================

DEFINE_LOG_CATEGORY(LogMFCatch);
DEFINE_LOG_CATEGORY(LogMFAbility);
DEFINE_LOG_CATEGORY(LogMFCharacter);
DEFINE_LOG_CATEGORY(LogMFInventory);

// ============================================================
// CVar: MF.Debug.ScreenLog
// 控制台输入: MF.Debug.ScreenLog 1  开启 / 0  关闭
// 也可在 DefaultEngine.ini [ConsoleVariables] 里预设。
// ============================================================

static int32 GMFScreenLogEnabled = 1;
static FAutoConsoleVariableRef CVarMFScreenLog(
	TEXT("MF.Debug.ScreenLog"),
	GMFScreenLogEnabled,
	TEXT("Enable MF on-screen debug messages. 1=on (default), 0=off.\n")
	TEXT("Stripped entirely in Shipping builds."),
	ECVF_Default
);

// ============================================================
// MFLog::PrintToScreen
// ============================================================

void MFLog::PrintToScreen(float Duration, FColor Color, const FString& Message)
{
#if !UE_BUILD_SHIPPING
	if (GEngine && GMFScreenLogEnabled)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,          // key=-1 → 不覆盖旧消息，每次追加一行
			Duration,
			Color,
			Message);
	}
#endif
}
