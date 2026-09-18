#include "scenetestcolliders.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>


SceneTestColliders::SceneTestColliders() {
    widget = new WidgetTestColliders();
    connect(widget, SIGNAL(updatedParameters()), this, SLOT(updateSimParams()));
}

SceneTestColliders::~SceneTestColliders() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (vaoPlane)    delete vaoPlane;
    if (vaoSphere)   delete vaoSphere;
    if (vaoCube)     delete vaoCube;
    if (vaoCylinder) delete vaoCylinder;
}

void SceneTestColliders::initialize() {

    // load shader
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");

    // create VAOs
    Model quad = Model::createQuad();
    vaoPlane = glutils::createVAO(shaderPhong, &quad, buffers);

    Model sphere = Model::createIcosphere(3);
    vaoSphere = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphere = sphere.numFaces();

    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shaderPhong, &cube, buffers);

    Model cylinder = Model::createCylinder(32, true);
    vaoCylinder = glutils::createVAO(shaderPhong, &cylinder, buffers);
    numFacesCylinder = cylinder.numFaces();

    glutils::checkGLError();
}

void SceneTestColliders::reset()
{
    updateSimParams();
}


void SceneTestColliders::updateSimParams()
{
    // for this particular test scene, all the work is done here

    // get the two points for the particle's trajectory segment
    pPrevPos = widget->getPrevPos();
    pPredPos = widget->getCurrPos();
    particleRadius = widget->getParticleRadius();

    // get the collider
    Collider* collider;
    switch (widget->getColliderType()) {
        case 0:
            collider = new ColliderPlane(widget->getPlaneNormal().normalized(), widget->getPlaneD());
            break;
        case 1:
            collider = new ColliderSphere(widget->getSphereCenter(), widget->getSphereRadius());
            break;
        case 2:
            collider = new ColliderAABB(widget->getBoxMin(), widget->getBoxMax());
            break;
        default:
            collider = nullptr;
            break;
    }

    // collision?
    if (collider) {

        Particle particle;
        particle.prevPos = pPrevPos;
        particle.pos = pPredPos;
        particle.radius = particleRadius;

        if (widget->getContinuousCollision()) {
            collision = collider->testCollision(&particle, colInfo);
        }
        else {
            collision = collider->isInside(&particle);
        }

        if (collision) {
            collider->resolveCollision(&particle, colInfo, widget->getRestitution(), widget->getFriction());
            pCorrPos = particle.pos;

            QString colText = "Collision\n";
            colText += "Col.Pos: " + QString::number(colInfo.position[0], 'f', 2) + "," +
                                     QString::number(colInfo.position[1], 'f', 2) + "," +
                                     QString::number(colInfo.position[2], 'f', 2) + "\n";
            colText += "Col.Nor: " + QString::number(colInfo.normal[0], 'f', 2) + "," +
                                     QString::number(colInfo.normal[1], 'f', 2) + "," +
                                     QString::number(colInfo.normal[2], 'f', 2) + "\n";
            colText += "Resolve: " + QString::number(pCorrPos[0], 'f', 2) + "," +
                                     QString::number(pCorrPos[1], 'f', 2) + "," +
                                     QString::number(pCorrPos[2], 'f', 2) + "\n";
            widget->setCollisionInfo(colText);
        }
        else {
            widget->setCollisionInfo("No collision");
        }

        delete collider;
    }


}

