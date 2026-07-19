#pragma once

#include <GeneralStructures.h>

#include <Common/INI/INIConfig.h>

#include "EffectData.h"

class BodySpinData : public EffectData
{
public:
	int Speed = 0;
	int SweepPeriod = 0;

	BodySpinData() : EffectData()
	{ }

	virtual void Read(INIBufferReader* reader) override
	{
		Read(reader, "BodySpin.");
	}

	virtual void Read(INIBufferReader* reader, std::string title) override
	{
		EffectData::Read(reader, title);

		Speed = reader->Get(title + "Speed", Speed);
		SweepPeriod = reader->Get(title + "SweepPeriod", SweepPeriod);

		Enable = Speed != 0 || SweepPeriod > 0;
	}

	template <typename T>
	bool Serialize(T& stream)
	{
		stream
			.Process(Speed)
			.Process(SweepPeriod)
			;
		return true;
	}

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectData::Load(stream, registerForChange);
		return this->Serialize(stream);
	}

	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectData::Save(stream);
		return const_cast<BodySpinData*>(this)->Serialize(stream);
	}
};
