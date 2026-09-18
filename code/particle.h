#ifndef PARTICLE_H
#define PARTICLE_H

#include "defines.h"

class Particle
{
public:

    static const int PhaseDimension = 6;

    Vec3 pos, prevPos;
    Vec3 vel;
    Vec3 force;
    double mass;
    double radius = 1.0;
    double life   = 0.0;
    Vec3 color    = Vec3(1, 1, 1);
    unsigned int id = 0;

    Particle() {
        pos	    = Vec3(0.0, 0.0, 0.0);
        vel	    = Vec3(0.0, 0.0, 0.0);
        force   = Vec3(0.0, 0.0, 0.0);
        prevPos = pos;
        mass    = 1.0;
    }

    Particle(const Vec3& p, const Vec3& v, float m) {
        pos		= p;
        vel		= v;
        force	= Vec3(0.0, 0.0, 0.0);
        prevPos = pos;
        mass	= m;
    }

    Particle(const Particle& p) {
        id      = p.id;
        pos     = p.pos;
        vel     = p.vel;
        force   = p.force;
        prevPos = p.pos;
        mass    = p.mass;
        color   = p.color;
        radius  = p.radius;
        life    = p.life;
    }

    ~Particle() {
    }
};


#endif // PARTICLE_H
