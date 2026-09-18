#include "widgetfluidsim.h"
#include "ui_widgetfluidsim.h"
#include <QPushButton>

WidgetFluidSim::WidgetFluidSim(QWidget *parent)
    : QWidget(parent), ui(new Ui::WidgetFluidSim)
{
    ui->setupUi(this);

    // Wire the Update button just like the fountain widget
    connect(ui->btnUpdate, &QPushButton::clicked, this,
            [=](){ emit updatedParameters(); });
    // Spawn "N" particles once
    connect(ui->btnSpawn, &QPushButton::clicked, this, [=]{
        emit spawnClicked();
    });

    // Toggle continuous emission
    connect(ui->chkAutoEmit, &QCheckBox::toggled, this, [=](bool on){
        emit autoEmitToggled(on);
    });

    connect(ui->spinSpawnCount, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [=](double n){
        emit spawnCountChanged(static_cast<int>(n));  // or make signal take double if you prefer
    });

    connect(ui->spinMaxParticles, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [=](double n){
        emit maxParticlesChanged(static_cast<int>(n));
    });
}

WidgetFluidSim::~WidgetFluidSim()
{
    delete ui;
}

double WidgetFluidSim::getGravity() const  { return ui->gravity->value(); }
