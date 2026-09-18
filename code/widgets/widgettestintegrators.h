#ifndef WIDGETTESTINTEGRATORS_H
#define WIDGETTESTINTEGRATORS_H

#include <QWidget>
#include "defines.h"

namespace Ui {
class WidgetTestIntegrators;
}

class WidgetTestIntegrators : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetTestIntegrators(QWidget *parent = nullptr);
    ~WidgetTestIntegrators();

    int getIntegratorType() const;
    double getStiffness() const;
    double getDamping() const;
    double getForceMagnitude() const;
    double getForceFrequency() const;
    double getInitialPos() const;
    double getInitialVel() const;
    bool drawPhaseAnalytic() const;
    bool drawPhaseNumerical() const;
    bool drawOscillationAnalytic() const;
    bool drawOscillationNumerical() const;
    bool drawScaleVectors() const;
    double getTimeScale() const;

    void setInfo(const QString& txt);

protected slots:
    void changedParameter();
    void setCriticalDamp();

signals:
    void updatedParameters();

private:
    Ui::WidgetTestIntegrators *ui;

};

#endif // WIDGETTESTINTEGRATORS_H
