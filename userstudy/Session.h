#pragma once

#include "DemoGlobal.h"
#include "Scene.h"
#include "Controls.h"
#include "Matcher.h"
#include "Blender.h"

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(Scene * scene, Controls * control, Matcher * matcher, Blender * blender, QObject *parent = 0);
    Scene * s;
    Controls * c;
    Matcher * m;
    Blender * b;

signals:
    void update();

public slots:
    void shapeChanged(int,QGraphicsItem*);

    void hideAll();
    void showSelect();
    void showMatch();
    void showCreate();
};