void SceneTestColliders::paint(const Camera& camera)
{
    // pointer to current context OpenGL functions
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();

    shaderPhong->bind();
    shaderPhong->setUniformValue("normalSign", 1.0f);

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shaderPhong->setUniformValue("ProjMatrix", camProj);
    shaderPhong->setUniformValue("ViewMatrix", camView);

    // lighting
    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(80,60,80)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);


    // draw the different spheres representing the particle
    QMatrix4x4 modelMat;
    vaoSphere->bind();
    shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
    shaderPhong->setUniformValue("matshin", 100.f);

    // previous (light blue)
    modelMat = QMatrix4x4();
    modelMat.translate(pPrevPos[0], pPrevPos[1], pPrevPos[2]);
    modelMat.scale(std::max(particleRadius, 0.05));
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 172.0f/255.0f, 229.0f/255.0f, 234.0f/255.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);

    // predicted (dark blue)
    modelMat = QMatrix4x4();
    modelMat.translate(pPredPos[0], pPredPos[1], pPredPos[2]);
    modelMat.scale(std::max(particleRadius, 0.05));
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.0f, 12.0f/255.0f, 123.0f/255.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);

    if (collision) {
        // corrected (green)
        modelMat = QMatrix4x4();
        modelMat.translate(pCorrPos[0], pCorrPos[1], pCorrPos[2]);
        modelMat.scale(std::max(particleRadius, 0.05));
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        shaderPhong->setUniformValue("matdiff", 36.0f/255.0f, 229.0f/255.0f, 51.0f/255.0f);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);

        // collision point
        modelMat = QMatrix4x4();
        modelMat.translate(colInfo.position[0], colInfo.position[1], colInfo.position[2]);
        modelMat.scale(std::max(particleRadius, 0.05));
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        shaderPhong->setUniformValue("matdiff", 235.0f/255.0f, 51.0f/255.0f, 36.0f/255.0f);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
    }


    {
        vaoCylinder->bind();

        Vec3 dir = pPredPos - pPrevPos;
        Vec3 ctr = 0.5*(pPredPos + pPrevPos);
        Vec3 n = dir.normalized();
        modelMat = QMatrix4x4();
        modelMat.translate(ctr[0], ctr[1], ctr[2]);
        modelMat.rotate(Math::toDeg(std::acos(n[1])), QVector3D(n[2], 0, -n[0]));
        modelMat.scale(0.025, 0.5*dir.norm(), 0.025);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        shaderPhong->setUniformValue("matdiff", 0.7f, 0.7f, 0.9f);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);

        if (collision) {
            dir = pCorrPos - colInfo.position;
            ctr = 0.5*(pCorrPos + colInfo.position);
            n = dir.normalized();
            modelMat = QMatrix4x4();
            modelMat.translate(ctr[0], ctr[1], ctr[2]);
            modelMat.rotate(Math::toDeg(std::acos(n[1])), QVector3D(n[2], 0, -n[0]));
            modelMat.scale(0.025, 0.5*dir.norm(), 0.025);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            shaderPhong->setUniformValue("matdiff", 0.7f, 0.9f, 0.7f);
            glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);
        }
    }


    shaderPhong->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    modelMat = QMatrix4x4();
    switch (widget->getColliderType()) {
        case 0: {
            Vec3 n = widget->getPlaneNormal().normalized();
            double a = Math::toDeg(std::acos(n[1]));
            modelMat.rotate(a, QVector3D(n[2], 0, -n[0]));
            modelMat.translate(0, -widget->getPlaneD(), 0);
            modelMat.scale(10);

            vaoPlane->bind();
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            break;
        }
        case 1: {
            Vec3 scenter = widget->getSphereCenter();
            double srad  = widget->getSphereRadius();
            double cdist = (camera.getPos() - scenter).norm();

            GLint cullFaceMode;
            glFuncs->glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);

            vaoSphere->bind();

            modelMat.translate(scenter[0], scenter[1], scenter[2]);
            modelMat.scale(srad);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            shaderPhong->setUniformValue("alpha", 0.5f);

            glFuncs->glEnable(GL_CULL_FACE);
            glFuncs->glCullFace(cdist < srad ? GL_FRONT : GL_BACK);
            glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
            glFuncs->glCullFace(cullFaceMode);
            glFuncs->glDisable(GL_CULL_FACE);
            break;
        }
        case 2: {
            Vec3 bmin = widget->getBoxMin();
            Vec3 bmax = widget->getBoxMax();
            Vec3 bcenter = 0.5*(bmin + bmax);
            Vec3 bscale = 0.5*(bmax - bmin);
            modelMat.translate(bcenter[0], bcenter[1], bcenter[2]);
            modelMat.scale(bscale[0], bscale[1], bscale[2]);

            vaoCube->bind();
            shaderPhong->setUniformValue("alpha", 0.2f);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            glFuncs->glDepthMask(GL_FALSE);
            glFuncs->glDrawElements(GL_TRIANGLES, 3*2*6, GL_UNSIGNED_INT, 0);
            glFuncs->glDepthMask(GL_TRUE);
            break;
        }
        default:
            break;
    }
    shaderPhong->setUniformValue("alpha", 1.0f);

    shaderPhong->release();

    glutils::checkGLError();
}


void SceneTestColliders::update(double)
{
    // nothing to do here, it's a special scene for testing purposes.
    // see updatedSimParams()
}

void SceneTestColliders::mousePressed(const QMouseEvent*, const Camera&)
{
}

void SceneTestColliders::mouseMoved(const QMouseEvent*, const Camera&)
{
}

void SceneTestColliders::mouseReleased(const QMouseEvent*, const Camera&)
{

}

