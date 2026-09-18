#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "defines.h"
#include <QMatrix4x4>

class Camera {

public:
    Camera();
    Camera(const Vec3& eye, double dist, double ax, double ay, double zn=1.0, double zf=1000.0, double fov=60.0);

    // attribute setters/getters
    void setEulerAngles(double ax, double ay) { angleX = ax; angleY = ay; }
    void setPlanes(double n, double f) { znear = n; zfar = f; }
    void setFOV(double f) { fov = f; }
    void setViewport(int w, int h) { vpWidth = w; vpHeight = h; }
    double getFOV() const { return fov; }
    double getAspectRatio() const { return double(vpWidth)/vpHeight; }
    int getWidth() const { return vpWidth; }
    int getHeight() const { return vpHeight; }

    Vec3 getEye() const { return eye; }
    Vec3 getPos() const { return eye + dist*zAxis(); }
    double getEyeDistance() const { return dist; }
    double getAngleX() const { return angleX; }
    double getAngleY() const { return angleY; }
    double getEyeDistance() { return dist; }
    Vec3 xAxis() const;
    Vec3 yAxis() const;
    Vec3 zAxis() const;

    // interaction
    void rotateUpDown(double angle);
    void rotateLeftRight(double angle);
    void move(const Vec3& disp);
    void moveBackForth(double dist, bool moveEye = false);
    void moveUpDown(double dist);
    void moveLeftRight(double dist);

    // camera matrices
    QMatrix4x4 getPerspectiveMatrix() const;
    QMatrix4x4 getViewMatrix() const;

    // ray
    Vec3 getRayDir(int pixelX, int pixelY) const;
    Vec3 worldSpaceDisplacement(int dpixelsX, int dpixelsY, double distPlane = 1) const;

    // creates and returns a camera to view a given box domain
    static Camera CreateFromSceneBounds(const Vec3& bmin, const Vec3& bmax);

protected:
    Vec3   eye;     // VRP (view reference point), where the camera looks at
    double angleX;  // Euler angle X
    double angleY;  // Euler angle Y
    double dist;    // Camera location distance to VRP

    double znear;   // Near plane
    double zfar;    // Far plane
    double fov;     // Focal length

    double vpWidth; // viewport width
    double vpHeight;// viewport height
};

#endif
