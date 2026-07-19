#pragma once

#include <string>
#include <vector>

#include <GeneralDefinitions.h>
#include <AnimClass.h>

#include "../EffectScript.h"
#include "AmmoTriggerData.h"

class AmmoTriggerEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(AmmoTrigger);

	virtual void Clean() override
	{
		EffectScript::Clean();
		_count.clear();
	}

	virtual void OnUpdate() override;
	virtual void OnWarpUpdate() override;

#pragma region Save/Load
	template <typename T>
	bool Serialize(T& stream) {
		return stream
			.Process(this->_count)
			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectScript::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectScript::Save(stream);
		return const_cast<AmmoTriggerEffect*>(this)->Serialize(stream);
	}
#pragma endregion
private:
	void Watch();
	bool CanActive(double num, Point2D range);

	std::map<int, int> _count{};
};
