#include "integrators.h"


void IntegratorEuler::step(ParticleSystem &system, double dt) {
    double t0 = system.getTime();
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd x1 = x0+dt*dx;
    system.setState(x1);
    system.setTime(t0+dt);
    system.updateForces();
}


void IntegratorSymplecticEuler::step(ParticleSystem &system, double dt) {
    double t0 = system.getTime();

    // v_{t+dt} = v_t + dt * a_t
    Vecd v0 = system.getVelocities();
    Vecd a0 = system.getAccelerations();   // F/m per particle
    Vecd v1 = v0 + dt * a0;
    system.setVelocities(v1);

    // p_{t+dt} = p_t + dt * v_{t+dt}
    Vecd p0 = system.getPositions();
    Vecd p1 = p0 + dt * v1;
    system.setPositions(p1);

    // advance time and refresh forces for the next step
    system.setTime(t0 + dt);
    system.updateForces();
}


void IntegratorMidpoint::step(ParticleSystem &system, double dt) {
    const double t0 = system.getTime();
    const Vecd   x0 = system.getState();

    // Compute inital Euler step (k1)
    system.setState(x0);
    system.setTime(t0);
    system.updateForces();
    const Vecd k1 = system.getDerivative();

    // Midpoint state (and time)
    const double tm = t0 + 0.5*dt;
    const Vecd   xm = x0 + 0.5*dt * k1;

    // k2 at midpoint
    system.setState(xm);
    system.setTime(tm);
    system.updateForces();
    const Vecd k2 = system.getDerivative();

    // Full step using k2
    const Vecd x1 = x0 + dt * k2;

    system.setState(x1);
    system.setTime(t0 + dt);
    system.updateForces();
}


void IntegratorRK2::step(ParticleSystem &system, double dt) {
    const double t0 = system.getTime();
    const Vecd   x0 = system.getState();

    // k1 at (t0, x0)
    system.setState(x0);
    system.setTime(t0);
    system.updateForces();
    const Vecd k1 = system.getDerivative();

    // Euler predictor to t0+dt
    const Vecd xE = x0 + dt * k1;

    // k2 at (t0+dt, xE)
    system.setState(xE);
    system.setTime(t0 + dt);
    system.updateForces();
    const Vecd k2 = system.getDerivative();

    // Average the slopes
    const Vecd x1 = x0 + 0.5*dt * (k1 + k2);

    system.setState(x1);
    system.setTime(t0 + dt);
    system.updateForces();
}


void IntegratorRK4::step(ParticleSystem &system, double dt) {
    const double t0 = system.getTime();
    const Vecd   x0 = system.getState();

    // k1 at (t0, x0)
    system.setState(x0);
    system.setTime(t0);
    system.updateForces();
    const Vecd k1 = system.getDerivative();

    // k2 = f(t0 + dt/2, x0 + dt/2 * k1)
    const double th = t0 + 0.5*dt;
    Vecd xtmp = x0 + 0.5*dt * k1;
    system.setState(xtmp);
    system.setTime(th);
    system.updateForces();
    const Vecd k2 = system.getDerivative();

    // k3 = f(t0 + dt/2, x0 + dt/2 * k2)
    xtmp = x0 + 0.5*dt * k2;
    system.setState(xtmp);
    system.setTime(th);
    system.updateForces();
    const Vecd k3 = system.getDerivative();

    // k4 = f(t0 + dt, x0 + dt * k3)
    xtmp = x0 + dt * k3;
    system.setState(xtmp);
    system.setTime(t0 + dt);
    system.updateForces();
    const Vecd k4 = system.getDerivative();

    // advance: x_{n+1} = x_n + dt/6 (k1 + 2k2 + 2k3 + k4)
    const Vecd x1 = x0 + (dt/6.0) * (k1 + 2.0*k2 + 2.0*k3 + k4);

    system.setState(x1);
    system.setTime(t0 + dt);
    system.updateForces();
}


void IntegratorVerlet::step(ParticleSystem& system, double dt)
{
    // Damping factor on the Verlet displacement term (optional)
    const double k = 0.99;
    const double t0 = system.getTime();

    system.setTime(t0);
    system.updateForces();

    // Positions, velocities, accelerations at t0
    Vecd p_t = system.getPositions();        // P(t)
    Vecd v_t = system.getVelocities();       // V(t)
    Vecd a0  = system.getAccelerations();    // A(t) = F(t)/m

    // Previous positions P(t - dt): synthesize on the very first step
    Vecd p_prev;
    if (system.isFirstStep()) {
        // Taylor backstep: P(t - dt) ≈ P(t) - V(t) dt + 0.5 A(t) dt^2
        p_prev = p_t - v_t * dt + 0.5 * a0 * (dt * dt);
        system.setPreviousPositions(p_prev);
        system.markFirstStepDone();
    } else {
        p_prev = system.getPreviousPositions();
    }

    // Verlet position update
    // P_{t+dt} = P_t + k * (P_t - P_{t-dt}) + (dt^2) * (F/m)
    Vecd p_next = p_t + k * (p_t - p_prev) + (dt * dt) * a0;

    // Velocity estimate (staggered in position-Verlet, fine for debug/IO)
    Vecd v_next = (p_next - p_t) / dt;

    // --- Commit new state ---
    // Shift history: current -> previous, next -> current
    system.setPreviousPositions(p_t);
    system.setPositions(p_next);
    system.setVelocities(v_next);
    system.setTime(t0 + dt);
    system.updateForces();
}
