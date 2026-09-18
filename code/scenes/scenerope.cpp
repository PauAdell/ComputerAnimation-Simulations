#include "scenerope.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>


SceneRope::SceneRope() {
    widget = new WidgetRope();
    widget->setIntegrator(0);
    connect(widget, SIGNAL(updatedParameters()),
            this, SLOT(updateSimParams()));
}

SceneRope::~SceneRope() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (vaoSphereS)  delete vaoSphereS;
    if (vaoSphereL)  delete vaoSphereL;
    if (vaoCube)     delete vaoCube;
    if (vaoRope)     delete vaoRope;
    if (vboRope)     delete vboRope;

    system.deleteParticles();
    if (fGravity)  delete fGravity;
    for (Force* f : springs) delete f;
}

void SceneRope::initialize() {
    // load shader
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");
    shaderLines = glutils::loadShaderProgram(":/shaders/lines.vert",
                                             ":/shaders/lines.geom",
                                             ":/shaders/lines.frag");

    // create sphere VAOs
    Model sphere = Model::createIcosphere(3);
    vaoSphereL = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereL = sphere.numFaces();

    sphere = Model::createIcosphere(1);
    vaoSphereS = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereS = sphere.numFaces();

    // create rope VAO
    vaoRope = new QOpenGLVertexArrayObject();
    vaoRope->create();
    vaoRope->bind();
    vboRope = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    vboRope->create();
    vboRope->bind();
    vboRope->setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);
    vboRope->allocate(1000*3*sizeof(float)); // sync with widget max particles
    shaderLines->setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 0);
    shaderLines->enableAttributeArray("vertex");
    vaoRope->release();


    // create forces
    fGravity = new ForceConstAcceleration();
    system.addForce(fGravity);

    // colliders
    colliderBall.setCenter(Vec3(0,-50,0));
    colliderBall.setRadius(30);
}

void SceneRope::reset()
{
    // we only update numParticles on resets
    updateSimParams();

    // reset particles
    system.clearParticles();
    for (Particle* p : particles) delete p;
    particles.clear();

    // reset forces
    fGravity->clearInfluencedParticles();
    for (ForceSpring* f : springs) delete f;
    springs.clear();
    system.clearForces();
    system.addForce(fGravity);

    // rope props
    ropeLength = widget->getRopeLength();
    numParticles = widget->getNumParticles();
    edgeLength = ropeLength/(numParticles-1);
    anchoredEnd = widget->anchorRopeEnd();

    if (integrator) delete integrator;
    switch(widget->getIntegrator()) {
        case 0: integrator = new IntegratorEuler(); break;
        case 1: integrator = new IntegratorSymplecticEuler(); break;
        case 2: integrator = new IntegratorMidpoint(); break;
        case 3: integrator = new IntegratorRK2(); break;
        case 4: integrator = new IntegratorRK4(); break;
        case 5: integrator = new IntegratorVerlet(); break;
        default: integrator = nullptr; break;
    }

    // create particles
    int startConfig = widget->getStartConfig();
    const Vec3 color1 = Vec3(235/255.0, 51/255.0, 36/255.0);
    const Vec3 color2 = Vec3(163/255.0, 73/255.0, 164/255.0);

    for (int i = 0; i < numParticles; i++) {

        Vec3 pos(0,0,0);
        double t = i*edgeLength;
        switch (startConfig) {
            case 0: // vertical
                pos = Vec3(0, 90, 0) + t*Vec3(0, -1, 0);
                break;
            case 1: // horizontal
                pos = Vec3(-90, 90, 0) + t*Vec3(1, 0, 0);
                break;
            case 2: // helix
                pos = Vec3(0, 90, 0) - t*Vec3(std::cos(t/3.0), t/8.0,
                                              std::sin(t/3.0)).normalized();
                break;
            default:
                break;
        }

        Particle* p = new Particle();
        p->id = i;
        p->pos = pos;
        p->prevPos = pos;
        p->vel = Vec3(0,0,0);
        p->mass = 1;
        p->radius = particleRadius;
        p->color = (1.0 - double(i)/numParticles)*color1
                        + double(i)/numParticles *color2;
        particles.push_back(p);

        // if anchor, keep this particle out of physical sim
        if (anchoredEnd && i == 0) {
            anchor = p;
            anchor->color = Vec3(50/255.0, 130/255.0, 246/255.0);
        }
        else {
            system.addParticle(p);
            fGravity->addInfluencedParticle(p);
        }
    }

    // create springs
    double ks = widget->getStiffness();
    double kd = widget->getDamping();
    for (int i = 1; i < numParticles; i++) {
        ForceSpring* f = new ForceSpring();
        f->setParticlePair(particles[i-1], particles[i]);
        f->setRestLength(edgeLength);
        f->setSpringConstant(ks);
        f->setDampingCoeff(kd);
        system.addForce(f);
        springs.push_back(f);
    }
}

