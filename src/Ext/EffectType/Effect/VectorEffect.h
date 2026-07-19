#pragma once

#include <string>
#include <vector>

#include <GeneralDefinitions.h>

#include "../EffectScript.h"
#include "VectorData.h"

class VectorEffect : public EffectScript
{
public:
	EFFECT_SCRIPT(Vector);

	virtual void Clean() override
	{
		EffectScript::Clean();

		_active = false;
		_elapsedFrames = 0;
		_moveFrame = 0;
		_currentSpeed = 0.0;
		_effectiveTimeStep = 1;
		_initialLocation = {};
		_initialOriginPos = {};
		_pLauncher = nullptr;
		_pTarget = nullptr;
		_pSource = nullptr;
		_totalDuration = 0;
		_randomTargetOffset = {};
		_facingRad = 0.0;
		_currentAngle = 0.0;
		_prevCirclePos = {};
		_currentCircleRadius = 0.0;
		_currentCircleSpeed = 0.0;
		_currentCircleAngle = 0.0;
	}

	virtual void OnStart() override;

	VectorResult GetVectorResult();

	// ========================================================================
	// V2 设计文档 §5 状态变量
	// ========================================================================

	bool ShouldMoveThisFrame() const
	{
		return (_moveFrame % _effectiveTimeStep) == 0;
	}

	void AdvanceFrame()
	{
		_elapsedFrames++;
		_moveFrame++;
	}

#pragma region Save/Load
	template <typename T>
	bool Serialize(T& stream) {
		stream
			.Process(this->_active)
			.Process(this->_elapsedFrames)
			.Process(this->_moveFrame)
			.Process(this->_currentSpeed)
			.Process(this->_effectiveTimeStep)
			.Process(this->_initialLocation)
			.Process(this->_initialOriginPos)
			.Process(this->_pLauncher)
			.Process(this->_pTarget)
			.Process(this->_pSource)
			.Process(this->_totalDuration)
			.Process(this->_randomTargetOffset)
			.Process(this->_facingRad)
			.Process(this->_currentAngle)
			.Process(this->_prevCirclePos)
			.Process(this->_currentCircleRadius)
			.Process(this->_currentCircleSpeed)
			.Process(this->_currentCircleAngle);
		return stream.Success();
	};

	virtual bool Load(ExStreamReader& stream, bool registerForChange) override
	{
		EffectScript::Load(stream, registerForChange);
		return this->Serialize(stream);
	}
	virtual bool Save(ExStreamWriter& stream) const override
	{
		EffectScript::Save(stream);
		return const_cast<VectorEffect*>(this)->Serialize(stream);
	}
#pragma endregion

	bool _active = false;
	int _elapsedFrames = 0;             // 已执行运动帧数
	int _moveFrame = 0;                 // 真实帧计数
	double _currentSpeed = 0.0;         // 当前速度
	int _effectiveTimeStep = 1;         // 有效 TimeStep

	CoordStruct _initialLocation{};     // 初始位置快照
	CoordStruct _initialOriginPos{};    // 初始 Origin 快照（NoUpdate 用）
	ObjectClass* _pLauncher = nullptr;
	ObjectClass* _pTarget = nullptr;
	ObjectClass* _pSource = nullptr;

	int _totalDuration = 0;             // AE 总持续时间（ReachTarget 用）
	CoordStruct _randomTargetOffset{};  // 随机偏移
	double _facingRad = 0.0;           // OnStart 时锁定的朝向弧度（FLH 旋转用）
	double _currentAngle = 0.0;        // MoveTo 模式自增角度（°）
	CoordStruct _prevCirclePos{};      // MoveTo 圆周模式上一帧世界坐标
	double _currentCircleRadius = 0.0; // Circle 模式动态半径
	double _currentCircleSpeed = 0.0;  // Circle 模式动态线速度
	double _currentCircleAngle = 0.0;  // Circle 模式动态角速度
};
