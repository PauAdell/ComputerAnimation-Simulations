#include "scenenbody.h"
#include "glutils.h"
#include "model.h"
#include <QMatrix4x4>
#include <iostream>


SceneNBody::SceneNBody() {
    widget = new WidgetNBody();

}

SceneNBody::~SceneNBody() {
    if (widget)        delete widget;
    if (shaderPhong)   delete shaderPhong;
    if (shaderLines)   delete shaderLines;
    if (vaoSphere)     delete vaoSphere;
    if (vaoTrajectory) delete vaoTrajectory;
    if (vaoFloor)      delete vaoFloor;
    if (vboTrajectoryPoints) delete vboTrajectoryPoints;
}

void SceneNBody::initialize() {

    /*
     * RENDER INITS
     */

    // load shader programs
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");
    shaderLines = glutils::loadShaderProgram(":/shaders/lines.vert", ":/shaders/lines.geom", ":/shaders/lines.frag");

    // create sphere VAO
    Model sphere = Model::createIcosphere(3);
    vaoSphere = glutils::createVAO(shaderPhong, &sphere, buffers);
    numSphereFaces = sphere.numFaces();

    // create floor VAO
    Model quad = Model::createQuad();
    vaoFloor = glutils::createVAO(shaderPhong, &quad, buffers);

    // initialize the buffer we'll use to render trajectories
    vaoTrajectory = new QOpenGLVertexArrayObject();
    vaoTrajectory->create();
    vaoTrajectory->bind();
    vboTrajectoryPoints = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    vboTrajectoryPoints->create();
    vboTrajectoryPoints->bind();
    vboTrajectoryPoints->setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);
    vboTrajectoryPoints->allocate(MAX_TRAJ_POINTS*3*sizeof(float));
    shaderLines->setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 0);
    shaderLines->enableAttributeArray("vertex");
    vaoTrajectory->release();
}

Vec3 getParticleColor(double t) {
    int hue = int(t*360);
    int saturation = 160;  // mid saturation
    int value = 160;       // high brightness

    QColor color = QColor::fromHsv(hue, saturation, value);
    color = color.convertTo(QColor::Rgb);
    return Vec3(color.redF(), color.greenF(), color.blueF());
}

void SceneNBody::reset() {

    if (integrator) delete integrator;
    switch (widget->getIntegratorType()) {
        case 0: integrator = new IntegratorEuler(); break;
        case 1: integrator = new IntegratorSymplecticEuler(); break;
        case 2: integrator = new IntegratorMidpoint(); break;
        case 3: integrator = new IntegratorRK2(); break;
        case 4: integrator = new IntegratorRK4(); break;
        case 5: integrator = new IntegratorVerlet(); break;
        default: integrator = nullptr; break;
    }

    // create the N particles corresponding to the bodies
    system.deleteParticles();
    system.deleteForces();

    int numBodies = widget->getNumBodies();
    double massRatio = widget->getMassRange();
    const double sceneR = 50;
    double massScale = 1e15; // for G=1, use 5e4 to get similar results

    for (int i = 0; i < numBodies; i++) {
        double a = i*2*M_PI/numBodies;
        double b = widget->getBodiesLayout() > 0 ? 0.5*M_PI*Random::get(-0.9, 0.9) : 0;

        // note that we are creating initial velocities tangent to the XZ plane
        // for the 2-body case on sphere, this does not necessarily lead to periodic ellipse orbits
        // because these tangents might point outside the orbital plane that contains both particles
        Vec3 radial, tangent;
        switch (widget->getBodiesLayout()) {
        case 0:
        case 1:
            radial  = Vec3(std::cos(a)*std::cos(b), std::sin(b), std::sin(a)*std::cos(b));
            tangent = Vec3(std::sin(a)*std::cos(b), 0, -std::cos(a)*std::cos(b));
            break;
        case 2:
            radial  = Vec3(std::cos(a), std::sin(b), std::sin(a));
            tangent = Vec3(std::sin(a), 0, -std::cos(a));
            break;
        }

        Particle* p = new Particle();
        p->pos = sceneR * radial;
        p->vel = 0.2*sceneR * tangent;
        p->mass = massScale * (1 + (massRatio - 1)*(double(i)/(numBodies-1)));
        p->radius = 5*std::sqrt(p->mass/massScale);
        p->color = getParticleColor(double(i)/numBodies);
        system.addParticle(p);
    }

    // create a gravitational field force for each particle
    const double G = 6.6743e-11;
    for (int i = 0; i < numBodies; i++) {
        ForceGravitation* force = new ForceGravitation(system.getParticle(i), G);
        force->setSmoothingFactors(widget->getSmoothingA(), widget->getSmoothingB());
        for (int j = 0; j < numBodies; j++) {
            if (i != j) {
                force->addInfluencedParticle(system.getParticle(j));
            }
        }
        system.addForce(force);
    }

    // update system forces
    system.resetFirstStep();
    system.updateForces();

    // trajectories
    trajectories = std::vector<std::list<Vec3>>(numBodies);

    // reset timer
    firstIteration = true;
}


