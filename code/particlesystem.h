#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include "defines.h"
#include "particle.h"
#include "forces.h"

class ParticleSystem
{
public:
    ParticleSystem() {}
    virtual ~ParticleSystem() {}

    // phase space
    virtual int  getStateSize()	        const;
    virtual Vecd getState()				const;
    virtual Vecd getDerivative()		const;
    virtual Vecd getSecondDerivative()	const;

    // sets phase space values (pos-vel)
    virtual void setState(const Vecd& state);

    // clear and recompute force accumulators per particle
    virtual void updateForces();

    // individual physical magnitudes getters and setters
    virtual Vecd getPositions()         const;
    virtual Vecd getVelocities()        const;
    virtual Vecd getAccelerations()     const;
    virtual Vecd getPreviousPositions() const;
    virtual void setPositions(const Vecd& pos);
    virtual void setVelocities(const Vecd& vel);
    virtual void setPreviousPositions(const Vecd& pos);

    // particles
    unsigned int getNumParticles() const;
    void addParticle(Particle* p);
    const Particle* getParticle(unsigned int i) const;
    Particle* getParticle(unsigned int i);
    const std::vector<Particle*>& getParticles() const;
    std::vector<Particle*>& getParticles();
    void clearParticles();  // clears vector but does not delete items
    void deleteParticles(); // deletes items and clears vector

    // forces
    void addForce(Force* f);
    unsigned int getNumForces() const;
    const Force* getForce(unsigned int i) const;
    Force* getForce(unsigned int i);
    void clearForces();     // clears vector but does not delete items
    void deleteForces();    // deletes items and clears vector

    // time
    double getTime() const;
    void setTime(double t);
    const double* getTimePointer() const;

    bool isFirstStep() const;
    void markFirstStepDone();
    void resetFirstStep();

protected:
    std::vector<Particle*>	particles;
    std::vector<Force*>		forces;
    double time = 0;
    bool firstStep = true;
};


inline int ParticleSystem::getStateSize() const {
    return Particle::PhaseDimension * particles.size();
}

inline unsigned int ParticleSystem::getNumParticles() const {
    return particles.size();
}

inline unsigned int ParticleSystem::getNumForces() const {
    return forces.size();
}

inline const Particle* ParticleSystem::getParticle(unsigned int i) const {
    return particles[i];
}

inline Particle* ParticleSystem::getParticle(unsigned int i) {
    return particles[i];
}

inline const std::vector<Particle*>& ParticleSystem::getParticles() const {
    return particles;
}

inline std::vector<Particle*>& ParticleSystem::getParticles() {
    return particles;
}

inline const Force* ParticleSystem::getForce(unsigned int i) const {
    return forces[i];
}

inline Force* ParticleSystem::getForce(unsigned int i) {
    return forces[i];
}

inline void ParticleSystem::addParticle(Particle *p) {
    particles.push_back(p);
}

inline void ParticleSystem::addForce(Force *f) {
    forces.push_back(f);
}

inline void ParticleSystem::clearParticles() {
    particles.clear();
}

inline void ParticleSystem::clearForces() {
    forces.clear();
}

inline void ParticleSystem::deleteParticles() {
    for (std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); it++)
        delete (*it);
    particles.clear();
}

inline void ParticleSystem::deleteForces() {
    for (std::vector<Force*>::iterator it = forces.begin(); it != forces.end(); it++)
        delete (*it);
    forces.clear();
}

inline double ParticleSystem::getTime() const {
    return time;
}

inline void ParticleSystem::setTime(double t) {
    time = t;
}

inline const double* ParticleSystem::getTimePointer() const {
    return &time;
}


#endif // PARTICLESYSTEM_H
