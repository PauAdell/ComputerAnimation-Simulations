#ifndef SCENE_H
#define SCENE_H

#include <QWidget>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include "camera.h"

class Scene : public QObject
{
    Q_OBJECT
public:
    Scene() {}
    virtual ~Scene() {};

    virtual void initialize() = 0;
    virtual void reset() = 0;
    virtual void update(double dt) = 0;
    virtual void paint(const Camera& cam) = 0;

    virtual void mousePressed (const QMouseEvent*, const Camera&) {};
    virtual void mouseMoved   (const QMouseEvent*, const Camera&) {};
    virtual void mouseReleased(const QMouseEvent*, const Camera&) {};
    virtual void keyPressed   (const QKeyEvent*,   const Camera&) {};

    virtual void getSceneBounds(Vec3& bmin, Vec3& bmax) = 0;
    virtual unsigned int getNumParticles() { return 0; }

    virtual QWidget* sceneUI() = 0;

    void clearBuffers() {
        for (QOpenGLBuffer* buf : buffers) {
            buf->destroy();
            delete buf;
        }
        buffers.clear();
    }

protected:
    std::vector<QOpenGLBuffer*> buffers;
};

#endif // SCENE_H
