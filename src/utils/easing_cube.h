// ----------------------------------------------------
// イージングキューブ [easing_cube.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-1-23
// Version: 1.0
// ----------------------------------------------------
#ifndef EASING_CUBE_H
#define EASING_CUBE_H

#include <DirectXMath.h>

class EasingCube
{
private:
	double m_AccumlatedTime{};
	double m_MoveTime{ };
	DirectX::XMFLOAT3 m_StartPosition{};
	DirectX::XMFLOAT3 m_EndPosition{};
	bool m_IsStart{};

private:
	double easeOutCubic(double t) const {
		return 1 + (--t) * t * t;
	}

	double easeInBack(double t) const {
		return t * t * (2.70158 * t - 1.70158);
	}

public:
	EasingCube(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, double time) 
		: m_StartPosition(start), m_EndPosition(end), m_MoveTime(time)
	{
	}

	void start() { m_IsStart = true; }
	void Reset() { m_IsStart = false; m_AccumlatedTime = 0.0; }
	void Update(double elapsed_time);
	void Draw() const;
};


#endif // EASING_CUBE_H
