#include "widgettestintegrators.h"
#include "ui_widgettestintegrators.h"

WidgetTestIntegrators::WidgetTestIntegrators(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetTestIntegrators)
{
    ui->setupUi(this);

    connect(ui->btnCriticalDamp, SIGNAL(clicked()), this, SLOT(setCriticalDamp()));
    connect(ui->integrator, SIGNAL(currentIndexChanged(int)), this, SLOT(changedParameter()));
    connect(ui->stiffness, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->damping, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->forceMagnitude, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->forceFrequency, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->initialPos, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
    connect(ui->initialVel, SIGNAL(valueChanged(double)), this, SLOT(changedParameter()));
}

WidgetTestIntegrators::~WidgetTestIntegrators()
{
    delete ui;
}

void WidgetTestIntegrators::changedParameter()
{
    emit updatedParameters();
}

void WidgetTestIntegrators::setCriticalDamp()
{
    // assuming m=1
    ui->damping->setValue(2*std::sqrt(getStiffness()));
}

int WidgetTestIntegrators::getIntegratorType() const
{
    return ui->integrator->currentIndex();
}

double WidgetTestIntegrators::getStiffness() const
{
    return ui->stiffness->value();
}

double WidgetTestIntegrators::getDamping() const
{
    return ui->damping->value();
}

double WidgetTestIntegrators::getForceMagnitude() const
{
    return ui->forceMagnitude->value();
}

double WidgetTestIntegrators::getForceFrequency() const
{
    return ui->forceFrequency->value();
}

double WidgetTestIntegrators::getInitialPos() const
{
    return ui->initialPos->value();
}

double WidgetTestIntegrators::getInitialVel() const
{
    return ui->initialVel->value();
}

bool WidgetTestIntegrators::drawPhaseAnalytic() const
{
    return ui->drawPhaseAnalytic->isChecked();
}

bool WidgetTestIntegrators::drawPhaseNumerical() const
{
    return ui->drawPhaseNumerical->isChecked();
}

bool WidgetTestIntegrators::drawOscillationAnalytic() const
{
    return ui->drawOscillationAnalytic->isChecked();
}

bool WidgetTestIntegrators::drawOscillationNumerical() const
{
    return ui->drawOscillationNumerical->isChecked();
}

bool WidgetTestIntegrators::drawScaleVectors() const
{
    return ui->drawScaleVectors->isChecked();
}

double WidgetTestIntegrators::getTimeScale() const
{
    return ui->timeScale->value();
}

void WidgetTestIntegrators::setInfo(const QString &txt)
{
    ui->textSimVals->setText(txt);
}
