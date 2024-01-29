// cmania.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Game.h"
#include "Win32ConsoleComponent.h"
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
#include "RulesetManager.h"

#include "ManiaRuleset.h"
#include "TaikoRuleset.h"
#include "StdRuleset.h"

// cmania 的入口点
int main() {
	auto am = GetBassAudioManager();							  // 获取全局的bass引擎包装
	am->openDevice(AudioManagerExtensions::getDefaultDevice(am)); // 初始化Bass引擎

	EnableConstantDisplayAndPower(true); // 禁止息屏 休眠或者什么东西

	Game game;
	game.Use(MakeWin32ConsoleComponent)
		.Use(MakeTickSource)
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

	auto& scr = *MakeMainMenuScreen(); // 构建主屏幕
	game.Raise("navigate", scr);	   // 导航到主屏幕

	while (true)
		std::this_thread::sleep_for(std::chrono::milliseconds(0x7fffffff)); // 防止控制台退出
}