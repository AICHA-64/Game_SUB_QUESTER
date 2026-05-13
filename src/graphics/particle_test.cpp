// ----------------------------------------------------
// パーティクルのテスト [particle_test.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-2-04
// Version: 1.0
// ----------------------------------------------------
#include "particle_test.h"
#include "billboard.h"
using namespace DirectX;

double easeInCirc(double t)
{
	return 1 - sqrt(1 - t);
}

void NormalParticle::Update(double elapsed_time)
{
	float ratio = std::min(GetAccumulatedTime() / GetLifeTime(), 1.0f);

	m_scale = 1.0f - ratio * 2.0f; // 時間経過で大きくなる
	m_alpha = (1.0f - ratio); // 時間経過で透明になる

	AddPosition(GetVelocity() * static_cast<float>(elapsed_time));

	Particle::Update(elapsed_time);
}

void NormalParticle::Draw() const
{
	XMFLOAT3 position{};
	XMStoreFloat3(&position, GetPosition());

	Billboard_Draw(m_texture_id, position, { m_scale, m_scale }, XMFLOAT4{0.3f, 1.0f, 1.0f, m_alpha});
}

Particle* NormalEmitter::createparticle()
{
	std::uniform_real_distribution<float> m_dir_dist{ -DirectX::XM_PI, DirectX::XM_PI };
	std::uniform_real_distribution<float> m_length_dist{ 0.0f, 1.0f };


	std::uniform_real_distribution<float> m_speed_dist{ 1.0f, 5.0f };
	std::uniform_real_distribution<float> m_lifetime_dist{ 0.5f, 1.0f };

	float angle = m_dir_dist(m_mt);


	XMVECTOR direction{ cosf(angle), sinf(angle), sinf(angle)};
	XMVECTOR velocity{ direction * m_speed_dist(m_mt) };

	return new NormalParticle(m_texture_id, GetPosition(), velocity, m_lifetime_dist(m_mt));
}

//------------------------------------------------------------------------------

void SplashParticle::Update(double elapsed_time)
{
	float ratio = std::min(GetAccumulatedTime() / GetLifeTime(), 1.0f);

	// 根元を大きくしつつ、終端は元と同じように 0 になるように調整
	m_scale = (1.0f - ratio) * 1.0f;
	m_alpha = 1.0f - static_cast<float>(easeInCirc(ratio)); // 時間経過で透明になる

	AddPosition(GetVelocity() * static_cast<float>(elapsed_time));
	AddVelocity(XMVECTOR{ 0.0f, -9.8f, 0.0f } *static_cast<float>(elapsed_time));

	Particle::Update(elapsed_time);
}

void SplashParticle::Draw() const
{
	XMFLOAT3 position{};
	XMStoreFloat3(&position, GetPosition());

	Billboard_Draw(m_texture_id, position, { m_scale, m_scale }, XMFLOAT4{ 0.6f, 1.0f, 1.0f, m_alpha });
}

Particle* SplashEmitter::createparticle()
{
	std::uniform_real_distribution<float> m_dir_dist{ -DirectX::XM_PI, DirectX::XM_PI };
	std::uniform_real_distribution<float> m_length_dist{ 0.0f, 2.0f };
	std::uniform_real_distribution<float> m_speed_dist{ 5.0f, 8.0f };
	std::uniform_real_distribution<float> m_lifetime_dist{ 0.5f, 1.5f };

	float angle = m_dir_dist(m_mt);
	// float ey = XMVectorGetY(GetPosition());
	XMVECTOR c{ cosf(angle), 0.0f, sinf(angle) };
	c += GetPosition();
	c *= m_length_dist(m_mt);
	c += { 0.0f, 10.0f, 0.0f };
	XMVECTOR direction{ c - GetPosition() };
	direction = XMVector3Normalize(direction);
	XMVECTOR velocity{ direction * m_speed_dist(m_mt) };

	return new SplashParticle(m_texture_id, GetPosition(), velocity, m_lifetime_dist(m_mt));
}
