#pragma once
#include "Ruleset.h"
class RulesetRendererBase {
public:
	virtual ~RulesetRendererBase() {
	}
};
/// <summary>
/// 表示了一个 Ruleset 的渲染器.
/// </summary>
/// <typeparam name="HitObject"></typeparam>
template <typename Ruleset>
class RulesetRenderer : RulesetRendererBase {
public:
	virtual void RenderRuleset(Ruleset& ruleset, GameBuffer& buffer) = 0;
};