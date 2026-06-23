// Copyright ProjectMF. All Rights Reserved.

#include "MFTimeControlSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UMFTimeControlSubsystem::RequestTimeDilation(UObject* Source, float Dilation)
{
	if (!Source) return;

	Requests.Add(Source, FMath::Max(Dilation, 0.f));
	Recompute();
}

void UMFTimeControlSubsystem::ReleaseTimeDilation(UObject* Source)
{
	if (Requests.Remove(Source) > 0)
	{
		Recompute();
	}
}

void UMFTimeControlSubsystem::Recompute()
{
	float Effective = 1.f;
	bool  bAny      = false;

	for (auto It = Requests.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent(); // 来源已失效，丢弃
			continue;
		}

		// 最极端的慢生效。
		if (!bAny || It->Value < Effective)
		{
			Effective = It->Value;
			bAny      = true;
		}
	}

	CurrentDilation = bAny ? Effective : 1.f;

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, CurrentDilation);
	}
}

void UMFTimeControlSubsystem::Deinitialize()
{
	// 世界销毁前恢复正常时间，避免残留（PIE 反复进出等）。
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}
	Requests.Empty();

	Super::Deinitialize();
}
