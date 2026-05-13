// ----------------------------------------------------
// パーティクルのテスト [particle_test.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-2-04
// Version: 1.0
// ----------------------------------------------------
#ifndef PARTICLE_TEST_H
#define PARTICLE_TEST_H

#include "particle.h"
#include "texture.h"
#include <random>

class NormalParticle : public Particle
{
private:
	int m_texture_id{ -1 };
	float m_scale{ 1.0f };
	float m_alpha{ 0.0f };
public:
	NormalParticle(int texture_id, const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double life_time)
		: m_texture_id(texture_id)
		, Particle(position, velocity, life_time)
	{
	}

	void Update(double elapsed_time) override;
	void Draw() const override;
};

class NormalEmitter : public Emitter
{
private:
	int m_texture_id{ -1 };
	std::mt19937 m_mt;


public:
	NormalEmitter(size_t capacity, const DirectX::XMVECTOR& position, double particles_per_second, bool is_emmit)
		: Emitter(capacity, position, particles_per_second, is_emmit)
		, m_texture_id(Texture_Load(L"resource/texture/effect000.jpg"))
	{
	}

protected:
	Particle* createparticle() override;
};


class SplashParticle : public Particle
{
private:
	int m_texture_id{ -1 };
	float m_scale{ 1.0f };
	float m_alpha{ 0.0f };
public:
	SplashParticle(int texture_id, const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velocity, double life_time)
		: m_texture_id(texture_id)
		, Particle(position, velocity, life_time)
	{
	}

	void Update(double elapsed_time) override;
	void Draw() const override;
};

class SplashEmitter : public Emitter
{
private:
	int m_texture_id{ -1 };
	std::mt19937 m_mt;


public:
	SplashEmitter(size_t capacity, const DirectX::XMVECTOR& position, double particles_per_second, bool is_emmit)
		: Emitter(capacity, position, particles_per_second, is_emmit)
		, m_texture_id(Texture_Load(L"resource/texture/effect000.jpg"))
	{
	}

	void SetPosition(const DirectX::XMVECTOR& position) {
		Emitter::SetPosition(position);
	}

protected:
	Particle* createparticle() override;
};

#endif // PARTICLE_TEST_H
