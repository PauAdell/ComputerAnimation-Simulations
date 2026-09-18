#ifndef WIDGETNBODY_H
#define WIDGETNBODY_H

#include <QWidget>

namespace Ui {
class WidgetNBody;
}

class WidgetNBody : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetNBody(QWidget *parent = nullptr);
    ~WidgetNBody();

    int    getIntegratorType() const;
    int    getBodiesLayout()   const;
    int    getNumBodies()      const;
    double getMassRange()      const;
    double getSmoothingA()     const;
    double getSmoothingB()     const;
    bool   drawTrajectories()  const;

signals:
    void drawTrajectoriesChanged();

private:
    Ui::WidgetNBody *ui;
};

#endif // WIDGETNBODY_H
