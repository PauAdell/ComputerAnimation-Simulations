#ifndef SCENEFLUIDSIM_H
#define SCENEFLUIDSIM_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <list>
#include <vector>
#include "scene.h"
#include "widgetfluidsim.h"   // widget provides gravity via getGravity()
#include "particlesystem.h"
#include "integrators.h"
#include "colliders.h"

class SceneFluidSim : public Scene
{
    Q_OBJECT

public:
    SceneFluidSim();
    virtual ~SceneFluidSim();

    virtual void initialize() override;
    virtual void reset() override;
    virtual void update(double dt) override;
    virtual void paint(const Camera& cam) override;

    virtual void mousePressed(const QMouseEvent* e, const Camera& cam) override;
    virtual void mouseMoved(const QMouseEvent* e, const Camera& cam) override;

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) override {
        bmin = Vec3(-110, -10, -110);
        bmax = Vec3( 110, 120,  110);
    }
    virtual unsigned int getNumParticles() override { return system.getNumParticles(); }

    virtual QWidget* sceneUI() override { return widget; }

public slots:
    void updateSimParams();
    void onSpawnClicked();
    void onAutoEmitToggled(bool on);
    void onSpawnCountChanged(int n);
    void onMaxParticlesChanged(int n);

protected:
    WidgetFluidSim* widget = nullptr;

    QOpenGLShaderProgram* shader = nullptr;
    QOpenGLVertexArrayObject* vaoSphereL = nullptr;
    QOpenGLVertexArrayObject* vaoQuad    = nullptr; // for the 6 planes
    unsigned int numFacesSphereL = 0;

    IntegratorSymplecticEuler integrator;
    ParticleSystem system;
    void spawnParticles(int n);
    void spawnOneRandom();
    double spawnDx = 5.0;
    double uniformRadius = 0.49*spawnDx;
    bool autoEmit = false;
    int spawnPerClick = 50;
    int maxParticles = 1000;
    void spawnGrid(int count);
    void setParticleScaleFromDx(double dx);
    bool rho0Frozen = false;

    ForceConstAcceleration* fGravity = nullptr;

    // Six planes that form a box (normals point inward)
    ColliderPlane planeLeft, planeRight, planeBottom, planeTop, planeBack, planeFront;

    // Box parameters
    Vec3 boxCenter;   // center of the container
    Vec3 boxSize;     // size along X,Y,Z

    // sim params
    double kBounce = 0.5;
    double kFriction = 0.1;
    double emitRate = 200.0;       // particles per second

    // --- SPH params (brute-force) ---
    double sph_h      = 2.0*spawnDx;      // smoothing length
    double sph_rho0   = 1.0;   // rest density
    double sph_k      = 500.0;   // stiffness in state eq p = k (rho - rho0)
    double sph_mu     = 5.0;      // viscosity coefficient
    double sph_mass   = sph_rho0 * spawnDx*spawnDx*spawnDx;      // mass per particle (tune with rho0 & spacing)
    double sph_eps    = 1e-6;     // small epsilon to avoid div-by-zero

    // working buffers (indexed like a vector copy of system.getParticles())
    std::vector<double> rho, pres;
    std::vector<Vec3>   acc;

    // we’ll integrate ourselves (symplectic Euler)
    Vec3 gravity = Vec3(0, -9.81, 0);

    // --- SPH steps: density/pressure/forces ---
    void sphComputeDensityPressure(const std::vector<Particle*>& P);
    void sphComputeForces(const std::vector<Particle*>& P);

    // kernels
    double  W_poly6(double r, double h) const;
    Vec3    gradW_spiky(const Vec3& rvec, double r, double h) const;
    double  lapW_viscosity(double r, double h) const;

    double lastMouseSecs = 0.0;
    bool carryFluidOnDrag = true;
    int mouseX = 0, mouseY = 0;
};

#endif // SCENEFLUIDSIM_H
