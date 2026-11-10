// P2 安全网 — 帧时间自动化采集（开发/编辑器专用）
//
// 背景：UE 5.7 里命令行 `-fpschart` 不会自动启动图表，`FPSChart` 也只在 `fpschartstop`
// 时才把 CSV 落盘（UEngine::StopFPSChart 是唯一的导出入口，退出进程不会自动 dump）。
// 因此这里提供一个「一条命令跑完预热 + 采样 + 导出 + 退出」的包装，避免人工分两次敲命令。

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

#include "Framework/PP_Log.h"

namespace PPFPSChartCapture
{
	enum class EPhase : uint8
	{
		Idle,
		Warmup,
		Capture
	};

	EPhase Phase = EPhase::Idle;
	double PhaseStartSeconds = 0.0;
	double WarmupSeconds = 5.0;
	double CaptureSeconds = 30.0;
	FString Label = TEXT("PPBaseline");
	FTSTicker::FDelegateHandle TickerHandle;

	UWorld* FindGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	bool Tick(float DeltaTime)
	{
		const double Now = FPlatformTime::Seconds();
		const double Elapsed = Now - PhaseStartSeconds;

		if (Phase == EPhase::Warmup)
		{
			if (Elapsed >= WarmupSeconds)
			{
				UWorld* World = FindGameWorld();
				if (!World)
				{
					UE_LOG(LogPP, Warning, TEXT("[FPSChartCapture] 预热结束但仍没有 Game/PIE 世界，放弃本次采集"));
					Phase = EPhase::Idle;
					TickerHandle.Reset();
					return false;
				}

				GEngine->StartFPSChart(Label, /*bRecordPerFrameTimes=*/true);
				Phase = EPhase::Capture;
				PhaseStartSeconds = Now;
				UE_LOG(LogPP, Log, TEXT("[FPSChartCapture] 开始采样：地图=%s 时长=%.0fs 标签=%s"),
					*World->GetMapName(), CaptureSeconds, *Label);
			}
			return true;
		}

		if (Phase == EPhase::Capture)
		{
			if (Elapsed >= CaptureSeconds)
			{
				UWorld* World = FindGameWorld();
				const FString MapName = World ? World->GetMapName() : FString(TEXT("UnknownMap"));

				GEngine->StopFPSChart(MapName);
				UE_LOG(LogPP, Log, TEXT("[FPSChartCapture] 采样结束，CSV 输出目录：%s"),
					*FPaths::ConvertRelativePathToFull(FPaths::ProfilingDir() / TEXT("FPSChartStats")));

				Phase = EPhase::Idle;
				TickerHandle.Reset();
				GLog->Flush();
				FPlatformMisc::RequestExit(false, TEXT("PP.FPSChartCapture"));
				return false;
			}
			return true;
		}

		return false;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GPPFPSChartCaptureCommand(
	TEXT("pp.FPSChartCapture"),
	TEXT("用法: pp.FPSChartCapture <采集秒数> [预热秒数] [标签]\n")
	TEXT("启动 FPSChart 采样，到时自动 stop 并写 CSV 到 Saved/Profiling/FPSChartStats/<时间戳>/，随后退出进程。"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		using namespace PPFPSChartCapture;

		if (Phase != EPhase::Idle)
		{
			UE_LOG(LogPP, Warning, TEXT("[FPSChartCapture] 已有采集在进行中，忽略本次请求"));
			return;
		}

		CaptureSeconds = Args.IsValidIndex(0) ? FMath::Max(FCString::Atod(*Args[0]), 1.0) : 30.0;
		WarmupSeconds = Args.IsValidIndex(1) ? FMath::Max(FCString::Atod(*Args[1]), 0.0) : 5.0;
		if (Args.IsValidIndex(2) && !Args[2].IsEmpty())
		{
			Label = Args[2];
		}

		Phase = EPhase::Warmup;
		PhaseStartSeconds = FPlatformTime::Seconds();
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), 0.0f);

		UE_LOG(LogPP, Log, TEXT("[FPSChartCapture] 已排队：预热 %.1fs → 采样 %.1fs → 导出并退出"),
			WarmupSeconds, CaptureSeconds);
	}));

#endif // WITH_EDITOR
