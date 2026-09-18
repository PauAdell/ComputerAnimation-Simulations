#include "scenetestintegrators.h"
#include "glutils.h"
#include "model.h"
#include "forces.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>


class ForceDampedHarmonicOscillator1D : public Force
{
public:
    ForceDampedHarmonicOscillator1D() {};
    ~ForceDampedHarmonicOscillator1D() {};

    void setSpringConstant(double k) { ks = k; }
    double getSpringConstant() const { return ks; }

    void setDampingCoeff(double k) { kd = k; }
    double getDampingCoeff() const { return kd; }

    void setDrivingForceMagnitude(double a) { F0 = a; }
    double getDrivingForceMagnitude() const { return F0; }

    void setDrivingForceFrequency(double w) { Fw = w; }
    double getDrivingForceFrequency() const { return Fw; }

    void setTimePointer(const double* tp) { timePtr = tp; }

    virtual void apply() {
        double t = *timePtr;
        for (Particle* p : particles) {
            p->force[dim] += evaluate(t, p->pos[dim], p->vel[dim]);
        }
    }

    double evaluate(double t, double x, double v) {
        return F0*cos(Fw*t) - ks*x - kd*v;
    }

protected:
    int dim = 0;
    const double* timePtr = nullptr;
    double ks;  // spring constant
    double kd;  // damping constant
    double F0;  // driving force amplitude
    double Fw;  // driving frequency
};


SceneTestIntegrators::SceneTestIntegrators() {
    widget = new WidgetTestIntegrators();
}

SceneTestIntegrators::~SceneTestIntegrators() {
    if (widget)      delete widget;
    if (shaderPhong) delete shaderPhong;
    if (shaderLines) delete shaderLines;
    if (vaoSphere)   delete vaoSphere;
    if (vaoCylinder) delete vaoCylinder;
    if (vaoCone)     delete vaoCone;
    if (vboTrajectoryPoints) delete vboTrajectoryPoints;
    if (vaoTrajectory) delete vaoTrajectory;
    if (force)      delete force;
    if (particle)   delete particle;
    if (integrator) delete integrator;
}

void SceneTestIntegrators::initialize() {

    // load shaders
    shaderPhong = glutils::loadShaderProgram(":/shaders/phong.vert", ":/shaders/phong.frag");
    shaderLines = glutils::loadShaderProgram(":/shaders/lines.vert", ":/shaders/lines.geom", ":/shaders/lines.frag");

    // create VAOs
    Model sphere = Model::createIcosphere(3);
    vaoSphere = glutils::createVAO(shaderPhong, &sphere, buffers);
    numFacesSphere = sphere.numFaces();

    Model cylinder = Model::createCylinder(32, true);
    vaoCylinder = glutils::createVAO(shaderPhong, &cylinder, buffers);
    numFacesCylinder = cylinder.numFaces();

    Model cone = Model::createCone(16, 1.0);
    vaoCone = glutils::createVAO(shaderPhong, &cone, buffers);
    numFacesCone = cone.numFaces();

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

    // physical system
    particle = new Particle();
    force = new ForceDampedHarmonicOscillator1D();
    force->addInfluencedParticle(particle);
    force->setTimePointer(particleSystem.getTimePointer());
    particleSystem.addParticle(particle);
    particleSystem.addForce(force);

    reset();

    glutils::checkGLError();
}

void SceneTestIntegrators::reset()
{
    init_x = widget->getInitialPos();
    init_v = widget->getInitialVel();

    time = 0;
    analytic_x = init_x;
    analytic_v = init_v;

    trajectoryAnalytic.clear();
    trajectoryNumerical.clear();
    oscillationAnalytic.clear();
    oscillationNumerical.clear();

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

    firstIteration = true;
    particleSystem.setTime(0);
    particle->pos = Vec3(0,0,0);
    particle->vel = Vec3(0,0,0);
    particle->pos[0] = widget->getInitialPos();
    particle->vel[0] = widget->getInitialVel();
    particleSystem.resetFirstStep();
    particleSystem.updateForces();

    force->setSpringConstant(widget->getStiffness());
    force->setDampingCoeff(widget->getDamping());
    force->setDrivingForceMagnitude(widget->getForceMagnitude());
    force->setDrivingForceFrequency(widget->getForceFrequency());

    update(0);
}

