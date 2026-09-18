#ifndef WIDGETFLUIDSIM_H
#define WIDGETFLUIDSIM_H

#include <QWidget>

namespace Ui {
class WidgetFluidSim;
}

class WidgetFluidSim : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetFluidSim(QWidget *parent = nullptr);
    ~WidgetFluidSim();

    // Params pulled by the scene
    double getGravity()   const;

signals:
    void updatedParameters();
    void spawnClicked();
    void autoEmitToggled(bool on);
    void spawnCountChanged(int n);
    void maxParticlesChanged(int n);

private:
    Ui::WidgetFluidSim *ui;
};

#endif // WIDGETFLUIDSIM_H
