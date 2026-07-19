#include "GiftBoxState.h"

#include <Ext/Helper/MathEx.h>
#include <Ext/Helper/Gift.h>
#include <Ext/TechnoType/TechnoStatus.h>
#include <Extension/TechnoExt.h>

bool GiftBoxState::CanOpen()
{
	return IsAlive() && !IsOpen && Timeup() && GetGiftData().Enable;
}


void GiftBoxState::ResetGiftBox()
{
	GiftBoxEntity data = GetGiftData();
	IsOpen = false;
	_delay = GetRandomValue(data.RandomDelay, data.Delay);
	if (_delay > 0)
	{
		_delayTimer.Start(_delay);
	}
}

void GiftBoxState::OnStart()
{
	// Dynamic 模式：自动读取被附加对象的类型名填入 Types
	if (Data.Data.Dynamic && !_dynamicFilled)
	{
		_dynamicFilled = true;
		std::string typeId;
		if (pTechno)
		{
			typeId = pTechno->GetTechnoType()->ID;
		}
		else if (pBullet && pBullet->Type)
		{
			typeId = pBullet->Type->ID;
		}
		if (!typeId.empty())
		{
			Data.Data.Gifts = { typeId };
			Data.EliteData.Gifts = Data.Data.Gifts;
		}
	}
	ResetGiftBox();
}

void GiftBoxState::OnUpdate()
{
#ifdef DEBUG
	StateScript<GiftBoxData>::OnUpdate();
#endif // DEBUG
	bool isElite = false;
	if (pTechno)
	{
		isElite = pTechno->Veterancy.IsElite();
	}
	else if (pBullet && pBullet->Owner)
	{
		isElite = pBullet->Owner->Veterancy.IsElite();
	}
	if (_isElite != isElite && IsAlive() && _delayTimer.Expired())
	{
		ResetGiftBox();
	}
	_isElite = isElite;
}

