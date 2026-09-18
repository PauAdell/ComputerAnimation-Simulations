#include "widgettestcolliders.h"
#include "ui_widgettestcolliders.h"

WidgetTestColliders::WidgetTestColliders(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetTestColliders)
{
    ui->setupUi(this);

    connect(ui->tabCollider, SIGNAL(currentChanged(int)), this, SLOT(changedParameter()));

    connect(ui->plane_nx, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->plane_ny, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->plane_nz, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->plane_d, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));

    connect(ui->sphere_cx, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->sphere_cy, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->sphere_cz, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->sphere_rad, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));

    connect(ui->aabb_minx, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->aabb_miny, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->aabb_minz, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->aabb_maxx, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->aabb_maxy, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->aabb_maxz, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));

    connect(ui->bounciness, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->friction, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->particleScale, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->checkCCD, SIGNAL(toggled(bool)), this, SLOT(changedParameter()));

    connect(ui->prevX, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->prevY, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->prevZ, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));

    connect(ui->currX, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->currY, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->currZ, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
}

WidgetTestColliders::~WidgetTestColliders()
{
    delete ui;
}

int WidgetTestColliders::getColliderType() const
{
    return ui->tabCollider->currentIndex();
}

Vec3 WidgetTestColliders::getPlaneNormal() const
{
    return Vec3(ui->plane_nx->value(), ui->plane_ny->value(), ui->plane_nz->value());
}

double WidgetTestColliders::getPlaneD() const
{
    return ui->plane_d->value();
}

Vec3 WidgetTestColliders::getSphereCenter() const
{
    return Vec3(ui->sphere_cx->value(), ui->sphere_cy->value(), ui->sphere_cz->value());
}

double WidgetTestColliders::getSphereRadius() const
{
    return ui->sphere_rad->value();
}

Vec3 WidgetTestColliders::getBoxMin() const
{
    return Vec3(ui->aabb_minx->value(), ui->aabb_miny->value(), ui->aabb_minz->value());
}

Vec3 WidgetTestColliders::getBoxMax() const
{
    return Vec3(ui->aabb_maxx->value(), ui->aabb_maxy->value(), ui->aabb_maxz->value());
}

double WidgetTestColliders::getRestitution() const
{
    return ui->bounciness->value();
}

double WidgetTestColliders::getFriction() const
{
    return ui->friction->value();
}

double WidgetTestColliders::getParticleRadius() const
{
    return ui->particleScale->value();
}

bool WidgetTestColliders::getContinuousCollision() const
{
    return ui->checkCCD->isChecked();
}

Vec3 WidgetTestColliders::getPrevPos() const
{
    return Vec3(ui->prevX->value(), ui->prevY->value(), ui->prevZ->value());
}

Vec3 WidgetTestColliders::getCurrPos() const
{
    return Vec3(ui->currX->value(), ui->currY->value(), ui->currZ->value());
}

void WidgetTestColliders::setCollisionInfo(const QString &text)
{
    ui->textCollision->setText(text);
}

void WidgetTestColliders::changedParameter()
{
    emit updatedParameters();
}
