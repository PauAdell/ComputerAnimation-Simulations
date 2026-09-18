#include "scenefluidsim.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <cmath>

SceneFluidSim::SceneFluidSim() {
    widget = new WidgetFluidSim();
    connect(widget, SIGNAL(updatedParameters()), this, SLOT(updateSimParams()));
    connect(widget, SIGNAL(spawnClicked()),            this, SLOT(onSpawnClicked()));
    connect(widget, SIGNAL(autoEmitToggled(bool)),     this, SLOT(onAutoEmitToggled(bool)));
    connect(widget, SIGNAL(spawnCountChanged(int)),    this, SLOT(onSpawnCountChanged(int)));
    connect(widget, SIGNAL(maxParticlesChanged(int)),  this, SLOT(onMaxParticlesChanged(int)));
}

SceneFluidSim::~SceneFluidSim() {
    if (widget)     delete widget;
    if (shader)     delete shader;
    if (vaoQuad)    delete vaoQuad;
    if (vaoSphereL) delete vaoSphereL;
    if (fGravity)   delete fGravity;
}

void SceneFluidSim::initialize() {
    // shader
    shader = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    // geometry: a quad for planes and a low-res sphere for particles
    Model quad = Model::createQuad();
    vaoQuad = glutils::createVAO(shader, &quad, buffers);

    Model sphereLowres = Model::createIcosphere(1);
    vaoSphereL = glutils::createVAO(shader, &sphereLowres, buffers);
    numFacesSphereL = sphereLowres.numFaces();

    // forces
    fGravity = new ForceConstAcceleration();
    system.addForce(fGravity);

    // container box (centered above ground)
    boxCenter = Vec3(0.0, 40.0, 0.0);
    boxSize   = Vec3(80.0, 80.0, 80.0); // inner dimensions

    // Set planes: n·x = d, choose normals pointing inward
    const double xmin = boxCenter[0] - 0.5*boxSize[0];
    const double xmax = boxCenter[0] + 0.5*boxSize[0];
    const double ymin = boxCenter[1] - 0.5*boxSize[1];
    const double ymax = boxCenter[1] + 0.5*boxSize[1];
    const double zmin = boxCenter[2] - 0.5*boxSize[2];
    const double zmax = boxCenter[2] + 0.5*boxSize[2];

    // normals pointing inward
    const Vec3 nL( 1, 0, 0);
    const Vec3 nR(-1, 0, 0);
    const Vec3 nB( 0, 1, 0);
    const Vec3 nT( 0,-1, 0);
    const Vec3 nK( 0, 0, 1); // back (inward +Z)
    const Vec3 nF( 0, 0,-1); // front (inward -Z)

    // pick a point on each plane, then d = -n·p
    planeLeft .setPlane(nL, -(nL.dot(Vec3(xmin, boxCenter[1], boxCenter[2]))));   // x = xmin
    planeRight.setPlane(nR, -(nR.dot(Vec3(xmax, boxCenter[1], boxCenter[2]))));   // x = xmax
    planeBottom.setPlane(nB, -(nB.dot(Vec3(boxCenter[0], ymin, boxCenter[2]))));  // y = ymin
    planeTop   .setPlane(nT, -(nT.dot(Vec3(boxCenter[0], ymax, boxCenter[2]))));  // y = ymax
    planeBack .setPlane(nK, -(nK.dot(Vec3(boxCenter[0], boxCenter[1], zmin))));   // z = zmin
    planeFront.setPlane(nF, -(nF.dot(Vec3(boxCenter[0], boxCenter[1], zmax))));   // z = zmax
}

void SceneFluidSim::reset() {
    updateSimParams();
    Random::seed(1337);

    fGravity->clearInfluencedParticles();
    system.deleteParticles();
}

