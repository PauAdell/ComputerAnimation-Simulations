#include "particlesystem.h"

Vecd ParticleSystem::getState() const {
    Vecd state(this->getStateSize());
    for (unsigned int i = 0; i < particles.size(); i++) {
        state[Particle::PhaseDimension*i    ] = particles[i]->pos[0];
        state[Particle::PhaseDimension*i + 1] = particles[i]->pos[1];
        state[Particle::PhaseDimension*i + 2] = particles[i]->pos[2];
        state[Particle::PhaseDimension*i + 3] = particles[i]->vel[0];
        state[Particle::PhaseDimension*i + 4] = particles[i]->vel[1];
        state[Particle::PhaseDimension*i + 5] = particles[i]->vel[2];
    }
    return state;
}

Vecd ParticleSystem::getDerivative() const {
    Vecd deriv(this->getStateSize());
    for (unsigned int i = 0; i < particles.size(); i++) {
        deriv[Particle::PhaseDimension*i    ] = particles[i]->vel[0];
        deriv[Particle::PhaseDimension*i + 1] = particles[i]->vel[1];
        deriv[Particle::PhaseDimension*i + 2] = particles[i]->vel[2];
        deriv[Particle::PhaseDimension*i + 3] = particles[i]->force[0]/particles[i]->mass;
        deriv[Particle::PhaseDimension*i + 4] = particles[i]->force[1]/particles[i]->mass;
        deriv[Particle::PhaseDimension*i + 5] = particles[i]->force[2]/particles[i]->mass;
    }
    return deriv;
}

Vecd ParticleSystem::getSecondDerivative() const {
    Vecd deriv(this->getStateSize());
    for (unsigned int i = 0; i < particles.size(); i++) {
        deriv[Particle::PhaseDimension*i + 0] = particles[i]->force[0]/particles[i]->mass;
        deriv[Particle::PhaseDimension*i + 1] = particles[i]->force[1]/particles[i]->mass;
        deriv[Particle::PhaseDimension*i + 2] = particles[i]->force[2]/particles[i]->mass;
        deriv[Particle::PhaseDimension*i + 3] = 0;
        deriv[Particle::PhaseDimension*i + 4] = 0;
        deriv[Particle::PhaseDimension*i + 5] = 0;
    }
    return deriv;
}

void ParticleSystem::setState(const Vecd& state) {
    for (unsigned int i = 0; i < particles.size(); i++) {
        particles[i]->pos[0]  = state[Particle::PhaseDimension*i    ];
        particles[i]->pos[1]  = state[Particle::PhaseDimension*i + 1];
        particles[i]->pos[2]  = state[Particle::PhaseDimension*i + 2];
        particles[i]->vel[0]  = state[Particle::PhaseDimension*i + 3];
        particles[i]->vel[1]  = state[Particle::PhaseDimension*i + 4];
        particles[i]->vel[2]  = state[Particle::PhaseDimension*i + 5];
    }
}

void ParticleSystem::updateForces() {
    // clear force accumulators
    for (unsigned int i = 0; i < particles.size(); i++) {
        particles[i]->force = Vec3(0.0, 0.0, 0.0);
    }
    // apply forces
    for (unsigned int i = 0; i < forces.size(); i++) {
        forces[i]->apply();
    }
}

Vecd ParticleSystem::getPositions() const {
    Vecd res(3*this->getNumParticles());
    for (unsigned int i = 0; i < particles.size(); i++) {
        res[3*i  ] = particles[i]->pos[0];
        res[3*i+1] = particles[i]->pos[1];
        res[3*i+2] = particles[i]->pos[2];
    }
    return res;
}

Vecd ParticleSystem::getVelocities() const {
    Vecd res(3*this->getNumParticles());
    for (unsigned int i = 0; i < particles.size(); i++) {
        res[3*i  ] = particles[i]->vel[0];
        res[3*i+1] = particles[i]->vel[1];
        res[3*i+2] = particles[i]->vel[2];
    }
    return res;
}

Vecd ParticleSystem::getAccelerations() const {
    Vecd res(3*this->getNumParticles());
    for (unsigned int i = 0; i < particles.size(); i++) {
        res[3*i  ] = particles[i]->force[0]/particles[i]->mass;
        res[3*i+1] = particles[i]->force[1]/particles[i]->mass;
        res[3*i+2] = particles[i]->force[2]/particles[i]->mass;
    }
    return res;
}

Vecd ParticleSystem::getPreviousPositions() const {
    Vecd res(3*this->getNumParticles());
    for (unsigned int i = 0; i < particles.size(); i++) {
        res[3*i  ] = particles[i]->prevPos[0];
        res[3*i+1] = particles[i]->prevPos[1];
        res[3*i+2] = particles[i]->prevPos[2];
    }
    return res;
}

void ParticleSystem::setPositions(const Vecd& pos) {
    for (unsigned int i = 0; i < particles.size(); i++) {
        particles[i]->pos[0] = pos[3*i    ];
        particles[i]->pos[1] = pos[3*i + 1];
        particles[i]->pos[2] = pos[3*i + 2];
    }
}

void ParticleSystem::setVelocities(const Vecd& vel) {
    for (unsigned int i = 0; i < particles.size(); i++) {
        particles[i]->vel[0] = vel[3*i    ];
        particles[i]->vel[1] = vel[3*i + 1];
        particles[i]->vel[2] = vel[3*i + 2];
    }
}

void ParticleSystem::setPreviousPositions(const Vecd& ppos) {
    for (unsigned int i = 0; i < particles.size(); i++) {
        particles[i]->prevPos[0] = ppos[3*i    ];
        particles[i]->prevPos[1] = ppos[3*i + 1];
        particles[i]->prevPos[2] = ppos[3*i + 2];
    }
}

bool ParticleSystem::isFirstStep() const {
    return firstStep;
}

void ParticleSystem::markFirstStepDone() {
    firstStep = false;
}

void ParticleSystem::resetFirstStep() {
    firstStep = true;
}
