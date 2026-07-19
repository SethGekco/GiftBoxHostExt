#pragma once

#include <Common/Components/ScriptComponent.h>

/// 炮塔旋转：暴力覆盖 SecondaryFacing，数据来自 AE
class TurretSpin : public TechnoScript
{
public:
	TECHNO_SCRIPT(TurretSpin);

	virtual void Clean() override
	{
		TechnoScript::Clean();
		_turretTime = 0;
		_turretFlip = true;
	}

	virtual void OnUpdate() override;
	virtual void OnPut(CoordStruct* pCoord, DirType dirType) override;

private:
	int _turretTime = 0;
	bool _turretFlip = true;
};

