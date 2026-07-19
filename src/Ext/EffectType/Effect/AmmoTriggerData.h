#pragma once

#include <string>
#include <vector>

#include <GeneralStructures.h>

#include <Common/INI/INIConfig.h>

#include <Ext/EffectType/Effect/EffectData.h>
#include <Ext/Helper/MathEx.h>
#include "AmmoData.h"

enum class AmmoTriggerWho : int
{
	ME = 0,
	SOURCE = 1,
};

template <>
inline bool Parser<AmmoTriggerWho>::TryParse(const char* pValue, AmmoTriggerWho* outValue)
{
	switch (toupper(static_cast<unsigned char>(*pValue)))
	{
	case 'S':
		if (outValue)
		{
			*outValue = AmmoTriggerWho::SOURCE;
		}
		return true;
	case 'M':
		if (outValue)
		{
			*outValue = AmmoTriggerWho::ME;
		}
		return true;
	}
	return false;
}

class AmmoTriggerEntity
{
public:
	bool Enable = false;

	Point2D Range = { 0, -1 };
	int TriggeredTimes = -1;

	double Num = 0;
	AmmoType NumType = AmmoType::Number;
	AmmoTriggerWho NumFrom = AmmoTriggerWho::ME;
	AmmoAction Action = AmmoAction::ADD;
	bool ResetNum = false;

	bool Attach = false;
	std::vector<std::string> AttachEffects{};
	std::vector<double> AttachChances{};
	AmmoTriggerWho AttachTo = AmmoTriggerWho::ME;
	AmmoTriggerWho AttachFrom = AmmoTriggerWho::SOURCE;

	bool Remove = false;
	std::vector<std::string> RemoveEffects{};
	std::vector<int> RemoveEffectsLevel{};
	std::vector<std::string> RemoveEffectsWithMarks{};
	bool RemoveEffectsSkipNext = false;
	AmmoTriggerWho RemoveWho = AmmoTriggerWho::ME;

	virtual void Read(INIBufferReader* reader, std::string title)
	{
		Range = reader->Get(title + "Range", Range);
		TriggeredTimes = reader->Get(title + "TriggeredTimes", TriggeredTimes);

		std::string numStr{ "" };
		numStr = reader->Get(title + "Num", numStr);
		if (IsNotNone(numStr))
		{
			if (std::regex_match(numStr, INIReader::Number))
			{
				int buffer = 0;
				const char* pFmt = "%d";
				if (sscanf_s(numStr.c_str(), pFmt, &buffer) == 1)
				{
					Num = buffer;
					NumType = AmmoType::Number;
				}
			}
			else
			{
				std::string upper = uppercase(numStr);
				if (upper.substr(0, 6) == "MAXAMM")
				{
					NumType = AmmoType::MaxAmmo;
				}
				else
				{
					const char v = upper[0];
					switch (v)
					{
					case 'H':
						NumType = AmmoType::HP;
						break;
					case 'M':
						NumType = AmmoType::MaxHP;
						break;
					case 'A':
						NumType = AmmoType::Ammo;
						break;
					}
				}
			}
		}
		NumFrom = reader->Get(title + "NumFrom", NumFrom);

		Action = reader->Get(title + "Action", Action);
		ResetNum = reader->Get(title + "ResetNum", ResetNum);

		AttachEffects = reader->GetList(title + "AttachEffects", AttachEffects);
		ClearIfGetNone(AttachEffects);
		AttachChances = reader->GetChanceList(title + "AttachChances", AttachChances);
		Attach = !AttachEffects.empty();
		AttachTo = reader->Get(title + "AttachTo", AttachTo);
		AttachFrom = reader->Get(title + "AttachFrom", AttachFrom);

		RemoveEffects = reader->GetList(title + "RemoveEffects", RemoveEffects);
		ClearIfGetNone(RemoveEffects);
		RemoveEffectsLevel = reader->GetList(title + "RemoveEffectsLevel", RemoveEffectsLevel);
		RemoveEffectsWithMarks = reader->GetList(title + "RemoveEffectsWithMarks", RemoveEffectsWithMarks);
		ClearIfGetNone(RemoveEffectsWithMarks);
		Remove = !RemoveEffects.empty() || !RemoveEffectsWithMarks.empty();
		RemoveEffectsSkipNext = reader->Get(title + "RemoveEffectsSkipNext", RemoveEffectsSkipNext);
		RemoveWho = reader->Get(title + "RemoveWho", RemoveWho);

		Enable = Num != 0 || NumType != AmmoType::Number || ResetNum || Attach || Remove;
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Enable)
			.Process(this->Range)
			.Process(this->TriggeredTimes)

			.Process(this->NumType)
			.Process(this->Num)
			.Process(this->Action)
			.Process(this->ResetNum)

			.Process(this->Attach)
			.Process(this->AttachEffects)
			.Process(this->AttachChances)
			.Process(this->AttachTo)
			.Process(this->AttachFrom)

			.Process(this->Remove)
			.Process(this->RemoveEffects)
			.Process(this->RemoveEffectsLevel)
			.Process(this->RemoveEffectsWithMarks)
			.Process(this->RemoveEffectsSkipNext)
			.Process(this->RemoveWho)

			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange)
	{
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const
	{
		return const_cast<AmmoTriggerEntity*>(this)->Serialize(stream);
	}
#pragma endregion
};

class AmmoTriggerData : public EffectData
{
public:
	EFFECT_DATA(AmmoTrigger);

	std::vector<AmmoTriggerEntity> Actions{};

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "AmmoTrigger.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		AmmoTriggerEntity defaultEntity;
		defaultEntity.Read(reader, title);
		if (defaultEntity.Enable)
		{
			Actions.push_back(defaultEntity);
		}
		for (int i = 0; i < 128; i++)
		{
			AmmoTriggerEntity entity{};
			entity.Read(reader, "AmmoTrigger" + std::to_string(i) + ".");
			if (entity.Enable)
			{
				Actions.push_back(entity);
			}
		}

		Enable = !Actions.empty();
	}

#pragma region save/load
	template <typename T>
	bool Serialize(T& stream)
	{
		return stream
			.Process(this->Actions)
			.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectData::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		return const_cast<AmmoTriggerData*>(this)->Serialize(stream);
	}
#pragma endregion
};
