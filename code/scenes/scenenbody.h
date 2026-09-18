#ifndef SCENENBODY_H
#define SCENENBODY_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "scene.h"
#include "widgetnbody.h"
#include "particlesystem.h"
#include "integrators.h"

class SceneNBody : public Scene
{
    Q_OBJECT

public:
    SceneNBody();
    virtual ~SceneNBody();

    virtual void initialize();
    virtual void reset();
    virtual void update(double dt);
    virtual void paint(const Camera& cam);

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) {
        bmin = Vec3(-150, -20, -150);
        bmax = Vec3( 150,  50,  150);
    }
    virtual unsigned int getNumParticles() { return system.getNumParticles(); }

    virtual QWidget* sceneUI() { return widget; }

protected:
    void updateTrajectoryCoordsBuffer(const std::list<Vec3>& trajectory);
    Integrator* createIntegrator(int type);

protected:
    WidgetNBody* widget = nullptr;

    QOpenGLShaderProgram* shaderPhong = nullptr;
    QOpenGLShaderProgram* shaderLines = nullptr;
    QOpenGLVertexArrayObject* vaoSphere = nullptr;
    QOpenGLVertexArrayObject* vaoTrajectory = nullptr;
    QOpenGLVertexArrayObject* vaoFloor = nullptr;
    QOpenGLBuffer* vboTrajectoryPoints = nullptr;
    unsigned int numSphereFaces = 0;
    const unsigned int MAX_TRAJ_POINTS = 1000;

    Integrator* integrator = nullptr;
    ParticleSystem system;
    bool firstIteration;

    std::vector<std::list<Vec3>> trajectories;
};
#endif // SCENENBODY_H
