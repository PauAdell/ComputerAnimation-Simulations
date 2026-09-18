#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sceneprojectiles.h"
#include "scenefountain.h"
#include "scenerope.h"
#include "scenecloth.h"
#include "scenetestintegrators.h"
#include "scenetestcolliders.h"
#include "scenenbody.h"
#include "scenefluidsim.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnSimstep, SIGNAL(clicked()), ui->openGLWidget, SLOT(doSimStep()));
    connect(ui->btnSimloop, SIGNAL(clicked()), ui->openGLWidget, SLOT(doSimLoop()));
    connect(ui->btnPause,   SIGNAL(clicked()), ui->openGLWidget, SLOT(pauseSim()));
    connect(ui->btnReset,   SIGNAL(clicked()), ui->openGLWidget, SLOT(resetSim()));
    connect(ui->timestep,   SIGNAL(valueChanged(double)), ui->openGLWidget, SLOT(setTimeStep(double)));
    ui->openGLWidget->setTimeStep(ui->timestep->value());

    connect(ui->actionCameraReset, SIGNAL(triggered()), ui->openGLWidget, SLOT(resetCamera()));
    connect(ui->actionCameraX, SIGNAL(triggered()), ui->openGLWidget, SLOT(cameraViewX()));
    connect(ui->actionCameraY, SIGNAL(triggered()), ui->openGLWidget, SLOT(cameraViewY()));
    connect(ui->actionCameraZ, SIGNAL(triggered()), ui->openGLWidget, SLOT(cameraViewZ()));

    connect(ui->actionSceneProjectiles,&QAction::triggered, this, [=](void){ changeScene(new SceneProjectiles()); });
    connect(ui->actionSceneFountain,   &QAction::triggered, this, [=](void){ changeScene(new SceneFountain()); });
    connect(ui->actionSceneRope,       &QAction::triggered, this, [=](void){ changeScene(new SceneRope()); });
    connect(ui->actionSceneCloth,      &QAction::triggered, this, [=](void){ changeScene(new SceneCloth()); });
    connect(ui->actionSceneTestIntegrators, &QAction::triggered, this, [=](void){ changeScene(new SceneTestIntegrators()); });
    connect(ui->actionSceneTestColliders,   &QAction::triggered, this, [=](void){ changeScene(new SceneTestColliders()); });
    connect(ui->actionSceneNBody,   &QAction::triggered, this, [=](void){ changeScene(new SceneNBody()); });
    connect(ui->actionFluidSim,   &QAction::triggered, this, [=](void){ changeScene(new SceneFluidSim()); });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeScene(Scene* sc)
{
    // First set scene to glwidget. This will trigger deleting old scene and its associated widget.
    // Otherwise, qDeleteAll deletes the widget first, and then the scene destructor crashes
    ui->openGLWidget->setScene(sc);
    qDeleteAll(ui->controlsScene->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
    ui->controlsScene->layout()->addWidget(sc->sceneUI());
}