void SceneFluidSim::updateSimParams() {
    const double g = widget->getGravity();
    fGravity->setAcceleration(Vec3(0, -g, 0));
    gravity = Vec3(0, -g, 0);

    // tune here if you like via the widget in the future
    kBounce = 0.4;
    kFriction = 0.05;
    emitRate = 50.0;
}
void SceneFluidSim::paint(const Camera& camera) {
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
    shader->bind();

    // camera
    shader->setUniformValue("ProjMatrix", camera.getPerspectiveMatrix());
    shader->setUniformValue("ViewMatrix", camera.getViewMatrix());

    // -----------------------------
    // FIRST: draw particles opaque
    // -----------------------------
    vaoSphereL->bind();
    shader->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
    shader->setUniformValue("matshin", 100.f);
    shader->setUniformValue("matalpha", 1.0f); // opaque spheres

    for (const Particle* particle : system.getParticles()) {
        const Vec3 p = particle->pos;
        const Vec3 c = particle->color;
        const double r = particle->radius;

        QMatrix4x4 M; M.translate(p[0], p[1], p[2]); M.scale(r);
        shader->setUniformValue("ModelMatrix", M);
        shader->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        gl->glDrawElements(GL_TRIANGLES, 3 * numFacesSphereL, GL_UNSIGNED_INT, 0);
    }
    vaoSphereL->release();

    // ------------------------------------------
    // THEN: draw the box as semi-transparent
    // ------------------------------------------
    // Enable alpha blending and disable depth writes.
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glDepthMask(GL_FALSE);

    // (Optional) If you enable face culling elsewhere, either disable it here
    // or draw twice (backfaces then frontfaces). Simplest:
    // gl->glDisable(GL_CULL_FACE);

    vaoQuad->bind();
    shader->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shader->setUniformValue("matshin", 0.0f);

    // mildly see-through walls
    shader->setUniformValue("matalpha", 0.2f);

    auto drawWall = [&](const QMatrix4x4& M, const QVector3D& kd){
        shader->setUniformValue("ModelMatrix", M);
        shader->setUniformValue("matdiff", kd.x(), kd.y(), kd.z());
        gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    };

    // recompute bounds (unchanged)
    const float hx = float(0.5 * boxSize[0]);
    const float hy = float(0.5 * boxSize[1]);
    const float hz = float(0.5 * boxSize[2]);
    const double xmin = boxCenter[0] - hx;
    const double xmax = boxCenter[0] + hx;
    const double ymin = boxCenter[1] - hy;
    const double ymax = boxCenter[1] + hy;
    const double zmin = boxCenter[2] - hz;
    const double zmax = boxCenter[2] + hz;

    // draw 6 walls (same as your code)
    {
        QMatrix4x4 M; M.translate(boxCenter[0], ymin, boxCenter[2]); M.scale(hx, 1.0f, hz);
        drawWall(M, QVector3D(0.85f, 0.85f, 0.85f));
    }
    {
        QMatrix4x4 M; M.translate(boxCenter[0], ymax, boxCenter[2]); M.scale(hx, 1.0f, hz);
        drawWall(M, QVector3D(0.85f, 0.85f, 0.85f));
    }
    {
        QMatrix4x4 M; M.translate(xmin, boxCenter[1], boxCenter[2]); M.rotate(-90.0f, QVector3D(0,0,1)); M.scale(hy, 1.0f, hz);
        drawWall(M, QVector3D(0.75f, 0.85f, 0.75f));
    }
    {
        QMatrix4x4 M; M.translate(xmax, boxCenter[1], boxCenter[2]); M.rotate( 90.0f, QVector3D(0,0,1)); M.scale(hy, 1.0f, hz);
        drawWall(M, QVector3D(0.75f, 0.85f, 0.75f));
    }
    {
        QMatrix4x4 M; M.translate(boxCenter[0], boxCenter[1], zmin); M.rotate( 90.0f, QVector3D(1,0,0)); M.scale(hx, 1.0f, hy);
        drawWall(M, QVector3D(0.75f, 0.75f, 0.85f));
    }
    {
        QMatrix4x4 M; M.translate(boxCenter[0], boxCenter[1], zmax); M.rotate(-90.0f, QVector3D(1,0,0)); M.scale(hx, 1.0f, hy);
        drawWall(M, QVector3D(0.75f, 0.75f, 0.85f));
    }
    vaoQuad->release();

    // restore state
    gl->glDepthMask(GL_TRUE);
    gl->glDisable(GL_BLEND);

    shader->release();
}

