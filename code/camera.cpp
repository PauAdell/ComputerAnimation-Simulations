#include "camera.h"
#include <cmath>

Camera::Camera() {
    eye = Vec3(0, 0, 0);
    angleX = 0;
    angleY = 0;
    dist = 1;
    znear = 1.0;
    zfar = 1000.0;
    fov = 60;
    vpWidth = vpHeight = 1;
}

Camera::Camera(const Vec3& eye, double dist, double ax, double ay, double near, double far, double fov) {
    this->eye = eye;
    this->dist = dist;
    this->angleX = ax;
    this->angleY = ay;
    this->znear = near;
    this->zfar = far;
    this->fov = fov;
}


Vec3 Camera::xAxis() const {
    QMatrix4x4 vm = getViewMatrix();
    return Vec3(vm.row(0)[0], vm.row(0)[1], vm.row(0)[2]);
}

Vec3 Camera::yAxis() const {
    QMatrix4x4 vm = getViewMatrix();
    return Vec3(vm.row(1)[0], vm.row(1)[1], vm.row(1)[2]);
}

Vec3 Camera::zAxis() const {
    return Vec3(std::cos(angleX)*std::sin(angleY),
                std::sin(angleX),
                std::cos(angleX)*std::cos(angleY)
                );
}

void Camera::rotateUpDown(double a) {
    angleX += a;
}

void Camera::rotateLeftRight(double a) {
    angleY += a;
}

void Camera::moveBackForth(double d, bool moveEye) {
    if (moveEye) {
        Vec3 z = zAxis();
        eye = eye - d*z;
    }
    else {
        dist -= d;
    }
    znear -= d;
    zfar  -= d;
}

void Camera::move(const Vec3& disp) {
    eye += disp;
}

void Camera::moveUpDown(double d) {
    eye = eye + d * yAxis();
}

void Camera::moveLeftRight(double d){
    eye = eye + d * xAxis();
}

QMatrix4x4 Camera::getPerspectiveMatrix() const {
    QMatrix4x4 mat;
    mat.perspective(fov, float(vpWidth)/float(vpHeight), znear, zfar);
    return mat;
}

QMatrix4x4 Camera::getViewMatrix() const {
    QMatrix4x4 mat;
    mat.translate(0, 0, float(-dist));
    mat.rotate( angleX*180.0/M_PI, QVector3D(1, 0, 0));
    mat.rotate(-angleY*180.0/M_PI, QVector3D(0, 1, 0));
    mat.translate(float(-eye[0]), float(-eye[1]), float(-eye[2]));
    return mat;
}

Vec3 Camera::getRayDir(int pixelX, int pixelY) const {
    double dx = 2*(pixelX + 0.5)/vpWidth - 1;
    double dy = 1 - 2*(pixelY + 0.5)/vpHeight;

    double sy = std::tan(Math::toRad(0.5*fov));
    double sx = getAspectRatio()*sy;
    Vec3 camDir(dx*sx, dy*sy, -1);

    QMatrix4x4 vm = getViewMatrix();
    Vec3 wx(vm.row(0)[0], vm.row(0)[1], vm.row(0)[2]);
    Vec3 wy(vm.row(1)[0], vm.row(1)[1], vm.row(1)[2]);
    Vec3 wz(vm.row(2)[0], vm.row(2)[1], vm.row(2)[2]);

    Vec3 rayDir = camDir.x()*wx + camDir.y()*wy + camDir.z()*wz;
    return rayDir.normalized();
}

Vec3 Camera::worldSpaceDisplacement(int dpixelsX, int dpixelsY, double dist) const {
    QMatrix4x4 vm = getViewMatrix();
    Vec3 wx(vm.row(0)[0], vm.row(0)[1], vm.row(0)[2]);
    Vec3 wy(vm.row(1)[0], vm.row(1)[1], vm.row(1)[2]);
    double sy = dist * std::tan(0.5*Math::toRad(fov));
    double sx = getAspectRatio()*sy;

    Vec3 disp = sx * 2 * dpixelsX/double(vpWidth)  * wx
              + sy * 2 * dpixelsY/double(vpHeight) * wy;
    return disp;
}

Camera Camera::CreateFromSceneBounds(const Vec3& bmin, const Vec3& bmax) {
    Vec3 d = 0.5*(bmax - bmin);
    Vec3 c = 0.5*(bmin + bmax);
    double r = d.norm();
    double f = 2*Math::toDeg(std::asin(0.5));
    return Camera(c, 2*r, 0, 0, r, 3*r, f);
}
