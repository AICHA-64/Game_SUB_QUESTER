// ----------------------------------------------------
// パーティクル [particle.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-02-04
// Version: 1.0
// ----------------------------------------------------
#include "particle.h"
using namespace DirectX;

void Emitter::Update(double elapsed_time)
{
	m_accumulated_time += elapsed_time;

	const double sec_per_particle = 1.0 / m_particles_per_second;

	while (m_accumulated_time >= sec_per_particle) {
		if (m_count >= m_capacity) break;
		if(m_is_emmit) {
			m_particles[m_count++] = createparticle();
		}
		m_accumulated_time -= sec_per_particle;
	}

	// 更新処理
	for (int i = 0; i < m_count; i++) {
		m_particles[i]->Update(elapsed_time);
	}

	for (size_t i = m_count; i-- > 0; ) {
		if (m_particles[i]->IsDestroy()) {
			delete m_particles[i];
			m_particles[i] = m_particles[--m_count];
		}
	}
}

void Emitter::Draw()
{
	for (int i = 0; i < m_count; i++) {
		m_particles[i]->Draw();
	}
}
