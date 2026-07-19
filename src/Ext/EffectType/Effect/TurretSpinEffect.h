#pragma once

#include "../EffectScript.h"
#include "TurretSpinData.h"

// 空壳：框架需要此类存在以创建 AE 组件
// 实际旋转逻辑由 TechnoStatus::OnUpdate_TurretSpin 驱动
class TurretSpinEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(TurretSpin);
};
