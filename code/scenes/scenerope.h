#ifndef SCENEROPE_H
#define SCENEROPE_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "scene.h"
#include "widgetrope.h"
#include "particlesystem.h"
#include "integrators.h"
#include "colliders.h"


class SceneRope : public Scene
{
    Q_OBJECT

public:
    SceneRope();
    virtual ~SceneRope();

    virtual void initialize();
    virtual void reset();
    virtual void update(double dt);
    virtual void paint(const Camera& cam);

    virtual void mousePressed(const QMouseEvent* e, const Camera& cam);
    virtual void mouseMoved(const QMouseEvent* e, const Camera& cam);
    virtual void mouseReleased(const QMouseEvent* e, const Camera& cam);

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) {
        bmin = Vec3(-100, -100, -100);
        bmax = Vec3( 100,  100,  100);
    }
    virtual unsigned int getNumParticles() { return system.getNumParticles(); }

    virtual QWidget* sceneUI() { return widget; }

public slots:
    void updateSimParams();

protected:
    WidgetRope* widget = nullptr;

    QOpenGLShaderProgram* shaderPhong = nullptr;
    QOpenGLShaderProgram* shaderLines = nullptr;
    QOpenGLVertexArrayObject* vaoSphereS = nullptr;
    QOpenGLVertexArrayObject* vaoSphereL = nullptr;
    QOpenGLVertexArrayObject* vaoCube    = nullptr;
    QOpenGLVertexArrayObject* vaoRope    = nullptr;
    QOpenGLBuffer* vboRope = nullptr;
    unsigned int numFacesSphereS = 0, numFacesSphereL = 0;
    bool showParticles;

    Integrator* integrator = nullptr;
    ParticleSystem system;
    ForceConstAcceleration* fGravity = nullptr;
    std::vector<Particle*> particles;
    std::vector<ForceSpring*> springs;
    double deltaTime;

    bool checkCollisions;
    double colBounce = 0.01;
    double colFriction = 0.05;
    double particleRadius = 1;

    double ropeLength;
    double edgeLength;
    int numParticles;
    bool anchoredEnd;
    Particle *anchor;
    int selectedParticle = -1;
    Vec3 cursorWorldPos;

    ColliderSphere colliderBall;

    int grabX, grabY;
};

#endif // SCENEROPE_H