void SceneTestIntegrators::paint(const Camera& camera)
{
    // pointer to current context OpenGL functions
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();

    // camera matrices
    QMatrix4x4 camProj = camera.getPerspectiveMatrix();
    QMatrix4x4 camView = camera.getViewMatrix();
    QVector3D phaseCtr(10, 0, 0);
    QVector3D oscillatorCtr(-5, 0, 0);
    QMatrix4x4 modelMat;
    QMatrix4x4 ctrTranslation;

    // start with phong shaded elements
    shaderPhong->bind();
    shaderPhong->setUniformValue("normalSign", 1.0f);
    shaderPhong->setUniformValue("ProjMatrix", camProj);

    // lighting
    const int numLights = 1;
    const QVector3D lightPosWorld[numLights] = {QVector3D(0,0,100)};
    const QVector3D lightColor[numLights] = {QVector3D(1,1,1)};
    QVector3D lightPosCam[numLights];
    for (int i = 0; i < numLights; i++) {
        lightPosCam[i] = camView.map(lightPosWorld[i]);
    }
    shaderPhong->setUniformValue("numLights", numLights);
    shaderPhong->setUniformValueArray("lightPos", lightPosCam, numLights);
    shaderPhong->setUniformValueArray("lightColor", lightColor, numLights);

    // center for drawing the phase space diagram
    ctrTranslation.setToIdentity();
    ctrTranslation.translate(phaseCtr);
    shaderPhong->setUniformValue("ViewMatrix", camView * ctrTranslation);

    // draw axes
    vaoCylinder->bind();
    shaderPhong->setUniformValue("matdiff", 0.2f, 0.2f, 0.2f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.f);

    modelMat.setToIdentity();
    modelMat.scale(0.05, 10, 0.05);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);

    modelMat.setToIdentity();
    modelMat.rotate(90, QVector3D(0, 0, 1));
    modelMat.scale(0.05, 10, 0.05);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);

    // draw vector field
    shaderPhong->setUniformValue("matdiff", 0.8f, 0.8f, 0.8f);
    double dx = 1;
    double dy = 1;
    double xmin = -10, ymin = -10;
    double xmax =  10, ymax =  10;
    double s = 0.7*std::min(dx, dy);
    double phase_x = xmin;
    while (phase_x <= xmax) {
        double phase_y = ymin;
        while (phase_y <= ymax) {

            // vector at this point
            Vec3 f(phase_y, force->evaluate(time, phase_x, phase_y), 0);
            double magnitude = 1.0;
            if (widget->drawScaleVectors()) {
                magnitude *= std::sqrt(f.norm()); //sqrt to avoid scaling too much
            }

            if (f.norm() > 1e-6) {
                // position the arrow
                double a = Math::toDeg(std::atan2(-f[0], f[1]));
                modelMat.setToIdentity();
                modelMat.translate(phase_x, phase_y, 0);
                modelMat.rotate(a, QVector3D(0, 0, 1));
                modelMat.scale(s * magnitude);

                // draw arrow (cone + cylinder)
                modelMat.scale(1.0/3.0); // arrow size is 3
                vaoCone->bind();
                modelMat.translate(0, 2.0, 0);
                modelMat.scale(0.5, 1, 0.5);
                shaderPhong->setUniformValue("ModelMatrix", modelMat);
                glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCone, GL_UNSIGNED_INT, 0);
                modelMat.scale(2, 1, 2);
                vaoCylinder->bind();
                modelMat.translate(0, -1.0, 0);
                modelMat.scale(0.2, 1, 0.2);
                shaderPhong->setUniformValue("ModelMatrix", modelMat);
                glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);
            }

            phase_y += dy;
        }
        phase_x += dx;
    }

    // draw the analytic solution position in phase space    
    if (widget->drawPhaseAnalytic()) {
        vaoSphere->bind();
        shaderPhong->setUniformValue("matdiff", 0.1f, 0.8f, 0.1f);
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        modelMat.setToIdentity();
        modelMat.translate(analytic_x, analytic_v, 0);
        modelMat.scale(0.4);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
    }

    // draw the numerical solution position in phase space
    if (widget->drawPhaseNumerical()) {
        vaoSphere->bind();
        shaderPhong->setUniformValue("matdiff", 0.8f, 0.1f, 0.1f);
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        modelMat.setToIdentity();
        modelMat.translate(particle->pos[0], particle->vel[0], 0);
        modelMat.scale(0.4);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
    }



    // draw oscillators
    ctrTranslation.setToIdentity();
    ctrTranslation.translate(oscillatorCtr);
    shaderPhong->setUniformValue("ViewMatrix", camView * ctrTranslation);

    // axes
    // draw axes
    vaoCylinder->bind();
    shaderPhong->setUniformValue("matdiff", 0.2f, 0.2f, 0.2f);
    shaderPhong->setUniformValue("matspec", 0.0f, 0.0f, 0.0f);
    shaderPhong->setUniformValue("matshin", 0.f);

    modelMat.setToIdentity();
    modelMat.scale(0.05, 10, 0.05);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);

    modelMat.setToIdentity();
    modelMat.translate(-20, 0, 0);
    modelMat.rotate(90, QVector3D(0, 0, 1));
    modelMat.scale(0.05, 20, 0.05);
    shaderPhong->setUniformValue("ModelMatrix", modelMat);
    glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesCylinder, GL_UNSIGNED_INT, 0);

    // draw the analytic solution oscillator
    if (widget->drawOscillationAnalytic()) {
        vaoSphere->bind();
        shaderPhong->setUniformValue("matdiff", 0.1f, 0.8f, 0.1f);
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        modelMat.setToIdentity();
        modelMat.translate(0, analytic_x, 0);
        modelMat.scale(0.4);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
    }

    // draw the numerical solution oscillator
    if (widget->drawOscillationNumerical()) {
        vaoSphere->bind();
        shaderPhong->setUniformValue("matdiff", 0.8f, 0.1f, 0.1f);
        shaderPhong->setUniformValue("matspec", 1.0f, 1.0f, 1.0f);
        shaderPhong->setUniformValue("matshin", 100.f);
        modelMat.setToIdentity();
        modelMat.translate(0, particle->pos[0], 0);
        modelMat.scale(0.4);
        shaderPhong->setUniformValue("ModelMatrix", modelMat);
        glFuncs->glDrawElements(GL_TRIANGLES, 3*numFacesSphere, GL_UNSIGNED_INT, 0);
    }

    shaderPhong->release();


    shaderLines->bind();
    shaderLines->setUniformValue("ProjMatrix", camProj);
    shaderLines->setUniformValue("radius", float(0.075));
    shaderLines->setUniformValue("shading", false);
    shaderLines->setUniformValue("darkenTrail", true);
    shaderLines->setUniformValue("numVerts", int(trajectoryAnalytic.size()));
    shaderLines->setUniformValue("maxVerts", MAX_TRAJ_POINTS);

    if (widget->drawOscillationAnalytic() || widget->drawOscillationNumerical()) {
        // now draw the oscillator function
        ctrTranslation.setToIdentity();
        ctrTranslation.translate(oscillatorCtr);
        ctrTranslation.scale(widget->getTimeScale(), 1, 1);
        ctrTranslation.translate(QVector3D(-time, 0, 0));
        shaderLines->setUniformValue("ViewMatrix", camView * ctrTranslation);

        if (widget->drawOscillationAnalytic()) {
            updateTrajectoryCoordsBuffer(oscillationAnalytic);
            shaderLines->setUniformValue("matdiff", 0.1f, 0.8f, 0.1f);
            vaoTrajectory->bind();
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryAnalytic.size()),
                                                             MAX_TRAJ_POINTS));
            vaoTrajectory->release();
        }
        if (widget->drawOscillationNumerical()) {
            updateTrajectoryCoordsBuffer(oscillationNumerical);
            shaderLines->setUniformValue("matdiff", 0.8f, 0.1f, 0.1f);
            vaoTrajectory->bind();
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryNumerical.size()),
                                                             MAX_TRAJ_POINTS));
            vaoTrajectory->release();
        }
    }

    // draw phase-space trajectories
    if (widget->drawPhaseNumerical() || widget->drawPhaseAnalytic()) {
        ctrTranslation.setToIdentity();
        ctrTranslation.translate(phaseCtr);
        shaderLines->setUniformValue("ViewMatrix", camView * ctrTranslation);
        if (widget->drawPhaseAnalytic()) {
            updateTrajectoryCoordsBuffer(trajectoryAnalytic);
            shaderLines->setUniformValue("matdiff", 0.1f, 0.8f, 0.1f);
            vaoTrajectory->bind();
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryAnalytic.size()),
                                                             MAX_TRAJ_POINTS));
            vaoTrajectory->release();
        }
        if (widget->drawPhaseNumerical()) {
            updateTrajectoryCoordsBuffer(trajectoryNumerical);
            shaderLines->setUniformValue("matdiff", 0.8f, 0.1f, 0.1f);
            vaoTrajectory->bind();
            glFuncs->glDrawArrays(GL_LINE_STRIP, 0, std::min(static_cast<unsigned int>(trajectoryNumerical.size()),
                                                             MAX_TRAJ_POINTS));
            vaoTrajectory->release();
        }
    }

    shaderLines->release();

    glutils::checkGLError();
}