void SceneFluidSim::update(double dt) {
    // Emit new particles inside the box; reuse dead ones when possible
    int want = 0;
    if (autoEmit) {
        want += std::max(1, int(std::round(emitRate * dt))); // same rate if you enable it
    }

    // Clamp to remaining capacity
    const int remaining = std::max(0, maxParticles - int(system.getNumParticles()));
    const int toEmit = std::min(want, remaining);

    for (int i = 0; i < toEmit; ++i) spawnOneRandom();

    // --- Build indexable list (unchanged) ---
    std::vector<Particle*> P;
    P.reserve(system.getNumParticles());
    for (Particle* p : system.getParticles()) P.push_back(p);

    if (!P.empty()) {
        // 1) densities & pressure (uses your rho/pres buffers)
        sphComputeDensityPressure(P);                     // fills rho[], pres[]  :contentReference[oaicite:0]{index=0}

        double rmin=1e9, rmax=-1e9, rsum=0;
        for (double r : rho) { rmin=std::min(rmin,r); rmax=std::max(rmax,r); rsum+=r; }
        double ravg = rho.empty()?0:rsum/rho.size();
        // ✅ Optional one-time auto-calibration:
        if (!rho0Frozen) { if (ravg > 0.0) sph_rho0 = ravg; rho0Frozen = true; }

        // (keep your density coloring for debug)
        for (int i = 0; i < (int)P.size(); ++i) {
            double t = (rmax > rmin) ? (rho[i]-rmin)/(rmax-rmin) : 0.0;
            P[i]->color = Vec3(0.6 + 0.4*t, 0.8 + 0.2*t, 1.0);
        }

        //deboug by colouring
        for (int i = 0; i < (int)P.size(); ++i) {
            double t = (rmax > rmin) ? (rho[i] - rmin) / (rmax - rmin) : 0.0;
            P[i]->color = Vec3(0.6 + 0.4*t, 0.8 + 0.2*t, 1.0); // brighter with higher ρ
        }

        // 2) forces (pressure + viscosity + gravity) -> acc[]
        sphComputeForces(P);                              // fills acc[] (already includes gravity)  :contentReference[oaicite:1]{index=1}

        for (int i = 0; i < (int)P.size(); ++i) {
           P[i]->force = P[i]->mass * acc[i];
        }
    }

    Vecd ppos = system.getPositions();
    integrator.step(system, dt);
    system.setPreviousPositions(ppos);

    // Collisions against the 6 planes
    Collision col;
    for (Particle* p : system.getParticles()) {
        if (planeLeft.testCollision(p, col)) planeLeft.resolveCollision(p, col, kBounce, kFriction);
        if (planeRight.testCollision(p, col)) planeRight.resolveCollision(p, col, kBounce, kFriction);
        if (planeBottom.testCollision(p, col)) planeBottom.resolveCollision(p, col, kBounce, kFriction);
        if (planeTop.testCollision(p, col)) planeTop.resolveCollision(p, col, kBounce, kFriction);
        if (planeBack.testCollision(p, col)) planeBack.resolveCollision(p, col, kBounce, kFriction);
        if (planeFront.testCollision(p, col)) planeFront.resolveCollision(p, col, kBounce, kFriction);
    }
}

void SceneFluidSim::mousePressed(const QMouseEvent* e, const Camera&) {
    lastMouseSecs = e->timestamp() * 1e-3; // milliseconds → seconds
    mouseX = e->pos().x();
    mouseY = e->pos().y();
}

inline Vec3 lerpVec3(const Vec3& a, const Vec3& b, double t) {
    return a + (b - a) * t;   // assumes Vec3 has +, -, * by scalar
}

