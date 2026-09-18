QT       += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

CONFIG += c++11

INCLUDEPATH += code
INCLUDEPATH += code/scenes
INCLUDEPATH += code/widgets
INCLUDEPATH += extlibs

VPATH += code

SOURCES += \
    code/camera.cpp \
    code/colliders.cpp \
    code/forces.cpp \
    code/glutils.cpp \
    code/glwidget.cpp \
    code/integrators.cpp \
    code/main.cpp \
    code/mainwindow.cpp \
    code/model.cpp \
    code/particlesystem.cpp \
    code/scenes/scenecloth.cpp \
    code/scenes/scenefountain.cpp \
    code/scenes/sceneprojectiles.cpp \
    code/scenes/scenerope.cpp \
    code/scenes/scenetestcolliders.cpp \
    code/scenes/scenetestintegrators.cpp \
    code/scenes/scenenbody.cpp \
    code/scenes/scenefluidsim.cpp \
    code/widgets/widgetcloth.cpp \
    code/widgets/widgetfountain.cpp \
    code/widgets/widgetprojectiles.cpp \
    code/widgets/widgetrope.cpp \
    code/widgets/widgettestcolliders.cpp \
    code/widgets/widgettestintegrators.cpp \
    code/widgets/widgetnbody.cpp \
    code/widgets/widgetfluidsim.cpp \

HEADERS += \
    code/camera.h \
    code/colliders.h \
    code/defines.h \
    code/forces.h \
    code/glutils.h \
    code/glwidget.h \
    code/integrators.h \
    code/mainwindow.h \
    code/model.h \
    code/particle.h \
    code/particlesystem.h \
    code/scene.h \
    code/scenes/scenecloth.h \
    code/scenes/scenenbody.h \
    code/scenes/scenefountain.h \
    code/scenes/sceneprojectiles.h \
    code/scenes/scenerope.h \
    code/scenes/scenetestcolliders.h \
    code/scenes/scenetestintegrators.h \
    code/scenes/scenefluidsim.h \
    code/widgets/widgetcloth.h \
    code/widgets/widgetfountain.h \
    code/widgets/widgetprojectiles.h \
    code/widgets/widgetrope.h \
    code/widgets/widgettestcolliders.h \
    code/widgets/widgettestintegrators.h \
    code/widgets/widgetnbody.h \
    code/widgets/widgetfluidsim.h \

FORMS += \
    forms/mainwindow.ui \
    forms/widgetcloth.ui \
    forms/widgetfountain.ui \
    forms/widgetprojectiles.ui \
    forms/widgetrope.ui \
    forms/widgetnbody.ui \
    forms/widgettestcolliders.ui \
    forms/widgettestintegrators.ui \
    forms/widgetfluidsim.ui \

RESOURCES += shaders.qrc
