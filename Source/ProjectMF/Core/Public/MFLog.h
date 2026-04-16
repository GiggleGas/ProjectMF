// Copyright ProjectMF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// ============================================================
// Log Categories
// 各系统独立 Category，便于过滤。
// ============================================================

PROJECTMF_API DECLARE_LOG_CATEGORY_EXTERN(LogMFCatch,     Log, All);  // 抓宠系统
PROJECTMF_API DECLARE_LOG_CATEGORY_EXTERN(LogMFAbility,   Log, All);  // GAS 技能
PROJECTMF_API DECLARE_LOG_CATEGORY_EXTERN(LogMFCharacter, Log, All);  // 角色/玩家
PROJECTMF_API DECLARE_LOG_CATEGORY_EXTERN(LogMFInventory, Log, All);  // 背包系统
PROJECTMF_API DECLARE_LOG_CATEGORY_EXTERN(LogMFSpawnAI,   Log, All);  // AI生成系统

// ============================================================
// Screen Log Helper
// 屏幕打印的实现函数，由 MFLog.cpp 提供。
// ============================================================

namespace MFLog
{
	/**
	 * 在屏幕上打印一条消息（非 Shipping 构建 + CVar 开启时生效）。
	 * 不要直接调用此函数，使用下方宏。
	 *
	 * CVar: MF.Debug.ScreenLog 1/0  (默认 1)
	 */
	PROJECTMF_API void PrintToScreen(float Duration, FColor Color, const FString& Message);
}

// ============================================================
// Macros
// 用法（Format 需用 TEXT()）：
//   MF_LOG         (LogMFCatch, TEXT("Found: %s"), *Name);
//   MF_LOG_WARNING (LogMFCatch, TEXT("Missing config!"));
//   MF_LOG_ERROR   (LogMFCatch, TEXT("Null pointer at %s"), *Context);
// ============================================================

#if NO_LOGGING

// Shipping / NO_LOGGING 构建：彻底移除日志开销
#define MF_LOG(Category, Format, ...)         do {} while(0)
#define MF_LOG_WARNING(Category, Format, ...) do {} while(0)
#define MF_LOG_ERROR(Category, Format, ...)   do {} while(0)

#else // !NO_LOGGING

/** 普通信息（白色，5 秒） */
#define MF_LOG(Category, Format, ...)                                          \
	do {                                                                       \
		UE_LOG(Category, Log, Format, ##__VA_ARGS__);                          \
		::MFLog::PrintToScreen(5.f, FColor::White,                             \
			FString::Printf(Format, ##__VA_ARGS__));                           \
	} while(0)

/** 警告（黄色，7 秒） */
#define MF_LOG_WARNING(Category, Format, ...)                                  \
	do {                                                                       \
		UE_LOG(Category, Warning, Format, ##__VA_ARGS__);                      \
		::MFLog::PrintToScreen(7.f, FColor::Yellow,                            \
			FString::Printf(Format, ##__VA_ARGS__));                           \
	} while(0)

/** 错误（红色，10 秒） */
#define MF_LOG_ERROR(Category, Format, ...)                                    \
	do {                                                                       \
		UE_LOG(Category, Error, Format, ##__VA_ARGS__);                        \
		::MFLog::PrintToScreen(10.f, FColor::Red,                              \
			FString::Printf(Format, ##__VA_ARGS__));                           \
	} while(0)

#endif // NO_LOGGING