void SceneFluidSim::mouseMoved(const QMouseEvent* e, const Camera& cam) {
    const int dx = e->pos().x() - mouseX;
    const int dy = e->pos().y() - mouseY;
    mouseX = e->pos().x();
    mouseY = e->pos().y();

    Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, cam.getEyeDistance());

    const double nowSecs = e->timestamp() * 1e-3;
    double dtMouse = std::max(1e-4, nowSecs - lastMouseSecs);
    lastMouseSecs = nowSecs;

    if (e->buttons() & Qt::RightButton) {
        // Move the whole box with RMB (no modifiers)
        if (!e->modifiers()) {
            boxCenter += disp;

            // carry all particles by the same displacement (no relative wall penetration)
            for (Particle* p : system.getParticles()) {
                p->pos += disp;
                // optional: impart velocity so they keep moving after you let go
                const double carryFactor = 0.005;                  // only impart a fraction
                Vec3 vBox = disp / std::max(1e-4, dtMouse);
                p->vel = lerpVec3(p->vel, vBox, carryFactor);
            }

            // Update plane equations to follow the box
            // recompute bounds
            const double xmin = boxCenter[0] - 0.5*boxSize[0];
            const double xmax = boxCenter[0] + 0.5*boxSize[0];
            const double ymin = boxCenter[1] - 0.5*boxSize[1];
            const double ymax = boxCenter[1] + 0.5*boxSize[1];
            const double zmin = boxCenter[2] - 0.5*boxSize[2];
            const double zmax = boxCenter[2] + 0.5*boxSize[2];

            // inward normals
            const Vec3 nL( 1, 0, 0), nR(-1, 0, 0), nB( 0, 1, 0),
                       nT( 0,-1, 0), nK( 0, 0, 1), nF( 0, 0,-1);

            // pick a point on each plane, then d = -n·p
            planeLeft .setPlane(nL, -(nL.dot(Vec3(xmin, boxCenter[1], boxCenter[2]))));
            planeRight.setPlane(nR, -(nR.dot(Vec3(xmax, boxCenter[1], boxCenter[2]))));
            planeBottom.setPlane(nB, -(nB.dot(Vec3(boxCenter[0], ymin, boxCenter[2]))));
            planeTop   .setPlane(nT, -(nT.dot(Vec3(boxCenter[0], ymax, boxCenter[2]))));
            planeBack .setPlane(nK, -(nK.dot(Vec3(boxCenter[0], boxCenter[1], zmin))));
            planeFront.setPlane(nF, -(nF.dot(Vec3(boxCenter[0], boxCenter[1], zmax))));
        }
        // With Shift, resize the box uniformly a bit (optional nicety)
        else if (e->modifiers() & Qt::ShiftModifier) {
            boxSize += Vec3(disp[0], disp[1], disp[2]);
            for (int k = 0; k < 3; ++k) if (boxSize[k] < 10.0) boxSize[k] = 10.0;
            // recompute bounds
            const double xmin = boxCenter[0] - 0.5*boxSize[0];
            const double xmax = boxCenter[0] + 0.5*boxSize[0];
            const double ymin = boxCenter[1] - 0.5*boxSize[1];
            const double ymax = boxCenter[1] + 0.5*boxSize[1];
            const double zmin = boxCenter[2] - 0.5*boxSize[2];
            const double zmax = boxCenter[2] + 0.5*boxSize[2];

            // inward normals
            const Vec3 nL( 1, 0, 0), nR(-1, 0, 0), nB( 0, 1, 0),
                       nT( 0,-1, 0), nK( 0, 0, 1), nF( 0, 0,-1);

            // pick a point on each plane, then d = -n·p
            planeLeft .setPlane(nL, -(nL.dot(Vec3(xmin, boxCenter[1], boxCenter[2]))));
            planeRight.setPlane(nR, -(nR.dot(Vec3(xmax, boxCenter[1], boxCenter[2]))));
            planeBottom.setPlane(nB, -(nB.dot(Vec3(boxCenter[0], ymin, boxCenter[2]))));
            planeTop   .setPlane(nT, -(nT.dot(Vec3(boxCenter[0], ymax, boxCenter[2]))));
            planeBack .setPlane(nK, -(nK.dot(Vec3(boxCenter[0], boxCenter[1], zmin))));
            planeFront.setPlane(nF, -(nF.dot(Vec3(boxCenter[0], boxCenter[1], zmax))));
        }
    }
}

