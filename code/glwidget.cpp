#include "glwidget.h"
#include <QtCore/qtimer.h>
#include <QPainter>
#include <QPen>
#include <iostream>


GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    scene = nullptr;
}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
    camera.setViewport(w, h);
    updateFOV();
}

void GLWidget::paintGL()
{
    // Clear
    glClearColor(0.62f, 0.74f, 0.85f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw
    if (scene) {
        scene->paint(camera);
        paintTextOverlay();
    }

    // Simulate and call next frame draw
    if (runningSim && scene) {
        simStep();
        update();
    }
}

void GLWidget::paintTextOverlay()
{
    // We need to unbind VAO and program for now because it causes problem with commands below
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    QPainter painter;
    painter.begin(this);

    painter.setRenderHint(QPainter::Antialiasing);
    QPen penLineGrey(QColor(50, 50, 50));
    QPen penLineWhite(QColor(250, 250, 250));

    const int bX = 10;
    const int bY = 10;
    const int sizeX = 170;
    const int sizeY = 110;

    // Background
    painter.setPen(penLineGrey);
    painter.fillRect(QRect(bX, bY, sizeX, sizeY), QColor(0, 0, 255, 25));
    painter.drawRect(bX, bY, sizeX, sizeY);

    // Text
    QFont f1, f2;
    f1.setBold(true);
    f1.setPointSize(18);
    f2.setPointSize(10);
    painter.setFont(f1);
    painter.setPen(penLineWhite);
    painter.setFont(f2);
    painter.drawText(10 + 5, bY + 10 +  10, "Particles: " + QString::number(scene->getNumParticles()));
    painter.drawText(10 + 5, bY + 10 +  30, "Sim step:  " + QString::number(simSteps));
    painter.drawText(10 + 5, bY + 10 +  50, "Sim time:  " + QString::number(simTime, 'f', 3) + " s");
    painter.drawText(10 + 5, bY + 10 +  70, "Curr perf: " + QString::number(simPerf, 'f', 1) + " ms/step");
    painter.drawText(10 + 5, bY + 10 +  90, "Avg perf:  " + QString::number(simMs/double(simSteps), 'f', 1) + " ms/step");
    painter.end();

    // Reset GL depth test and alpha
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLWidget::setScene(Scene* sc)
{
    if (scene) delete scene;
    scene = sc;

    this->makeCurrent();
    scene->initialize();

    Vec3 bmin, bmax;
    scene->getSceneBounds(bmin, bmax);
    sceneRad = 0.5*((bmax - bmin).norm());

    resetCamera();
    this->resetSim();

    update();
}


void GLWidget::simStep() {
    timer.start();

    scene->update(timeStep);

    double tsim = 1e-6 * double(timer.nsecsElapsed());
    simMs += tsim;
    simSteps++;
    simTime += timeStep;
    simPerf = tsim;
}


void GLWidget::mousePressEvent(QMouseEvent * e)
{
    x0 = e->pos().x();
    y0 = e->pos().y();
    if (e->buttons() & Qt::RightButton) {
        if (scene) scene->mousePressed(e, camera);
    }

    update();
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
    int x = e->pos().x();
    int y = e->pos().y();

    if (e->buttons() & Qt::LeftButton) {
        camera.rotateLeftRight((x0 - x) * M_PI/width());
        camera.rotateUpDown((y - y0) * 0.5*M_PI/height());
    }
    else if (e->buttons() & Qt:: MiddleButton) {
        Vec3 d = camera.worldSpaceDisplacement(x0 - x, y - y0, camera.getEyeDistance());
        camera.move(d);
    }
    else if (e->buttons() & Qt::RightButton) {
        if (scene) scene->mouseMoved(e, camera);
    }

    x0 = x;
    y0 = y;

    update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (scene) scene->mouseReleased(e, camera);

    update();
}

void GLWidget::wheelEvent(QWheelEvent* e)
{
    int numDegrees = e->angleDelta().y() / 8;
    int numSteps = numDegrees / 15;

    if (!(e->modifiers()&Qt::ShiftModifier)) {
        double dist = camera.getEyeDistance();
        double speed = 0.1*dist;
        camera.moveBackForth(speed*numSteps, false);
        if (camera.getEyeDistance() < sceneRad) {
            camera.setPlanes(0.001*sceneRad, 2*sceneRad);
        }
        else {
            double n = camera.getEyeDistance() - sceneRad;
            camera.setPlanes(n, n + 2*sceneRad);
        }
    }

    update();
}

void GLWidget::keyPressEvent(QKeyEvent* e)
{
    scene->keyPressed(e, camera);
}

void GLWidget::doSimStep()
{
    this->makeCurrent();
    if (scene) simStep();
    update();
}

void GLWidget::doSimLoop()
{
    this->makeCurrent();
    runningSim = true;
    update();
}

void GLWidget::pauseSim()
{
    this->makeCurrent();
    runningSim = false;
    update();
}

void GLWidget::resetSim()
{
    this->makeCurrent();
    if (scene) scene->reset();
    simSteps = 0;
    simTime = 0;
    simPerf = 0;
    simMs = 0;
    update();
}

void GLWidget::resetCamera()
{
    if (scene) {
        Vec3 bmin, bmax;
        scene->getSceneBounds(bmin, bmax);
        camera  = Camera::CreateFromSceneBounds(bmin, bmax);
        camera.setViewport(width(), height());
        halfFOV = 0.5*camera.getFOV();
        updateFOV();

        update();
    }
}

void GLWidget::cameraViewX()
{
    camera.setEulerAngles(0, M_PI/2);
    update();
}

void GLWidget::cameraViewY()
{
    camera.setEulerAngles(M_PI/2, 0);
    update();
}

void GLWidget::cameraViewZ()
{
    camera.setEulerAngles(0, 0);
    update();
}

void GLWidget::updateFOV()
{
    double ar = double(width())/double(height());
    double f;
    if (ar < 1)
        f = 2*Math::toDeg(atan(tan(Math::toRad(halfFOV))/ar));
    else
        f = 2*halfFOV;
    camera.setFOV(f);
}
