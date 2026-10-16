// PersonalProjectCore 模块 — 全局日志分类

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

/**
 * 项目统一日志分类 LogPP
 *
 * 约定：所有模块的业务日志一律使用 LogPP，不得使用 LogTemp
 *       （LogTemp 是引擎内置的全局临时分类，无法按模块过滤，也不受项目日志配置控制）。
 *
 * 用法：UE_LOG(LogPP, Log, TEXT("..."));
 *
 * 分类声明带 PERSONALPROJECTCORE_API 导出宏，供其它模块（跨 DLL）共享该符号；
 * 定义体位于 Private/Framework/PP_Log.cpp。
 */
PERSONALPROJECTCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogPP, Log, All);
