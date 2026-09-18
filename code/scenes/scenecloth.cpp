#include "scenecloth.h"
#include "glutils.h"
#include "model.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>


SceneCloth::SceneCloth() {
    widget = new WidgetCloth();
    widget->setIntegrator(0);
    connect(widget, SIGNAL(updatedParameters()), this, SLOT(updateSimParams()));
    connect(widget, SIGNAL(freeAnchors()), this, SLOT(freeAnchors()));
}

SceneCloth::~SceneCloth() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (vaoSphereS)  delete vaoSphereS;
    if (vaoSphereL)  delete vaoSphereL;
    if (vaoCube)     delete vaoCube;
    if (vaoMesh)     delete vaoMesh;
    if (vboMesh)     delete vboMesh;
    if (iboMesh)     delete iboMesh;

    system.deleteParticles();
    if (fGravity)  delete fGravity;
    for (ForceSpring* f : springsStretch) delete f;
    for (ForceSpring* f : springsShear) delete f;
    for (ForceSpring* f : springsBend) delete f;
}

void SceneCloth::initialize() {

    // load shaders
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");
    shaderCloth = glutils::loadShaderProgram(":/shaders/cloth.vert", ":/shaders/cloth.geom", ":/shaders/cloth.frag");

    // create sphere VAOs
    Model sphere = Model::createIcosphere(3);
    vaoSphereL = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereL = sphere.numFaces();
    glutils::checkGLError();

    sphere = Model::createIcosphere(3);
    vaoSphereS = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphereS = sphere.numFaces();
    glutils::checkGLError();

    // create cube VAO
    Model cube = Model::createCube();
    vaoCube = glutils::createVAO(shaderPhong, &cube, buffers);
    glutils::checkGLError();


    // create cloth mesh VAO
    vaoMesh = new QOpenGLVertexArrayObject();
    vaoMesh->create();
    vaoMesh->bind();
    vboMesh = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    vboMesh->create();
    vboMesh->bind();
    vboMesh->setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);
    vboMesh->allocate(1000*1000*3*3*sizeof(float)); // sync with widget max particles
    shaderCloth->setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 0);
    shaderCloth->enableAttributeArray("vertex");
    iboMesh = new QOpenGLBuffer(QOpenGLBuffer::Type::IndexBuffer);
    iboMesh->create();
    iboMesh->bind();
    iboMesh->setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);
    iboMesh->allocate(1000*1000*2*3*sizeof(unsigned int));
    vaoMesh->release();

    // create gravity force
    fGravity = new ForceConstAcceleration();
    system.addForce(fGravity);

    // TODO: in my solution setup, these were the colliders
    colliderSphere.setCenter(Vec3(-10,-10,0));
    colliderSphere.setRadius(40);
    //colliderCube.setFromCenterSize(Vec3(-60,30,0), Vec3(60, 40, 60));
    //colliderWalls.setFromCenterSize(Vec3(0, 0, 0), Vec3(200, 200, 200));
}

