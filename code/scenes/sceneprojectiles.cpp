#include "sceneprojectiles.h"
#include "glutils.h"
#include "model.h"
#include <QMatrix4x4>
#include <iostream>


SceneProjectiles::SceneProjectiles() {
    widget = new WidgetProjectiles();

    const std::vector<std::string> solvers = {
        "Euler", "Symplectic Euler", "Midpoint", "RK2", "RK4", "Verlet"
    };
    widget->setSolverTypes(solvers);
    widget->setSolver1(0); // Euler
    widget->setSolver2(1); // Symplectic Euler
}

SceneProjectiles::~SceneProjectiles() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (shaderLines) delete shaderLines;
    if (vaoSphere)   delete vaoSphere;
    if (vaoFloor)    delete vaoFloor;
    if (vaoCube)     delete vaoCube;
    if (vaoTrajectory) delete vaoTrajectory;
    if (vboTrajectoryPoints) delete vboTrajectoryPoints;
    if (integrator1) delete integrator1;
    if (integrator2) delete integrator2;
    systemAnalytic.deleteParticles();
    systemAnalytic.deleteForces();
    systemNumerical1.deleteParticles();
    systemNumerical1.deleteForces();
    systemNumerical2.deleteParticles();
    systemNumerical2.deleteForces();
    clearBuffers();
}

void SceneProjectiles::initialize() {

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
    glutils::checkGLError();

    // create floor VAO
    Model quad = Model::createQuad();
    vaoFloor = glutils::createVAO(shaderPhong, &quad, buffers);
    glutils::checkGLError();

    // create cube VAO
    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shaderPhong, &cube, buffers);
    glutils::checkGLError();

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


    /*
     * PHYSICS INITS
     */

    // create the different systems, with one particle each
    systemAnalytic.addParticle(new Particle(Vec3( 0, 0, 0), Vec3(0,0,0), 1));
    systemAnalytic.getParticle(0)->color = Vec3(0, 0.5, 0);
    systemAnalytic.getParticle(0)->radius = 2;
    systemNumerical1.addParticle(new Particle(Vec3( 0, 0, 15), Vec3(0,0,0), 1));
    systemNumerical1.getParticle(0)->color = Vec3(0.5, 0, 0);
    systemNumerical1.getParticle(0)->radius = 2;
    systemNumerical2.addParticle(new Particle(Vec3( 0, 0, -15), Vec3(0,0,0), 1));
    systemNumerical2.getParticle(0)->color = Vec3(0, 0, 0.5);
    systemNumerical2.getParticle(0)->radius = 2;

    // only one force: gravity, but we need to create one per system to assign its particle
    fGravity1 = new ForceConstAcceleration(Vec3(0, -gravityAccel, 0));
    fGravity1->addInfluencedParticle(systemNumerical1.getParticle(0));
    systemNumerical1.addForce(fGravity1);

    fGravity2 = new ForceConstAcceleration(Vec3(0, -gravityAccel, 0));
    fGravity2->addInfluencedParticle(systemNumerical2.getParticle(0));
    systemNumerical2.addForce(fGravity2);

}


Integrator* createIntegrator(int type) {
    switch(type) {
        case 0: return new IntegratorEuler();
        case 1: return new IntegratorSymplecticEuler(); break;
        case 2: return new IntegratorMidpoint(); break;
        case 3: return new IntegratorRK2(); break;
        case 4: return new IntegratorRK4(); break;
        case 5: return new IntegratorVerlet(); break;
        default: return nullptr;
    }
}


