#ifndef SCENECLOTH_H
#define SCENECLOTH_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "scene.h"
#include "widgetcloth.h"
#include "particlesystem.h"
#include "integrators.h"
#include "colliders.h"


class SceneCloth : public Scene
{
    Q_OBJECT

public:
    SceneCloth();
    virtual ~SceneCloth();

    virtual void initialize();
    virtual void reset();
    virtual void update(double dt);
    virtual void paint(const Camera& cam);

    virtual void mousePressed(const QMouseEvent* e, const Camera& cam);
    virtual void mouseMoved(const QMouseEvent* e, const Camera& cam);
    virtual void mouseReleased(const QMouseEvent* e, const Camera& cam);
    virtual void keyPressed(const QKeyEvent* e, const Camera& cam);

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) {
        bmin = Vec3(-100, -100, -100);
        bmax = Vec3( 100,  100,  100);
    }
    virtual unsigned int getNumParticles() { return system.getNumParticles(); }

    virtual QWidget* sceneUI() { return widget; }

public slots:
    void updateSprings();
    void updateSimParams();
    void freeAnchors();

protected:
    // ui
    WidgetCloth* widget = nullptr;

    // opengl & render
    QOpenGLShaderProgram* shaderPhong = nullptr;
    QOpenGLShaderProgram* shaderCloth = nullptr;
    QOpenGLVertexArrayObject* vaoSphereS = nullptr;
    QOpenGLVertexArrayObject* vaoSphereL = nullptr;
    QOpenGLVertexArrayObject* vaoCube    = nullptr;
    QOpenGLVertexArrayObject* vaoMesh    = nullptr;
    QOpenGLBuffer* vboMesh = nullptr;
    QOpenGLBuffer* iboMesh = nullptr;
    unsigned int numFacesSphereS = 0, numFacesSphereL = 0;
    unsigned int numMeshIndices = 0;
    bool showParticles = true;

    // physics
    Integrator* integrator = nullptr; // TODO: pick a better one
    ParticleSystem system;
    ForceConstAcceleration* fGravity = nullptr;
    std::vector<ForceSpring*> springsStretch;
    std::vector<ForceSpring*> springsShear;
    std::vector<ForceSpring*> springsBend;

    // cloth properties
    std::vector<bool> fixedParticle;
    double clothWidth, clothHeight;
    int numParticles, numParticlesX, numParticlesY;
    int selectedParticle = -1;

    ColliderSphere colliderSphere;

    // collision properties
    bool checkCollisions = true;
    double colBounce = 0.01;
    double colFriction = 0.05;
    double particleRadius = 1;

    // mouse interaction
    int grabX, grabY;
    Vec3 cursorWorldPos;
};

#endif // SCENECLOTH_H