void SceneCloth::reset()
{
    // we only update numParticles on resets
    updateSimParams();

    // reset particles
    system.deleteParticles();

    // reset forces
    system.clearForces();
    fGravity->clearInfluencedParticles();
    for (ForceSpring* f : springsStretch) delete f;
    springsStretch.clear();
    for (ForceSpring* f : springsShear) delete f;
    springsShear.clear();
    for (ForceSpring* f : springsBend) delete f;
    springsBend.clear();

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

    // cloth props
    Vec2 dims = widget->getDimensions();
    Vec2i dimParticles = widget->getNumParticles();
    numParticlesX = dimParticles.x();
    numParticlesY = dimParticles.y();
    clothWidth  = dims[0];
    clothHeight = dims[1];
    double edgeX = dims[0]/numParticlesX;
    double edgeY = dims[1]/numParticlesY;
    particleRadius = widget->getParticleRadius();

    // create particles
    numParticles = numParticlesX * numParticlesY;
    fixedParticle = std::vector<bool>(numParticles, false);

    const double tiltAngleDeg = 60.0;     // how much to tilt
    const double tiltDirDeg   = 0.0;    // direction of the slope (0=x, 90=y)
    const double theta = tiltAngleDeg * M_PI / 180.0;
    const double phi   = tiltDirDeg   * M_PI / 180.0;

    // unit direction along which height changes
    const double nx = std::cos(phi);
    const double ny = std::sin(phi);

    for (int i = 0; i < numParticlesX; i++) {
        for (int j = 0; j < numParticlesY; j++) {

            int idx = i*numParticlesY + j;

            // TODO: you can play here with different start positions and/or fixed particles
            if ((i == 0 && j == 0) || (i == 0 && j == numParticlesY-1)) {
                fixedParticle[idx] = true;
            } else {
                fixedParticle[idx] = false;
            }

            double tx = i*edgeX - 0.5*clothWidth;
            double ty = j*edgeY - 0.5*clothHeight;

            double z = (tx * nx + ty * ny) * std::tan(theta);
            Vec3 pos = Vec3(ty+edgeY, 70 - tx - edgeX, z);

            Particle* p = new Particle();
            p->id = idx;
            p->pos = pos;
            p->prevPos = pos;
            p->vel = Vec3(0,0,0);
            p->mass = 1;
            p->radius = particleRadius;
            p->color = Vec3(235/255.0, 51/255.0, 36/255.0);

            system.addParticle(p);
            fGravity->addInfluencedParticle(p);
        }
    }

    // forces: gravity
    system.addForce(fGravity);

    // TODO: create spring force
    // Code for PROVOT layout

    // helper to add ONE spring if inside the grid
    auto addSpring = [&](int i, int j,
                         int ni, int nj,
                         std::vector<ForceSpring*>& bag)
    {
        // bounds
        if (ni < 0 || nj < 0 || ni >= numParticlesX || nj >= numParticlesY)
            return;

        int idxA = i  * numParticlesY + j;
        int idxB = ni * numParticlesY + nj;

        Particle* a = system.getParticle(idxA);
        Particle* b = system.getParticle(idxB);

        double L0 = (a->pos - b->pos).norm();

        // ks, kd = 0 for now; we set real values with updateSprings()
        ForceSpring* s = new ForceSpring(a, b, L0, 0.0, 0.0);
        bag.push_back(s);
        system.addForce(s);
    };

    // optional, just to avoid reallocs
    springsStretch.reserve(numParticlesX * numParticlesY * 2);
    springsShear.reserve(numParticlesX * numParticlesY * 2);
    springsBend.reserve(numParticlesX * numParticlesY * 2);

    for (int i = 0; i < numParticlesX; ++i) {
        for (int j = 0; j < numParticlesY; ++j) {

            // -------------------------------------------------
            // 1) STRETCH springs (structural)
            //    only to +x (right) and +y (up) to avoid dups
            // -------------------------------------------------
            addSpring(i, j, i + 1, j,     springsStretch);  // right  (i+1, j)
            addSpring(i, j, i,     j + 1, springsStretch);  // up     (i, j+1)

            // -------------------------------------------------
            // 2) SHEAR springs (diagonals)
            //    again, only "forward" diagonals
            // -------------------------------------------------
            addSpring(i, j, i + 1, j + 1, springsShear);    // up-right
            addSpring(i, j, i + 1, j - 1, springsShear);    // down-right (addSpring checks bounds)

            // -------------------------------------------------
            // 3) BEND springs (2 cells away)
            //    only forward to avoid dups
            // -------------------------------------------------
            addSpring(i, j, i + 2, j,     springsBend);     // 2 to the right
            addSpring(i, j, i,     j + 2, springsBend);     // 2 up
        }
    }

    updateSprings();

    // update index buffer
    iboMesh->bind();
    numMeshIndices = (numParticlesX - 1)*(numParticlesY - 1)*2*3;
    int* indices = new int[numMeshIndices];
    int idx = 0;
    for (int i = 0; i < numParticlesX-1; i++) {
        for (int j = 0; j < numParticlesY-1; j++) {
            indices[idx  ] = i*numParticlesY + j;
            indices[idx+1] = (i+1)*numParticlesY + j;
            indices[idx+2] = i*numParticlesY + j + 1;
            indices[idx+3] = i*numParticlesY + j + 1;
            indices[idx+4] = (i+1)*numParticlesY + j;
            indices[idx+5] = (i+1)*numParticlesY + j + 1;
            idx += 6;
        }
    }
    void* bufptr = iboMesh->mapRange(0, numMeshIndices*sizeof(int),
                                     QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(indices), numMeshIndices*sizeof(int));
    iboMesh->unmap();
    iboMesh->release();
    delete[] indices;
    glutils::checkGLError();
}


void SceneCloth::updateSprings()
{
    double ks = widget->getStiffness();
    double kd = widget->getDamping();

    // here I update all ks and kd parameters.
    // idea: if you want to enable/disable a spring type, you can set ks to 0 for these
    for (ForceSpring* f : springsStretch) {
        f->setSpringConstant(ks);
        f->setDampingCoeff(kd);
    }
    for (ForceSpring* f : springsShear) {
        f->setSpringConstant(ks);
        f->setDampingCoeff(kd);
    }
    for (ForceSpring* f : springsBend) {
        f->setSpringConstant(ks);
        f->setDampingCoeff(kd);
    }
}

