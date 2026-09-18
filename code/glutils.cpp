#include "glutils.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include "model.h"


GLenum glutils::printGLError(const char file[], int line, const char func[])
{
    QOpenGLFunctions* funcs = QOpenGLContext::currentContext()->functions();
    GLenum glErr = funcs->glGetError();
    std::string error;
    switch (glErr) {
        case 0x0500:
            error = "GL_INVALID_ENUM";
            break;
        case 0x501:
            error = "GL_INVALID_VALUE";
            break;
        case 0x502:
            error = "GL_INVALID_OPERATION";
            break;
        case 0x503:
            error = "GL_STACK_OVERFLOW";
            break;
        case 0x504:
            error = "GL_STACK_UNDERFLOW";
            break;
        case 0x505:
            error = "GL_OUT_OF_MEMORY";
            break;
        default:
            error = "unknown error!";
    }
    if (glErr != GL_NO_ERROR) {
        std::cerr << "glError in file " << file << " @ line " << line
                  << " (" << func << "): " << error << std::endl;
    }
    return glErr;
}


QOpenGLShaderProgram* glutils::loadShaderProgram(const QString& vsPath, const QString& fsPath, QObject* parent)
{
    QOpenGLShader vs (QOpenGLShader::Vertex, parent);
    QOpenGLShader fs (QOpenGLShader::Fragment, parent);
    vs.compileSourceFile(vsPath);
    fs.compileSourceFile(fsPath);

    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(parent);
    program->addShader(&vs);
    program->addShader(&fs);
    program->link();
    return program;
}

QOpenGLShaderProgram* glutils::loadShaderProgram(const QString& vsPath, const QString& gsPath,
                                        const QString& fsPath, QObject* parent)
{
    QOpenGLShader vs (QOpenGLShader::Vertex, parent);
    QOpenGLShader gs (QOpenGLShader::Geometry, parent);
    QOpenGLShader fs (QOpenGLShader::Fragment, parent);
    vs.compileSourceFile(vsPath);
    gs.compileSourceFile(gsPath);
    fs.compileSourceFile(fsPath);

    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(parent);
    program->addShader(&vs);
    program->addShader(&gs);
    program->addShader(&fs);
    program->link();
    return program;
}

QOpenGLVertexArrayObject* glutils::createVAO(QOpenGLShaderProgram* program, Model* model, std::vector<QOpenGLBuffer*>& buffers, QObject* parent)
{
    QOpenGLVertexArrayObject* vao = new QOpenGLVertexArrayObject(parent);
    vao->create();
    vao->bind();

    QOpenGLBuffer* vboVerts = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    buffers.push_back(vboVerts);
    vboVerts->create();
    vboVerts->bind();
    vboVerts->setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);
    vboVerts->allocate(model->getCoordsPtr(), model->numVertices()*3*sizeof(float));
    program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 0);
    program->enableAttributeArray("vertex");

    QOpenGLBuffer* vboNormals = new QOpenGLBuffer(QOpenGLBuffer::Type::VertexBuffer);
    buffers.push_back(vboNormals);
    vboNormals->create();
    vboNormals->bind();
    vboNormals->setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);
    vboNormals->allocate(model->getNormalsPtr(), model->numVertices()*3*sizeof(float));
    program->setAttributeBuffer("normal", GL_FLOAT, 0, 3, 0);
    program->enableAttributeArray("normal");

    QOpenGLBuffer* vboIndices = new QOpenGLBuffer(QOpenGLBuffer::Type::IndexBuffer);
    buffers.push_back(vboIndices);
    vboIndices->create();
    vboIndices->bind();
    vboIndices->setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);
    vboIndices->allocate(model->getIndicesPtr(), int(model->getIndices().size())*sizeof(unsigned int));

    vao->release();

    return vao;
}
