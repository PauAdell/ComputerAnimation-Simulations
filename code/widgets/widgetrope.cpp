#include "widgetrope.h"
#include "ui_widgetrope.h"

WidgetRope::WidgetRope(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetRope)
{
    ui->setupUi(this);

    connect(ui->btnUpdate, &QPushButton::clicked,
            this, [=] (void) { emit updatedParameters(); });
}

WidgetRope::~WidgetRope()
{
    delete ui;
}

double WidgetRope::getGravity() const {
    return ui->gravity->value();
}

double WidgetRope::getRopeLength() const {
    return ui->ropeLength->value();
}
int WidgetRope::getNumParticles() const {
    return ui->numParticles->value();
}

double WidgetRope::getStiffness() const {
    return ui->springStiffness->value();
}

double WidgetRope::getDamping() const {
    return ui->springDamping->value();
}

double WidgetRope::getParticleScale() const {
    return ui->particleScale->value();
}

bool WidgetRope::checkCollisions() const {
    return ui->checkCollisions->isChecked();
}

bool WidgetRope::anchorRopeEnd() const {
    return ui->anchoredEnd->isChecked();
}

bool WidgetRope::showParticles() const {
    return ui->showParticles->isChecked();
}

int WidgetRope::getStartConfig() const {
    return ui->startConfig->currentIndex();
}

int WidgetRope::getIntegrator() const {
    return ui->integrator->currentIndex();
}

void WidgetRope::setIntegrator(int idx) const {
    return ui->integrator->setCurrentIndex(idx);
}