void SceneFluidSim::spawnOneRandom() {
    // --- uniform size ---
    // Option A: use a member set elsewhere (recommended)
    double R = uniformRadius;

    // radius-aware interior bounds so we never spawn intersecting the walls
    const Vec3 half = 0.5 * boxSize;
    const Vec3 minB = boxCenter - half + Vec3(R, R, R);
    const Vec3 maxB = boxCenter + half - Vec3(R, R, R);

    // build a snapshot of existing particles for overlap checks
    std::vector<Particle*> P;
    P.reserve(system.getNumParticles());
    for (Particle* q : system.getParticles()) P.push_back(q);

    // rejection sampling to avoid overlaps
    Vec3 pos;
    bool placed = false;
    const int MAX_TRIES = 64;
    for (int tries = 0; tries < MAX_TRIES && !placed; ++tries) {
        const double x = Random::get(minB[0], maxB[0]);
        const double y = Random::get(minB[1], maxB[1]);
        const double z = Random::get(minB[2], maxB[2]);
        pos = Vec3(x, y, z);

        placed = true;
        for (Particle* q : P) {
            const double rr = R + q->radius;              // uniform, but future-proof
            if ((pos - q->pos).norm() < rr) { placed = false; break; }
        }
    }

    if (!placed) {
        // Fallback: drop it at box center (slightly above to avoid immediate floor contact)
        pos = boxCenter + Vec3(0, R, 0);
    }

    // create & register the particle
    Particle* p = new Particle();
    system.addParticle(p);
    if (fGravity) fGravity->addInfluencedParticle(p);    // harmless if unused elsewhere

    p->pos    = pos;
    p->vel    = Vec3(0,0,0);
    p->radius = R;
    p->mass   = sph_mass;                                // keep mass consistent with SPH
    p->color  = Vec3(0.6, 0.8, 1.0);
}


void SceneFluidSim::spawnParticles(int n) {
    const int remaining = std::max(0, maxParticles - int(system.getNumParticles()));
    n = std::min(n, remaining);
    for (int i = 0; i < n; ++i) spawnOneRandom();
}

void SceneFluidSim::onSpawnClicked() {
    // Spawn the current click batch size, respecting the maxParticles cap
    int remaining = std::max(0, maxParticles - int(system.getNumParticles()));
    int toMake = std::min(spawnPerClick, remaining);
    spawnGrid(toMake);
}

void SceneFluidSim::onAutoEmitToggled(bool on) {
    autoEmit = on;
}

void SceneFluidSim::onSpawnCountChanged(int n) {
    if (n < 0) n = 0;
    spawnPerClick = n;
}

void SceneFluidSim::onMaxParticlesChanged(int n) {
    if (n < 0) n = 0;
    maxParticles = n;

    // Want to immediately trim extras when lowering the cap:
    //while (int(system.getNumParticles()) > maxParticles) {
    //    system.deleteLastParticle(); // or a helper you have for removing particles
    //}
}

double SceneFluidSim::W_poly6(double r, double h) const {
    if (r >= 0.0 && r <= h) {
        const double x = (h*h - r*r);
        const double coeff = 315.0 / (64.0 * M_PI * std::pow(h, 9));
        return coeff * x*x*x;
    }
    return 0.0;
}

Vec3 SceneFluidSim::gradW_spiky(const Vec3& rvec, double r, double h) const {
    if (r > 0.0 && r <= h) {
        const double coeff = -45.0 / (M_PI * std::pow(h, 6));
        const double s = coeff * std::pow(h - r, 2) / r;
        return rvec * s; // rvec normalized within factor s
    }
    return Vec3(0,0,0);
}

double SceneFluidSim::lapW_viscosity(double r, double h) const {
    if (r >= 0.0 && r <= h) {
        const double coeff = 45.0 / (M_PI * std::pow(h, 6));
        return coeff * (h - r);
    }
    return 0.0;
}

// -------------- SPH: density & pressure --------------
void SceneFluidSim::sphComputeDensityPressure(const std::vector<Particle*>& P) {
    const int N = int(P.size());
    rho.assign(N, 0.0);
    pres.assign(N, 0.0);

    // density
    for (int i = 0; i < N; ++i) {
        const Vec3& xi = P[i]->pos;
        double rhoi = 0.0;
        for (int j = 0; j < N; ++j) {
            const Vec3& xj = P[j]->pos;
            const Vec3 rij = xi - xj;
            const double r = rij.norm();
            rhoi += sph_mass * W_poly6(r, sph_h);
        }
        rho[i] = std::max(rhoi, sph_eps);
    }

    // pressure: p_i = k * max(0, rho_i - rho0)
    for (int i = 0; i < N; ++i) {
        const double p = sph_k * std::max(0.0, rho[i] - sph_rho0);
        pres[i] = p;
    }
}

