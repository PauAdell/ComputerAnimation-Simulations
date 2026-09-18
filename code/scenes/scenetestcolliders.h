#ifndef SCENETESTCOLLIDERS_H
#define SCENETESTCOLLIDERS_H

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include "scene.h"
#include "widgettestcolliders.h"
#include "colliders.h"


class SceneTestColliders : public Scene
{
    Q_OBJECT

public:
    SceneTestColliders();
    virtual ~SceneTestColliders();

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

public slots:
    void updateSimParams();

protected:
    WidgetTestColliders* widget = nullptr;

    QOpenGLShaderProgram* shaderPhong = nullptr;
    QOpenGLVertexArrayObject* vaoPlane = nullptr;
    QOpenGLVertexArrayObject* vaoSphere = nullptr;
    QOpenGLVertexArrayObject* vaoCube   = nullptr;
    QOpenGLVertexArrayObject* vaoCylinder = nullptr;
    unsigned int numFacesSphere = 0;
    unsigned int numFacesCylinder = 0;

    Vec3 pPrevPos, pPredPos, pCorrPos;
    double particleRadius;
    bool collision = false;
    Collision colInfo;
};

#endif // SCENETESTCOLLIDERS_H