void SceneRope::updateSimParams()
{
    double g = widget->getGravity();
    fGravity->setAcceleration(Vec3(0, -g, 0));

    double ks = widget->getStiffness();
    double kd = widget->getDamping();
    for (ForceSpring* f : springs) {
        f->setSpringConstant(ks);
        f->setDampingCoeff(kd);
    }

    checkCollisions = widget->checkCollisions();
    particleRadius = widget->getParticleScale();
    for (Particle* p : particles) {
        p->radius = particleRadius;
    }

    showParticles = widget->showParticles();
}

void SceneRope::paint(const Camera& camera) {

    // pointer to current context OpenGL functions
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();

    shaderPhong->bind();

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shaderPhong->setUniformValue("ProjMatrix", camProj);
    shaderPhong->setUniformValue("ViewMatrix", camView);

    // lighting
    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(60,60,100)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // draw the different spheres
    QMatrix4x4 modelMat;
    if (showParticles) {
        vaoSphereS->bind();
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        for (int i = 0; i < numParticles; i++) {
            const Particle* particle = particles[i];
            Vec3   p = particle->pos;
            Vec3   c = particle->color;
            double r = particle->radius;
            if (i == selectedParticle) c = Vec3(1.0,0.9,0);

            modelMat = QMatrix4x4();
            modelMat.translate(p[0], p[1], p[2]);
            modelMat.scale(r * particleRadius);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            shaderPhong->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
            glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereS, GL_UNSIGNED_INT, 0);
        }
    }

    // draw collision sphere
    vaoSphereL->bind();
    Vec3 cc = colliderBall.getCenter();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(colliderBall.getRadius());
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereL, GL_UNSIGNED_INT, 0);

    shaderPhong->release();


    // update rope VBO coords
    vboRope->bind();
    float* pos = new float[3*numParticles];
    for (int i = 0; i < numParticles; i++) {
        pos[3*i  ] = particles[i]->pos.x();
        pos[3*i+1] = particles[i]->pos.y();
        pos[3*i+2] = particles[i]->pos.z();
    }
    void* bufptr = vboRope->mapRange(0, 3*numParticles*sizeof(float), QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(pos), 3*numParticles*sizeof(float));
    vboRope->unmap();
    vboRope->release();
    delete[] pos;


    // draw rope
    shaderLines->bind();
    shaderLines->setUniformValue("ProjMatrix", camProj);
    shaderLines->setUniformValue("ViewMatrix", camView);
    shaderLines->setUniformValue("radius", float(0.5*particleRadius));
    shaderLines->setUniformValue("matdiff", 0.7f, 0.0f, 0.0f);
    shaderLines->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderLines->setUniformValue("matshin", 0.0f);
    shaderLines->setUniformValue("numLights", numLights);
    shaderLines->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderLines->setUniformValueArray("lightColor", lightColor, numLights);
    vaoRope->bind();
    glFuncs->glDrawArrays(GL_LINE_STRIP, 0, numParticles);
    vaoRope->release();
    shaderLines->release();

    glutils::checkGLError();
}


void SceneRope::update(double dt)
{
    // integration step
    Vecd ppos = system.getPositions();
    integrator->step(system, dt);
    system.setPreviousPositions(ppos);
    deltaTime = dt;

    // ball & wall collisions
    Collision colInfo;
    if (checkCollisions) {
        for (Particle* p : particles) {
            if (colliderBall.testCollision(p, colInfo)) {
                colliderBall.resolveCollision(p, colInfo, colBounce, colFriction);
            }
        }
    }

    // user interaction
    if (selectedParticle >= 0) {
        Particle* p = particles[selectedParticle];
        p->pos     = cursorWorldPos;
        p->vel     = Vec3(0,0,0);
        if (checkCollisions) {
            if (colliderBall.testCollision(p, colInfo)) {
                colliderBall.resolveCollision(p, colInfo, colBounce, colFriction);
            }
        }
        p->prevPos = p->pos;
        p->vel     = Vec3(0,0,0);
    }
}


void SceneRope::mousePressed(const QMouseEvent* e, const Camera& cam)
{
    grabX = e->pos().x();
    grabY = e->pos().y();
}


void SceneRope::mouseMoved(const QMouseEvent* e, const Camera& cam)
{
    int dx = e->pos().x() - grabX;
    int dy = e->pos().y() - grabY;
    grabX = e->pos().x();
    grabY = e->pos().y();

    if (e->modifiers() & Qt::ControlModifier) {
        double d = -(colliderBall.getCenter() - cam.getPos()).dot(cam.zAxis());
        Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, d);
        colliderBall.setCenter(colliderBall.getCenter() + disp);
    }
}


void SceneRope::mouseReleased(const QMouseEvent*, const Camera&)
{
}