void SceneCloth::updateSimParams()
{
    double g = widget->getGravity();
    fGravity->setAcceleration(Vec3(0, -g, 0));

    updateSprings();

    for (Particle* p : system.getParticles()) {
        p->radius = widget->getParticleRadius();
    }

    showParticles = widget->showParticles();
}

void SceneCloth::freeAnchors()
{
    fixedParticle = std::vector<bool>(numParticles, false);
}

void SceneCloth::paint(const Camera& camera)
{
    QOpenGLFunctions* glFuncs = nullptr;
    glFuncs = QOpenGLContext::currentContext()->functions();

    shaderPhong->bind();
    shaderPhong->setUniformValue("normalSign", 1.0f);

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    shaderPhong->setUniformValue("ProjMatrix", camProj);
    shaderPhong->setUniformValue("ViewMatrix", camView);

    // lighting
    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(80,80,80)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // draw the particle spheres
    QMatrix4x4 modelMat;
    if (showParticles) {
        vaoSphereS->bind();
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        for (int i = 0; i < numParticles; i++) {
            const Particle* particle = system.getParticle(i);
            Vec3   p = particle->pos;
            Vec3   c = particle->color;
            if (fixedParticle[i])      c = Vec3(63/255.0, 72/255.0, 204/255.0);
            if (i == selectedParticle) c = Vec3(1.0,0.9,0);

            modelMat = QMatrix4x4();
            modelMat.translate(p[0], p[1], p[2]);
            modelMat.scale(particle->radius);
            shaderPhong->setUniformValue("ModelMatrix", modelMat);
            shaderPhong->setUniformValue("matdiff", GLfloat(c[0]), GLfloat(c[1]), GLfloat(c[2]));
            glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereS, GL_UNSIGNED_INT, 0);
        }
    }

    // TODO: draw colliders and walls

    // draw sphere
    vaoSphereS->bind();
    Vec3 cc = colliderSphere.getCenter();
    modelMat = QMatrix4x4();
    modelMat.translate(cc[0], cc[1], cc[2]);
    modelMat.scale(colliderSphere.getRadius());
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    shaderPhong->setUniformValue("matdiff", 0.8f, 0.4f, 0.4f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.0f);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphereS, GL_UNSIGNED_INT, 0);

    shaderPhong->release();


    // update cloth mesh VBO coords
    vboMesh->bind();
    float* pos = new float[3*numParticles];
    for (int i = 0; i < numParticles; i++) {
        pos[3*i  ] = system.getParticle(i)->pos.x();
        pos[3*i+1] = system.getParticle(i)->pos.y();
        pos[3*i+2] = system.getParticle(i)->pos.z();
    }
    void* bufptr = vboMesh->mapRange(0, 3*numParticles*sizeof(float),
                       QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
    memcpy(bufptr, (void*)(pos), 3*numParticles*sizeof(float));
    vboMesh->unmap();
    vboMesh->release();
    delete[] pos;

    // draw mesh
    shaderCloth->bind();
    shaderCloth->setUniformValue("ProjMatrix", camProj);
    shaderCloth->setUniformValue("ViewMatrix", camView);
    shaderCloth->setUniformValue("NormalMatrix", camView.normalMatrix());
    shaderCloth->setUniformValue("matdiffFront", 0.7f, 0.0f, 0.0f);
    shaderCloth->setUniformValue("matspecFront", 1.0f, 1.0f, 1.0f);
    shaderCloth->setUniformValue("matshinFront", 100.0f);
    shaderCloth->setUniformValue("matdiffBack", 0.7f, 0.3f, 0.0f);
    shaderCloth->setUniformValue("matspecBack", 0.0f, 0.0f, 0.0f);
    shaderCloth->setUniformValue("matshinBack", 0.0f);
    shaderCloth->setUniformValue("numLights", numLights);
    shaderCloth->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderCloth->setUniformValueArray("lightColor", lightColor, numLights);
    vaoMesh->bind();
    glFuncs->glDrawElements(GL_TRIANGLES, numMeshIndices, GL_UNSIGNED_INT, 0);
    vaoMesh->release();
    shaderCloth->release();

    glutils::checkGLError();
}