void SceneTestIntegrators::update(double dt)
{
    time += dt;

    QString info = "";

    /*
     * NUMERICAL SOLUTION
     */
    Vecd pos = particleSystem.getPositions();
    if (firstIteration && dt > 0) {
        particleSystem.setPreviousPositions(pos - dt*particleSystem.getVelocities());
        firstIteration = false;
    }
    if (integrator && dt > 0) integrator->step(particleSystem, dt);
    particleSystem.setPreviousPositions(pos);

    /*
     * ANALYTIC SOLUTION
     */
    const double k = force->getSpringConstant();
    const double b = force->getDampingCoeff();
    const double F_0 = force->getDrivingForceMagnitude();
    const double wf = force->getDrivingForceFrequency();
    const double m  = particle->mass;

    // Derived parameters
    const double w0 = std::sqrt(k / m);   // Natural frequency
    const double gamma = b / (2.0 * m);   // Damping factor (half of damping coefficient per mass)
    const double zheta = gamma / w0;

    // steady oscillation due to driving force (independent on initial conditions)
    double A_steady = F_0 / (m * std::sqrt(std::pow(w0 * w0 - wf * wf, 2) + std::pow(2 * gamma * wf, 2)));
    double delta = std::atan2(2 * gamma * wf, w0 * w0 - wf * wf);
    double x_steady =  A_steady * std::cos(wf * time - delta);
    double v_steady = -A_steady * wf * std::sin(wf * time - delta);

    // Initial conditions
    double x0 = init_x - A_steady * std::cos(-delta);
    double v0 = init_v + A_steady * wf * std::sin(-delta);

    // Transient solution: check for overdamped, critically damped, or underdamped case
    double x_transient = 0;
    double v_transient = 0;
    if (zheta > 1) {
        // Overdamped system
        info += "Overdamped\n";

        double root1 = -gamma + std::sqrt(gamma * gamma - w0 * w0);
        double root2 = -gamma - std::sqrt(gamma * gamma - w0 * w0);
        double C1 = (v0 - root2 * x0) / (root1 - root2);
        double C2 = x0 - C1;
        x_transient = C1 * std::exp(root1 * time) + C2 * std::exp(root2 * time);
        v_transient = C1 * root1 * std::exp(root1 * time) + C2 * root2 * std::exp(root2 * time);

    } else if (zheta == 1) {
        // Critically damped system
        info += "Critically damped\n";

        double C1 = x0;
        double C2 = v0 + gamma * x0;
        x_transient = (C1 + C2 * time) * std::exp(-gamma * time);
        v_transient = (C2 - gamma * (C1 + C2 * time)) * std::exp(-gamma * time);

    } else {
        // Underdamped system
        info += "Underdamped\n";

        double wd = std::sqrt(w0*w0 - gamma*gamma);

        double A_transient = std::sqrt(x0 * x0 + std::pow((v0 + gamma * x0) / wd, 2));
        double phi = std::atan2(-v0 - gamma * x0, x0 * wd);
        x_transient =  A_transient * std::exp(-gamma * time) * std::cos(wd * time + phi);
        v_transient = -A_transient * std::exp(-gamma * time) *
                       (gamma * std::cos(wd * time + phi) + wd * std::sin(wd * time + phi));
    }

    // motion has both the steady and transient terms
    analytic_x = x_transient + x_steady;
    analytic_v = v_transient + v_steady;


    // save trajectories
    appendTrajectoryPoint(trajectoryAnalytic,  Vec3(analytic_x, analytic_v, 0));
    appendTrajectoryPoint(trajectoryNumerical, Vec3(particle->pos[0], particle->vel[0], 0));
    appendTrajectoryPoint(oscillationAnalytic, Vec3(time, analytic_x, 0));
    appendTrajectoryPoint(oscillationNumerical, Vec3(time, particle->pos[0], 0));

    // display info
    info += "\n";
    info += "ANALYTICAL\n";
    info += "x: " + QString::number(analytic_x, 'f', 5) + "\n";
    info += "v: " + QString::number(analytic_v, 'f', 5) + "\n";
    info += "\n";
    info += "NUMERICAL\n";
    info += "x: " + QString::number(particle->pos[0], 'f', 5) + "\n";
    info += "v: " + QString::number(particle->vel[0], 'f', 5);
    widget->setInfo(info);
}

void SceneTestIntegrators::mousePressed(const QMouseEvent*, const Camera&)
{
}

void SceneTestIntegrators::mouseMoved(const QMouseEvent*, const Camera&)
{
}

void SceneTestIntegrators::mouseReleased(const QMouseEvent*, const Camera&)
{

}

void SceneTestIntegrators::updateTrajectoryCoordsBuffer(const std::list<Vec3> &trajectory)
{
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

void SceneTestIntegrators::appendTrajectoryPoint(std::list<Vec3> &trajectory, const Vec3 &p)
{
    trajectory.push_back(p);
    if (trajectory.size() > MAX_TRAJ_POINTS) {
        trajectory.pop_front();
    }
}

