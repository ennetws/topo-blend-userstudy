#pragma once

#include "DemoGlobal.h"
#include "Scene.h"
#include "Controls.h"
#include "Matcher.h"
#include "Blender.h"
#include <QElapsedTimer>

class Session;

struct StudyTask{
	QVector<PropertyMap> data;
	QString name;
	QMap<QString, QElapsedTimer> timer;
	Session * session;
	StudyTask(QString name = "", QVector<PropertyMap> data = QVector<PropertyMap>(), Session * session = NULL) : name(name), data(data), session(session){ }
};

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(Scene * scene, Controls * control, Matcher * matcher, Blender * blender, QObject *parent = 0);
    Scene * s;
    Controls * c;
    Matcher * m;
    Blender * b;

	QVector<StudyTask> tasks;
	int curTaskIndex;

signals:
    void update();

	void allTasksDone();

public slots:
    void shapeChanged(int,QGraphicsItem*);

    void hideAll();
    void showSelect();
    void showMatch();
    void showCreate();

	void displayNextTask();
	void emitAllTasksDone();
	void advanceTasks();
};