// -------------- SPH: forces (pressure + viscosity) --------------
void SceneFluidSim::sphComputeForces(const std::vector<Particle*>& P) {
    const int N = int(P.size());
    acc.assign(N, Vec3(0,0,0));

    for (int i = 0; i < N; ++i) {
        const Vec3& xi = P[i]->pos;
        const Vec3& vi = P[i]->vel;

        Vec3 fpress(0,0,0);
        Vec3 fvisc(0,0,0);

        for (int j = 0; j < N; ++j) {
            if (i == j) continue;

            const Vec3 rij = xi - P[j]->pos;
            const double r = rij.norm();
            if (r > sph_h) continue;

            // Pressure force (Müller et al.)
            // a_i^press = - sum_j m_j * (P_i/(rho_i^2) + P_j/(rho_j^2)) * gradW_spiky
            const Vec3  gradW = gradW_spiky(rij, r, sph_h);
            const double term = (pres[i] / (rho[i]*rho[i] + sph_eps)) +
                                (pres[j] / (rho[j]*rho[j] + sph_eps));
            fpress += gradW * (-sph_mass * term);

            // Viscosity force
            // a_i^visc = mu * sum_j m_j * (v_j - v_i)/rho_j * lapW_viscosity
            const Vec3 vij = P[j]->vel - vi;
            const double lap = lapW_viscosity(r, sph_h);
            fvisc += vij * (sph_mu * sph_mass * lap / (rho[j] + sph_eps));
        }

        // total acceleration = (fpress + fvisc) + gravity
        acc[i] = fpress + fvisc + gravity;
    }
}

void SceneFluidSim::spawnGrid(int count)
{
    if (count <= 0) return;

    // radius-aware interior box
    const double R = uniformRadius;
    const Vec3 minB = boxCenter - 0.5 * boxSize + Vec3(R, R, R);
    const Vec3 maxB = boxCenter + 0.5 * boxSize - Vec3(R, R, R);

    // grid size (cube-ish)
    int n = std::max(1, count);
    int nx = std::ceil(std::cbrt(double(n)));
    int ny = nx;
    int nz = nx;

    // total span of the grid with the current spacing
    auto span = [&](int m){ return (m <= 1) ? 0.0 : (m - 1) * spawnDx; };
    Vec3 spanG(span(nx), span(ny), span(nz));

    // if the grid doesn't fit, shrink dx just enough to fit
    Vec3 avail = maxB - minB;
    double sx = (nx > 1) ? avail[0] / (nx - 1) : spawnDx;
    double sy = (ny > 1) ? avail[1] / (ny - 1) : spawnDx;
    double sz = (nz > 1) ? avail[2] / (nz - 1) : spawnDx;
    double fitDx = std::min({spawnDx, sx, sy, sz});

    if (fitDx < spawnDx) {
        // keep parameters consistent when we shrink spacing to fit
        setParticleScaleFromDx(fitDx);
        // recompute span after dx change
        spanG = Vec3(span(nx), span(ny), span(nz));
    }

    // grid origin so it’s centered
    Vec3 origin = boxCenter - 0.5 * spanG;

    int made = 0;
    for (int iz = 0; iz < nz && made < count; ++iz)
    for (int iy = 0; iy < ny && made < count; ++iy)
    for (int ix = 0; ix < nx && made < count; ++ix) {
        Vec3 pos = origin + Vec3(ix * spawnDx, iy * spawnDx, iz * spawnDx);

        // clamp just in case of floating point drift
        for (int k = 0; k < 3; ++k) {
            pos[k] = std::max(minB[k], std::min(maxB[k], pos[k]));
        }

        Particle* p = new Particle();
        system.addParticle(p);
        if (fGravity) fGravity->addInfluencedParticle(p);

        p->pos    = pos;
        p->vel    = Vec3(0,0,0);
        p->radius = uniformRadius;
        p->mass   = sph_mass;
        p->color  = Vec3(0.6, 0.8, 1.0);

        ++made;
    }
}

void SceneFluidSim::setParticleScaleFromDx(double dx)
{
    // uniform spacing
    spawnDx = dx;

    // uniform radius (leave a little gap so neighbors don’t start overlapped)
    uniformRadius = 0.5 * dx * 0.98;

    // SPH coherence: mass ≈ rho0 * particle volume (here: use dx^3 voxel)
    sph_mass = sph_rho0 * dx * dx * dx;

    // Smoothing length (common starter rule)
    sph_h = 2.0 * dx;
}