void SceneNBody::update(double dt) {

    // update system
    Vecd pos = system.getPositions();
    if (firstIteration) {
        system.setPreviousPositions(pos - dt*system.getVelocities());
        firstIteration = false;
    }
    integrator->step(system, dt);
    system.setPreviousPositions(pos);

    // record trajectories
    for (unsigned int i = 0; i < system.getNumParticles(); i++) {
        trajectories[i].push_back(system.getParticle(i)->pos);
        if (trajectories[i].size() > MAX_TRAJ_POINTS) {
            trajectories[i].pop_front();
        }
    }
}


void SceneNBody::paint(const Camera& camera) {

    // pointer to current context OpenGL functions
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();

    // start using phong shader
    shaderPhong->bind();

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shaderPhong->setUniformValue("ProjMatrix", camProj);
    shaderPhong->setUniformValue("ViewMatrix", camView);

    // lighting
    const int numLights = 2;
    const QVector3D lightPosWorld[numLights] = {QVector3D(1000,1000,1000), QVector3D(-1000,1000,-1000)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1), QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // draw the different spheres
    vaoSphere->bind();
    for (const Particle* particle : system.getParticles()) {
        Vec3   p = particle->pos;
        Vec3   c = particle->color;

        QMatrix4x4 modelMat;
        modelMat.translate(p[0], p[1], p[2]);
        modelMat.scale(particle->radius);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        shaderPhong->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);

        glFuncs->glDrawElements(GL_TRIANGLES, 3*numSphereFaces, GL_UNSIGNED_INT, 0);
    }
    vaoSphere->release();

    // do we need to draw trajectories?
    if (widget->drawTrajectories()) {
        shaderLines->bind();
        shaderLines->setUniformValue("ProjMatrix", camProj);
        shaderLines->setUniformValue("ViewMatrix", camView);
        shaderLines->setUniformValue("radius", float(0.25));
        shaderLines->setUniformValue("shading", false);

        for (unsigned int i = 0; i < system.getNumParticles(); i++) {
            Vec3 c = system.getParticle(i)->color;
            shaderLines->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));

            updateTrajectoryCoordsBuffer(trajectories[i]);
            vaoTrajectory->bind();
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectories[i].size()),
                                                             MAX_TRAJ_POINTS));
            vaoTrajectory->release();
        }

        shaderLines->release();
    }

    // draw floor last (because of transparency)
    shaderPhong->bind();
    vaoFloor->bind();
    QMatrix4x4 modelMat;
    modelMat.scale(200, 1, 200);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.7f, 0.7f, 0.7f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    shaderPhong->setUniformValue("alpha", 0.3f);
    glFuncs->glDepthMask(GL_FALSE);
    glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glFuncs->glDepthMask(GL_TRUE);
    shaderPhong->setUniformValue("alpha", 1.0f);
    vaoFloor->release();
    shaderPhong->release();
}


void SceneNBody::updateTrajectoryCoordsBuffer(const std::list<Vec3>& trajectory) {
    vboTrajectoryPoints->bind();
    float* pos = new float[3*trajectory.size()];
    unsigned int i = 0;
    for (auto it = trajectory.begin(); it != trajectory.end(); it++) {
        pos[3*i  ] = it->x();
        pos[3*i+1] = it->y();
        pos[3*i+2] = it->z();
        i++;
    }
    void* bufptr = vboTrajectoryPoints->mapRange(0, 3*trajectory.size()*sizeof(float),
                                                 QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(pos), 3*trajectory.size()*sizeof(float));
    vboTrajectoryPoints->unmap();
    vboTrajectoryPoints->release();
    delete[] pos;
}
