#include "../TechnoStatus.h"

#include <Ext/Helper/Scripts.h>
#include <Ext/EffectType/Effect/BodySpinData.h>

void TechnoStatus::OnUpdate_BodySpin()
{
	if (!pTechno || IsDeadOrInvisible(pTechno))
		return;

	AttachEffect* aem = AEManager();
	if (!aem)
		return;

	BodySpinData* pData = nullptr;
	aem->ForeachChild([&pData](Component* c) {
		auto ae = dynamic_cast<AttachEffectScript*>(c);
		if (ae && ae->IsAlive() && !ae->IsPaused() && ae->AEData.BodySpin.Enable)
		{
			pData = &ae->AEData.BodySpin;
		}
	});
	if (!pData)
		return;

	int speed = pData->Speed;
	int period = pData->SweepPeriod;

	DirStruct dir = pTechno->PrimaryFacing.Current();
	double rad = dir.GetValue();

	if (period <= 0)
	{
		rad += speed;
		while (rad >= 32768) rad -= 65536;
		while (rad < -32768) rad += 65536;
	}
	else
	{
		rad += _spinTime * speed;
		while (rad >= 32768) rad -= 65536;
		while (rad < -32768) rad += 65536;

		if (_spinFlip)
		{
			if (++_spinTime >= period)
				_spinFlip = false;
		}
		else
		{
			if (--_spinTime <= -period)
				_spinFlip = true;
		}
	}

	dir.SetValue(static_cast<short>(rad));
	pTechno->PrimaryFacing.SetCurrent(dir);
}
