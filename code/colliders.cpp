#include "colliders.h"
#include <cmath>
#include <algorithm> // for std::min/std::max

// small numerical slop to avoid re-penetration
static constexpr double kEps = 1e-6;

static inline double dot(const Vec3& a, const Vec3& b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

/*
 * Generic function for collision response from contact plane
 */
void Collider::resolveCollision(Particle* p, const Collision& col, double kElastic, double kFriction) const
{
    Eigen::Vector3d n = col.normal.normalized();
    double h = dot(n, p->pos - col.position);

    // Only resolve if the particle is penetrating the plane
    if (h < kEps)
    {
        // --- 1. Correct position (push out of plane based on kElastic)
        p->pos -= (1.0 + kElastic) * h * n;

        // --- 2. Decompose velocity
        Eigen::Vector3d v = p->vel;
        Eigen::Vector3d vN = dot(v, n) * n;     // Normal component
        Eigen::Vector3d vT = v - vN;            // Tangential component

        // --- 3. Modify velocity
        //           VNt+dt     +         VTt+dt
        p->vel = -kElastic * vN + (1.0 - kFriction) * vT;
    }
}

/*
 * Plane (n·x + d = 0). We consider "inside" as the negative half-space.
 */
bool ColliderPlane::isInside(const Particle* p) const
{
    // Signed distance from center to plane
    Vec3 n = planeN.normalized();
    double s = dot(n, p->pos) + planeD;

    // "Inside" if the particle center is beyond the plane by more than its radius
    return (s < -p->radius);
}

bool ColliderPlane::testCollision(const Particle* p, Collision& colInfo) const
{
    const Vec3& n = planeN;  // assume normalized
    double d = planeD;

    Vec3 p0 = p->prevPos;    // start position
    Vec3 p1 = p->pos;        // predicted position
    double r = p->radius;

    double s0 = dot(n, p0) + d;
    double s1 = dot(n, p1) + d;

    // Sphere crosses the plane during this frame?
    if (s0 > r && s1 <= r)
    {
        double denom = s0 - s1;
        if (fabs(denom) < 1e-8) return false; // moving parallel

        // time of impact (when the sphere's surface just touches)
        double tau = (s0 - r) / denom;

        // center of sphere at impact
        Vec3 centerAtHit = p0 + tau * (p1 - p0);

        // (optional) contact point on plane, if you ever need it:
        // Vec3 contactPoint = centerAtHit - n * r;

        // fill collision info
        colInfo.normal   = n;
        colInfo.position = centerAtHit; // <-- center of particle, NOT the surface
        return true;
    }
    return false;
}


/*
 * Sphere collider
 */
bool ColliderSphere::isInside(const Particle* p) const
{
    double d = (p->pos - center).norm();
    // add its own radius from the boundary
    return (d + p->radius <= radius);
}

bool ColliderSphere::testCollision(const Particle* p, Collision& colInfo) const
{
    // particle motion
    Vec3 p0 = p->prevPos;        // start of step
    Vec3 p1 = p->pos;            // end of step (predicted)
    Vec3 v  = p1 - p0;           // motion vector

    // sphere collider data
    Vec3 c  = this->center;
    double rMoving  = p->radius;
    double rStatic  = this->radius;
    double R = rMoving + rStatic;   // sum of radii

    // vector from collider center to particle start
    Vec3 m = p0 - c;

    double a = v.dot(v);
    double b = 2.0 * m.dot(v);
    double c_q = m.dot(m) - R * R;

    // If no movement: just check current overlap
    if (std::abs(a) < 1e-12)
    {
        // particle didn't move → fall back to static test
        if (c_q <= 0.0) // already overlapping
        {
            // we consider this a collision at t=0
            Vec3 pHit = p0;   // center
            Vec3 n = (pHit - c).normalized();
            colInfo.normal   = n;
            colInfo.position = pHit;
            return true;
        }
        return false;
    }

    // Quadratic discriminant
    double disc = b*b - 4.0*a*c_q;
    if (disc < 0.0)
        return false; // no real intersection

    double sqrtDisc = std::sqrt(disc);

    // two possible times
    double t1 = (-b - sqrtDisc) / (2.0 * a);
    double t2 = (-b + sqrtDisc) / (2.0 * a);

    // we want the earliest time in [0,1]
    double tHit = std::numeric_limits<double>::infinity();

    if (t1 >= 0.0 && t1 <= 1.0) tHit = t1;
    else if (t2 >= 0.0 && t2 <= 1.0) tHit = t2;

    if (!std::isfinite(tHit))
        return false; // collision happens outside this frame

    // particle center at impact
    Vec3 pHit = p0 + tHit * v;

    // normal from collider to particle at impact
    Vec3 n = (pHit - c).normalized();

    colInfo.normal   = n;
    colInfo.position = pHit;   // <-- center at impact, like you wanted

    return true;
}

/*
 * AABB collider (axis-aligned)
 */
bool ColliderAABB::isInside(const Particle* p) const
{
    // Center must sit at least 'radius' away from each face (strictly inside the shrunk box)
    return (p->pos.x() > bmin.x() + p->radius && p->pos.x() < bmax.x() - p->radius &&
            p->pos.y() > bmin.y() + p->radius && p->pos.y() < bmax.y() - p->radius &&
            p->pos.z() > bmin.z() + p->radius && p->pos.z() < bmax.z() - p->radius);
}

bool ColliderAABB::testCollision(const Particle* p, Collision& colInfo) const
{
    // particle motion (CCD)
    Vec3 p0 = p->prevPos;        // start of step
    Vec3 p1 = p->pos;            // end of step (predicted)
    Vec3 v  = p1 - p0;           // motion

    double r = p->radius;

    // expand AABB by sphere radius
    Vec3 minE = bmin - Vec3(r, r, r);
    Vec3 maxE = bmax + Vec3(r, r, r);

    // slab method
    double tEnter = 0.0;
    double tExit  = 1.0;

    // we'll also remember which axis we entered on, to get normal
    int hitAxis = -1;

    // for each axis
    for (int axis = 0; axis < 3; ++axis)
    {
        double p0a = p0[axis];
        double va  = v[axis];
        double minA = minE[axis];
        double maxA = maxE[axis];

        if (std::abs(va) < 1e-12)
        {
            // moving parallel to this axis' slabs
            if (p0a < minA || p0a > maxA)
                return false; // outside, never enters
            // else: stays inside this slab for entire segment → do nothing
        }
        else
        {
            double t1 = (minA - p0a) / va;
            double t2 = (maxA - p0a) / va;
            double tMinAxis = std::min(t1, t2);
            double tMaxAxis = std::max(t1, t2);

            // narrow the interval
            if (tMinAxis > tEnter)
            {
                tEnter = tMinAxis;
                hitAxis = axis;          // we entered through this axis
            }
            if (tMaxAxis < tExit)
            {
                tExit = tMaxAxis;
            }

            // early out
            if (tEnter > tExit)
                return false;
        }
    }

    // check if the intersection happens within the frame
    if (tEnter < 0.0 || tEnter > 1.0)
        return false;

    // particle center at impact
    Vec3 pHit = p0 + tEnter * v;

    // compute normal from hitAxis
    Vec3 n(0.0, 0.0, 0.0);
    if (hitAxis == 0)
    {
        // x-axis slab
        // determine if we hit min or max side:
        double hitX = pHit.x();
        // if we came from left to right, we hit min side
        if (std::abs(hitX - minE.x()) < 1e-5)      n = Vec3(-1, 0, 0);
        else if (std::abs(hitX - maxE.x()) < 1e-5) n = Vec3( 1, 0, 0);
        else
        {
            // fallback: use sign of velocity
            n = (v.x() > 0.0) ? Vec3(-1,0,0) : Vec3(1,0,0);
        }
    }
    else if (hitAxis == 1)
    {
        double hitY = pHit.y();
        if (std::abs(hitY - minE.y()) < 1e-5)      n = Vec3(0,-1,0);
        else if (std::abs(hitY - maxE.y()) < 1e-5) n = Vec3(0, 1,0);
        else n = (v.y() > 0.0) ? Vec3(0,-1,0) : Vec3(0,1,0);
    }
    else if (hitAxis == 2)
    {
        double hitZ = pHit.z();
        if (std::abs(hitZ - minE.z()) < 1e-5)      n = Vec3(0,0,-1);
        else if (std::abs(hitZ - maxE.z()) < 1e-5) n = Vec3(0,0, 1);
        else n = (v.z() > 0.0) ? Vec3(0,0,-1) : Vec3(0,0,1);
    }
    else
    {
        // shouldn't happen, but in case:
        n = Vec3(0,1,0);
    }

    // fill collision info
    colInfo.normal   = n;
    colInfo.position = pHit;     // <-- center at impact (your requirement)
    return true;
}
