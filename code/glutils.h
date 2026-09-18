#ifndef GLUTILS_H
#define GLUTILS_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <iostream>

class Model;

namespace glutils {
    #define checkGLError() printGLError(__FILE__, __LINE__,__FUNCTION__)
    GLenum printGLError(const char file[], int line, const char func[]);

    QOpenGLShaderProgram* loadShaderProgram(const QString& vsPath, const QString& fsPath, QObject* parent=nullptr);
    QOpenGLShaderProgram* loadShaderProgram(const QString& vsPath, const QString& gsPath,
                                            const QString& fsPath, QObject* parent=nullptr);

    QOpenGLVertexArrayObject* createVAO(QOpenGLShaderProgram* program, Model* model, std::vector<QOpenGLBuffer*>& buffers, QObject* parent=nullptr);
}

#endif // GLUTILS_H
