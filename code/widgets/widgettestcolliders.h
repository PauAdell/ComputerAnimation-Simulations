#ifndef WIDGETTESTCOLLIDERS_H
#define WIDGETTESTCOLLIDERS_H

#include <QWidget>
#include "defines.h"

namespace Ui {
class WidgetTestColliders;
}

class WidgetTestColliders : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetTestColliders(QWidget *parent = nullptr);
    ~WidgetTestColliders();


    int    getColliderType() const;
    Vec3   getPlaneNormal() const;
    double getPlaneD() const;
    Vec3   getSphereCenter() const;
    double getSphereRadius() const;
    Vec3   getBoxMin() const;
    Vec3   getBoxMax() const;

    double getRestitution() const;
    double getFriction() const;
    double getParticleRadius() const;
    bool   getContinuousCollision() const;
    Vec3   getPrevPos() const;
    Vec3   getCurrPos() const;

    void setCollisionInfo(const QString& text);

protected slots:
    void changedParameter();

signals:
    void updatedParameters();

private:
    Ui::WidgetTestColliders *ui;

};

#endif // WIDGETTESTCOLLIDERS_H
