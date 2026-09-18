#ifndef SCENETESTINTEGRATORS_H
#define SCENETESTINTEGRATORS_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "scene.h"
#include "widgettestintegrators.h"
#include "colliders.h"
#include "particlesystem.h"
#include "integrators.h"


// Damped Harmonic Oscillator with Driving Force in 1D
// I put this class here since it's only used in this scene as example
class ForceDampedHarmonicOscillator1D;


class SceneTestIntegrators : public Scene
{
    Q_OBJECT

public:
    SceneTestIntegrators();
    virtual ~SceneTestIntegrators();

    virtual void initialize();
    virtual void reset();
    virtual void update(double dt);
    virtual void paint(const Camera& cam);

    virtual void mousePressed(const QMouseEvent* e, const Camera& cam);
    virtual void mouseMoved(const QMouseEvent* e, const Camera& cam);
    virtual void mouseReleased(const QMouseEvent* e, const Camera& cam);

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) {
        bmin = Vec3(-10, -10, -10);
        bmax = Vec3( 10,  10,  10);
    }
    virtual unsigned int getNumParticles() { return 1; }

    virtual QWidget* sceneUI() { return widget; }

protected:
    void updateTrajectoryCoordsBuffer(const std::list<Vec3>& trajectory);
    void appendTrajectoryPoint(std::list<Vec3>& trajectory, const Vec3& p);

protected:
    WidgetTestIntegrators* widget = nullptr;

    QOpenGLShaderProgram* shaderPhong = nullptr;
    QOpenGLShaderProgram* shaderLines = nullptr;
    QOpenGLVertexArrayObject* vaoSphere = nullptr;
    QOpenGLVertexArrayObject* vaoCone = nullptr;
    QOpenGLVertexArrayObject* vaoCylinder = nullptr;
    QOpenGLVertexArrayObject* vaoTrajectory = nullptr;
    QOpenGLBuffer* vboTrajectoryPoints = nullptr;
    unsigned int numFacesSphere, numFacesCylinder, numFacesCone;
    const unsigned int MAX_TRAJ_POINTS = 1000;


    // physic simulation
    ParticleSystem particleSystem;
    Integrator* integrator = nullptr;
    ForceDampedHarmonicOscillator1D* force = nullptr;
    Particle* particle = nullptr;
    bool firstIteration = true;

    // analytic solution
    double time = 0;
    double analytic_x;
    double analytic_v;
    double init_x, init_v;

    // trajectories
    std::list<Vec3> trajectoryAnalytic;
    std::list<Vec3> trajectoryNumerical;
    std::list<Vec3> oscillationAnalytic;
    std::list<Vec3> oscillationNumerical;
};


#endif // SCENETESTINTEGRATORS_H
