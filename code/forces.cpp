#include "forces.h"

void ForceConstAcceleration::apply() {
    for (Particle* p : particles) {
        // F = m * a
        p->force += p->mass * acceleration;   // 'acceleration' is the member set with setAcceleration(...)
    }
}

void ForceDrag::apply() {
    for (Particle* p : particles) {
        const Vec3 v = p->vel;
        const double speed = v.norm();

        // Quadratic + linear drag: Fd = -(k1 * v + k2 * |v| * v)
        const Vec3 fdrag = (v * (-klinear)) + (v * (-kquadratic * speed));
        p->force += fdrag;
    }
}

void ForceSpring::apply()
{
    // we need 2 particles
    if (particles.size() < 2) return;

    Particle* p1 = getParticle1();
    Particle* p2 = getParticle2();

    // current spring vector
    // P2 - P1
    Vec3 d = p2->pos - p1->pos;
    // ||P2 - P1||
    double len = d.norm();
    if (len < 1e-9) return;          // avoid division by zero

    // unit direction from p1 to p2
    Vec3 n = d / len;

    // rest length from the spring object (you have getRestLength())
    double L0 = getRestLength();

    // relative velocity along spring
    Vec3 relV = p2->vel - p1->vel;
    // (v2 - v1) * n
    double relVAlong = relV.dot(n);

    // slide formula:
    // ( ke * (|p2 - p1| - L0) + kd * (v2 - v1)·n ) * n
    double fMag = ks * (len - L0) + kd * relVAlong;
    Vec3 f = fMag * n;

    // apply equal and opposite forces
    p1->force += f;
    p2->force -= f;
}



void ForceGravitation::apply() {
    const size_t n = particles.size();
    if (n < 1) return;

    const double eps2 = 1e-12;   // avoid singularities / zero distance

    if (attractor) {
        const Eigen::Vector3d xa = attractor->pos;
        const double ma = attractor->mass;

        for (size_t i = 0; i < n; ++i) {
            Particle* pi = particles[i];
            if (pi == attractor) continue;           // just in case

            // Pi-Pj
            Eigen::Vector3d r = xa - pi->pos;        // toward attractor
            const double r2_raw = r.squaredNorm();
            if (r2_raw < eps2) continue;

            const Eigen::Vector3d dir = r / std::sqrt(r2_raw); // (Pi-Pj)/||Pi-Pj||
            const double denom = a + b * r2_raw;               // smoothing
            const double mag = (G * ma * pi->mass) / denom;    // G*mi*mj/d^2 (softened)

            pi->force += mag * dir;
        }
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        Particle* pi = particles[i];
        for (size_t j = i + 1; j < n; ++j) {
            Particle* pj = particles[j];

            // Pi-Pj
            Eigen::Vector3d r = pi->pos - pj->pos;
            const double r2_raw = r.squaredNorm();
            if (r2_raw < eps2) continue;

            const Eigen::Vector3d dir = r / std::sqrt(r2_raw);    // (Pi-Pj)/||Pi-Pj||
            const double denom = a + b * r2_raw;                  // softened d^2
            const double mag = (G * pi->mass * pj->mass) / denom; // G*mi*mj/d^2 (softened)

            const Eigen::Vector3d F = mag * dir;
            pi->force += F;
            pj->force -= F;                                         // Newton 3rd law
        }
    }
}