void SceneProjectiles::reset() {

    // get new interface values
    shotHeight   = widget->getHeight();
    shotAngle    = Math::toRad(widget->getAngle());
    shotSpeed    = widget->getSpeed();
    gravityAccel = widget->getGravity();

    // integrators
    if (integrator1) delete integrator1;
    if (integrator2) delete integrator2;
    integrator1 = createIntegrator(widget->getSolver1());
    integrator2 = createIntegrator(widget->getSolver2());

    // update initial particle positions
    const double zdist = 15;
    systemAnalytic.getParticle(0)->pos = Vec3(0, shotHeight, 0);
    systemAnalytic.getParticle(0)->vel = shotSpeed*Vec3(std::cos(shotAngle), std::sin(shotAngle), 0);
    systemNumerical1.getParticle(0)->pos = Vec3(0, shotHeight, zdist);
    systemNumerical1.getParticle(0)->vel = shotSpeed*Vec3(std::cos(shotAngle), std::sin(shotAngle), 0);
    systemNumerical2.getParticle(0)->pos = Vec3(0, shotHeight, -zdist);
    systemNumerical2.getParticle(0)->vel = shotSpeed*Vec3(std::cos(shotAngle), std::sin(shotAngle), 0);

    // update gravity accelerations
    fGravity1->setAcceleration(Vec3(0, -gravityAccel, 0));
    fGravity2->setAcceleration(Vec3(0, -gravityAccel, 0));

    // update system forces
    systemNumerical1.resetFirstStep();
    systemNumerical1.updateForces();
    systemNumerical2.resetFirstStep();
    systemNumerical2.updateForces();

    // trajectories
    trajectoryAnalytic.clear();
    trajectoryAnalytic.push_back(systemAnalytic.getParticle(0)->pos);
    trajectoryNumerical1.clear();
    trajectoryNumerical1.push_back(systemNumerical1.getParticle(0)->pos);
    trajectoryNumerical2.clear();
    trajectoryNumerical2.push_back(systemNumerical2.getParticle(0)->pos);

    // put particles to run
    system1active = true;
    system2active = true;

    // reset timer
    time = 0;
}


void SceneProjectiles::update(double dt) {

    // total ellapsed time (needed for analytic solution)
    time += dt;

    // ANALYTIC: projectile motion equations until we reach the ground
    Particle* p = systemAnalytic.getParticle(0);
    double vy0 = shotSpeed*std::sin(shotAngle);
    double tGround = (vy0 + std::sqrt(vy0*vy0 + 2*gravityAccel*shotHeight))/gravityAccel;
    if (time - dt <= tGround) {
        double t = std::min(time, tGround);
        p->pos[0] = t * shotSpeed * std::cos(shotAngle);
        p->pos[1] = shotHeight + t*vy0 - 0.5*gravityAccel*t*t;
        p->vel    = Vec3(shotSpeed*std::cos(shotAngle),
                         shotSpeed*std::sin(shotAngle) - gravityAccel*t, 0);

        trajectoryAnalytic.push_back(p->pos);
        if (trajectoryAnalytic.size() > MAX_TRAJ_POINTS) trajectoryAnalytic.pop_front();
    }


    // NUMERICAL INTEGRATORS:

    if (system1active) {
        // integration step
        integrator1->step(systemNumerical1, dt);

        // collision test
        Particle* p = systemNumerical1.getParticle(0);
        if (p->pos.y() < 0) {
            // resolve
            const Eigen::Vector3d prev = p->prevPos;   // must be set BEFORE integration
            const Eigen::Vector3d curr = p->pos;       // predicted (after integration)
            Eigen::Vector3d hit = curr;

            // If gound hit
            if (prev.y() > 0.0 && curr.y() < 0.0) {
                const double denom = prev.y() - curr.y();
                if (std::abs(denom) > 1e-12) {
                    const double s = prev.y() / denom;
                    hit = prev + s * (curr - prev);
                }
            } else {
                // No crossing detected; just project current point to plane
                hit = curr;
            }

            // Snap to plane with a tiny offset to avoid repeated penetration due to FP error
            constexpr double eps = 1e-8;
            hit.y() = eps;
            p->pos = hit;

            // stop sim for this system
            system1active = false;
        }

        // record trajectory
        trajectoryNumerical1.push_back(p->pos);
        if (trajectoryNumerical1.size() > MAX_TRAJ_POINTS) {
            trajectoryNumerical1.pop_front();
        }
    }

    if (system2active) {
        // integration step
        integrator2->step(systemNumerical2, dt);

        // collision test
        Particle* p = systemNumerical2.getParticle(0);
        if (p->pos.y() < 0) {
            // resolve
            const Eigen::Vector3d prev = p->prevPos;   // must be set BEFORE integration
            const Eigen::Vector3d curr = p->pos;       // predicted (after integration)
            Eigen::Vector3d hit = curr;

            if (prev.y() > 0.0 && curr.y() < 0.0) {
                const double denom = prev.y() - curr.y();
                if (std::abs(denom) > 1e-12) {
                    const double s = prev.y() / denom;
                    hit = prev + s * (curr - prev);
                }
            } else {
                // No crossing detected; just project current point to plane
                hit = curr;
            }

            // Snap to plane with a tiny offset to avoid repeated penetration due to FP error
            constexpr double eps = 1e-8;
            hit.y() = eps;
            p->pos = hit;

            // stop sim for this system
            system2active = false;
        }

        // record trajectory
        trajectoryNumerical2.push_back(p->pos);
        if (trajectoryNumerical2.size() > MAX_TRAJ_POINTS) {
            trajectoryNumerical2.pop_front();
        }
    }

    // print particle heights
    std::cout << "time: " << time << std::endl;
    std::cout << "analytic sol: " << systemAnalytic.getParticle(0)->pos[1] << std::endl;
    std::cout << "numerical 1: " << systemNumerical1.getParticle(0)->pos[1] << std::endl;
    std::cout << "numerical 2: " << systemNumerical2.getParticle(0)->pos[1] << std::endl;
    std::cout << std::endl;
}


