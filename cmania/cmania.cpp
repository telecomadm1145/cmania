// cmania.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Game.h"
#include "TickSource.h"
#include <thread>
#include "BufferController.h"
#include "FpsOverlay.h"
#include "ScreenController.h"
#include "MainMenuScreen.h"
#include "BeatmapManagementService.h"
#include "KeepAwake.h"
#include "BassAudioManager.h"
#include "LogOverlay.h"
#include "VolumeOverlay.h"
#include "RulesetManager.h"
#include "ManiaRuleset.h"
#include "TaikoRuleset.h"
#include "StdRuleset.h"
#include "CatchRuleset.h"
#ifdef _WIN32
#include "Win32ConsoleComponent.h"
#endif
#ifdef __linux__
#include "LinuxConsoleComponent.h"
#endif

Game game;

// cmania 的入口点
int main() {
	auto am = GetBassAudioManager();							  // 获取全局的bass引擎包装
	am->openDevice(AudioManagerExtensions::getDefaultDevice(am)); // 初始化Bass引擎

	EnableConstantDisplayAndPower(true); // 禁止息屏 休眠或者什么东西
#ifdef _WIN32
	game.Use(MakeWin32ConsoleComponent);
#endif
#ifdef __linux__
	game.Use(MakeLinuxConsoleComponent);
#endif
	game.Use(MakeTickSource)
		.Use(MakeBufferController)
		.Use(MakeScreenController)
		.Use(MakeBeatmapManagementService)
		.Use(MakeLogOverlay)
		.Use(MakeFpsOverlay)
		.Use(MakeRulesetManager); // 注入组件依赖
	game.Raise("start"); // 初始化组件
	game.GetFeature<IRulesetManager>().Register(MakeManiaRuleset());
	game.GetFeature<IRulesetManager>().Register(MakeTaikoRuleset());
	game.GetFeature<IRulesetManager>().Register(MakeStdRuleset());
	game.GetFeature<IRulesetManager>().Register(MakeCatchRuleset());

	auto& scr = *MakeMainMenuScreen(); // 构建主屏幕
	game.Raise("navigate", scr);	   // 导航到主屏幕

	while(true){}
}