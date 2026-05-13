// ----------------------------------------------------
// パーティクル [particle.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-02-04
// Version: 1.0
// ----------------------------------------------------
#ifndef PARTICLE_H
#define PARTICLE_H

#include <DirectXMath.h>

class Particle
{
private:
	DirectX::XMVECTOR m_position{};
	DirectX::XMVECTOR m_velocity{};
	double m_life_time{};
	double m_accumulated_time{};

public:
	Particle(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& velosity, double life_time)
		: m_position(position)
		, m_velocity(velosity)
		, m_life_time(life_time)
	{
	}

	virtual ~Particle() = default;

	virtual bool IsDestroy() const {
		return m_life_time <= 0.0;
	}

	virtual void Update(double elapsed_time) {
		m_accumulated_time += elapsed_time;
		if (m_accumulated_time > m_life_time) {
			Destroy();
		} else {
			m_position = DirectX::XMVectorAdd(m_position, DirectX::XMVectorScale(m_velocity, static_cast<float>(elapsed_time)));
		}
	}
	virtual void Draw() const = 0;

protected:
	void Destroy() {
		m_life_time = 0.0;
	}

	void SetPosition(const DirectX::XMVECTOR& position) {
		m_position = position;
	}
	void SetVelocity(const DirectX::XMVECTOR& velocity) {
		m_velocity = velocity;
	}

	const DirectX::XMVECTOR GetPosition() const {
		return m_position;
	}
	const DirectX::XMVECTOR GetVelocity() const {
		return m_velocity;
	}

	DirectX::XMVECTOR& AddPosition(const DirectX::XMVECTOR& v) {
		m_position = DirectX::XMVectorAdd(m_position, v);
		return m_position;
	}

	DirectX::XMVECTOR& AddVelocity(const DirectX::XMVECTOR& v) {
		m_velocity = DirectX::XMVectorAdd(m_velocity, v);
		return m_velocity;
	}

	float GetAccumulatedTime() const {
		return static_cast<float>(m_accumulated_time);
	}

	float GetLifeTime() const {
		return static_cast<float>(m_life_time);
	}

};

class Emitter // 粒子発生装置
{
private:
	DirectX::XMVECTOR m_position{};
	double m_particles_per_second{};
	double m_accumulated_time = 0.0;
	bool m_is_emmit{};
	size_t m_capacity{}; // 最大パーティクル数
	size_t m_count{};	// 現在のパーティクル数
	Particle** m_particles{ nullptr };

protected:
	virtual Particle* createparticle() = 0;

	const DirectX::XMVECTOR& GetPosition() const {
		return m_position;
	}

	double GetParticlesPerSecond() const {
		return m_particles_per_second;
	}

	double GetAccumulatedTime() const {
		return m_accumulated_time;
	}

	void SetPosition(const DirectX::XMVECTOR& position) {
		m_position = position;
	}

	void SetAccumulatedTime(double time) {
		m_accumulated_time = time;
	}

	void SetParticlesPerSecond(double particles_per_second) {
		m_particles_per_second = particles_per_second;
	}

public:
	Emitter(size_t capacity, const DirectX::XMVECTOR& position, double particles_per_second, bool is_emmit)
		: m_capacity(capacity)
		, m_position(position)
		, m_particles_per_second(particles_per_second)
		, m_is_emmit(is_emmit) 
	{
		m_particles = new Particle* [m_capacity];

		for (int i = 0; i < m_capacity; ++i) {
			m_particles[i] = nullptr;
		}
	}

	virtual ~Emitter() {
		delete[] m_particles;
	}
		

	bool IsEmmit() const {
		return m_is_emmit;
	}

		virtual void Update(double elapsed_time);
		virtual void Draw();

		void Emmit(bool isEmmit) { m_is_emmit = isEmmit; }
		void IsEmmitToggle() { m_is_emmit = !m_is_emmit; }
};

#endif // PARTICLE_H