void SceneProjectiles::paint(const Camera& camera) {

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
        lightPosCam[i] = camView * lightPosWorld[i];
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // draw floor
    vaoFloor->bind();
    QMatrix4x4 modelMat;
    modelMat.translate(150, 0, 0);
    modelMat.scale(200, 1, 50);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.7f, 0.7f, 0.7f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // draw cube
    vaoCube->bind();
    modelMat = QMatrix4x4();
    modelMat.translate(-1, 0.5*shotHeight - 1, 0);
    modelMat.scale(2, 0.5*shotHeight - 1, 25.0f);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    glFuncs->glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // draw the different spheres
    vaoSphere->bind();
    const Particle* particles[3] = { systemAnalytic.getParticle(0),
                                     systemNumerical1.getParticle(0),
                                     systemNumerical2.getParticle(0) };
    for (const Particle* particle : particles) {
        Vec3   p = particle->pos;
        Vec3   c = particle->color;
        double r = particle->radius;

        modelMat = QMatrix4x4();
        modelMat.translate(p[0], p[1], widget->renderSameZ() ? 0 : p[2]);
        modelMat.scale(r);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);

        shaderPhong->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);

        glFuncs->glDrawElements(GL_TRIANGLES, 3*numSphereFaces, GL_UNSIGNED_INT, 0);
    }
    vaoSphere->release();

    // we're done with this shader
    shaderPhong->release();

    // do we need to draw trajectories?
    if (widget->renderTrajectory()) {

        shaderLines->bind();
        shaderLines->setUniformValue("ProjMatrix", camProj);
        shaderLines->setUniformValue("ViewMatrix", camView);
        shaderLines->setUniformValue("radius", float(0.25));
        shaderLines->setUniformValue("shading", false);

        updateTrajectoryCoordsBuffer(trajectoryAnalytic, widget->renderSameZ());
        Vec3 c = systemAnalytic.getParticle(0)->color;
        shaderLines->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        vaoTrajectory->bind();
        glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryAnalytic.size()),
                                                         MAX_TRAJ_POINTS));
        vaoTrajectory->release();

        updateTrajectoryCoordsBuffer(trajectoryNumerical1, widget->renderSameZ());
        c = systemNumerical1.getParticle(0)->color;
        shaderLines->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        vaoTrajectory->bind();
        glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryNumerical1.size()),
                                                         MAX_TRAJ_POINTS));
        vaoTrajectory->release();

        updateTrajectoryCoordsBuffer(trajectoryNumerical2, widget->renderSameZ());
        c = systemNumerical2.getParticle(0)->color;
        shaderLines->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
        vaoTrajectory->bind();
        glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryNumerical2.size()),
                                                         MAX_TRAJ_POINTS));
        vaoTrajectory->release();

        shaderLines->release();
    }
}


void SceneProjectiles::updateTrajectoryCoordsBuffer(const std::list<Vec3>& trajectory, bool sameZ) {
    vboTrajectoryPoints->bind();
    float* pos = new float[3*trajectory.size()];
    unsigned int i = 0;
    for (auto it = trajectory.begin(); it != trajectory.end(); it++) {
        pos[3*i  ] = it->x();
        pos[3*i+1] = it->y();
        pos[3*i+2] = sameZ ? 0 : it->z();
        i++;
    }
    void* bufptr = vboTrajectoryPoints->mapRange(0, 3*trajectory.size()*sizeof(float),
                                                 QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(pos), 3*trajectory.size()*sizeof(float));
    vboTrajectoryPoints->unmap();
    vboTrajectoryPoints->release();
    delete[] pos;
}
