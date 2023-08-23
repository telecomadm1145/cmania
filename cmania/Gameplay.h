#pragma once
#include "GameBuffer.h"

class Gameplay {
public:
	/// <summary>
	/// 由外部注入的输入处理器
	/// </summary>
	InputHandler* Handler = 0;
	/// <summary>
	/// 更新 Gameplay 刻
	/// </summary>
	virtual void Update() = 0;
	/// <summary>
	/// 绘制游玩界面
	/// </summary>
	virtual void Render(GameBuffer&) = 0;
	/// <summary>
	/// 获取计分处理器
	/// </summary>
	virtual ScoreProcessorBase* GetScoreProcessor() = 0;

public:
	virtual ~Gameplay() {}
};