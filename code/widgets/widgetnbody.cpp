#include "widgetnbody.h"
#include "ui_widgetnbody.h"

WidgetNBody::WidgetNBody(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetNBody)
{
    ui->setupUi(this);

    connect(ui->trajectory, QOverload<bool>::of(&QCheckBox::toggled),
            this, [=] () { emit drawTrajectoriesChanged(); });
}

WidgetNBody::~WidgetNBody()
{
    delete ui;
}

int WidgetNBody::getIntegratorType() const {
    return ui->integrator->currentIndex();
}

int WidgetNBody::getBodiesLayout() const {
    return ui->bodiesLayout->currentIndex();
}

int WidgetNBody::getNumBodies() const {
    return ui->numBodies->value();
}

double WidgetNBody::getMassRange() const {
    return ui->massRange->value();
}

double WidgetNBody::getSmoothingA() const {
    return ui->smoothA->value();
}

double WidgetNBody::getSmoothingB() const {
    return ui->smoothB->value();
}

bool WidgetNBody::drawTrajectories() const {
    return ui->trajectory->isChecked();
}
