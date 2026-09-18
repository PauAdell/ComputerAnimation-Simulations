#include "widgetcloth.h"
#include "ui_widgetcloth.h"

WidgetCloth::WidgetCloth(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetCloth)
{
    ui->setupUi(this);

    connect(ui->btnUpdate, &QPushButton::clicked,
            this, [=] (void) { emit updatedParameters(); });
    connect(ui->btnFreeAnchors, &QPushButton::clicked,
            this, [=] (void) { emit freeAnchors(); });
}

WidgetCloth::~WidgetCloth()
{
    delete ui;
}

double WidgetCloth::getGravity() const {
    return ui->gravity->value();
}

double WidgetCloth::getStiffness() const {
    return ui->springStiffness->value();
}

double WidgetCloth::getDamping() const {
    return ui->springDamping->value();
}

Vec2 WidgetCloth::getDimensions() const {
    return Vec2(ui->sizeX->value(),
                ui->sizeY->value());
}

Vec2i WidgetCloth::getNumParticles() const {
    return Vec2i(ui->numParticlesX->value(),
                 ui->numParticlesY->value());
}

double WidgetCloth::getParticleRadius() const {
    return ui->particleRad->value();
}

bool WidgetCloth::showParticles() const {
    return ui->showParticles->isChecked();
}

int WidgetCloth::getIntegrator() const {
    return ui->integrator->currentIndex();
}

void WidgetCloth::setIntegrator(int idx) const {
    return ui->integrator->setCurrentIndex(idx);
}