void SceneCloth::update(double dt)
{

    // fixed particles: no velocity, no force acting
    for (int i = 0; i < numParticles; i++) {
        if (fixedParticle[i]) {
            Particle* p = system.getParticle(i);
            p->vel = Vec3(0,0,0);
            p->force = Vec3(0,0,0);
        }
    }

    // integration step
    Vecd ppos = system.getPositions();
    integrator->step(system, dt);
    system.setPreviousPositions(ppos);

    // user interaction
    if (selectedParticle >= 0) {
        Particle* p = system.getParticle(selectedParticle);
        p->pos = cursorWorldPos; //TODO: assign cursor world position (see code, it's already computed)
        p->vel = Vec3(0,0,0);

        // (optional) simple bounds clamp during drag
        Vec3 bmin, bmax; getSceneBounds(bmin, bmax);
        p->pos = p->pos.cwiseMax(bmin).cwiseMin(bmax);
        // TODO: test and resolve for collisions during user movement
        Collision colInfo;
        if (colliderSphere.testCollision(p, colInfo)) {
            colliderSphere.resolveCollision(p, colInfo, colBounce, colFriction);
        }

    }

    // Relaxation

    auto relaxBag = [&](const std::vector<ForceSpring*>& bag)
    {
        for (ForceSpring* s : bag)
        {
            Particle* a = s->getParticle1();
            Particle* b = s->getParticle2();

            const bool aFixed = fixedParticle[a->id];
            const bool bFixed = fixedParticle[b->id];

            // inverse-mass style weights: fixed -> 0
            const double wA = aFixed ? 0.0 : 1.0 / std::max(1e-12, a->mass);
            const double wB = bFixed ? 0.0 : 1.0 / std::max(1e-12, b->mass);
            const double wSum = wA + wB;
            if (wSum <= 0.0) continue;

            Eigen::Vector3d d = b->pos - a->pos;
            double len = d.norm();
            if (len < 1e-9) continue;

            const double L0 = s->getRestLength();
            double diff = len - L0;

            // how strongly we correct (0..1)
            const double relax = 0.8;
            Eigen::Vector3d n = d / len;
            Eigen::Vector3d corr = relax * diff * n;

            // move only the non-fixed ones
            if (!aFixed) a->pos += (wA / wSum) * corr;
            if (!bFixed) b->pos -= (wB / wSum) * corr;
        }
    };

    // a few iterations help
    const int relaxIters = 3;
    for (int it = 0; it < relaxIters; ++it) {
        relaxBag(springsStretch);
        relaxBag(springsShear);
        relaxBag(springsBend);
    }

    // collisions
    Collision colInfo;
    for (Particle* p : system.getParticles()) {
        // TODO: test and resolve collisions
        if (colliderSphere.testCollision(p, colInfo)) {
            colliderSphere.resolveCollision(p, colInfo, colBounce, colFriction);
        }
    }

    // needed after we have done collisions and relaxation, since spring forces depend on p and v
    system.updateForces();
}


void SceneCloth::mousePressed(const QMouseEvent* e, const Camera& cam)
{
    grabX = e->pos().x();
    grabY = e->pos().y();

    if (!(e->modifiers() & Qt::ControlModifier)) {

        Vec3 rayDir = cam.getRayDir(grabX, grabY);
        Vec3 origin = cam.getPos();

        selectedParticle = -1;
        double bestT = 1e30;
        const double pickR = std::max(1.5*particleRadius, 1.5);
        for (int i = 0; i < numParticles; ++i) {
            const Particle* p = system.getParticle(i);
            Vec3 op = p->pos - origin;

            // Project op onto rayDir
            double t = op.dot(rayDir);                // param along the ray
            if (t < 0) continue;                      // behind camera

            // Closest distance from point to ray
            Vec3 closest = origin + rayDir * t;
            double d = (p->pos - closest).norm();

            if (d <= pickR && t < bestT) {
                bestT = t;
                selectedParticle = i;
            }
        }

        if (selectedParticle >= 0) {
            cursorWorldPos = system.getParticle(selectedParticle)->pos;
        }
    }
}

void SceneCloth::mouseMoved(const QMouseEvent* e, const Camera& cam)
{
    int dx = e->pos().x() - grabX;
    int dy = e->pos().y() - grabY;
    grabX = e->pos().x();
    grabY = e->pos().y();

    if (e->modifiers() & Qt::ControlModifier) {

    }
    else if (e->modifiers() & Qt::ShiftModifier) {

    }
    else {
        if (selectedParticle >= 0) {
            double d = -(system.getParticle(selectedParticle)->pos - cam.getPos()).dot(cam.zAxis());
            Vec3 disp = cam.worldSpaceDisplacement(dx, -dy, d);
            cursorWorldPos += disp;
        }
    }
}

void SceneCloth::mouseReleased(const QMouseEvent*, const Camera&)
{
    selectedParticle = -1;
}

void SceneCloth::keyPressed(const QKeyEvent* e, const Camera&)
{
    if (selectedParticle >= 0 && e->key() == Qt::Key_F) {
        fixedParticle[selectedParticle] = true;
        Particle* p = system.getParticle(selectedParticle);
        p->prevPos = p->pos;
        p->vel = Vec3(0,0,0);
        p->force = Vec3(0,0,0);
    }
}
