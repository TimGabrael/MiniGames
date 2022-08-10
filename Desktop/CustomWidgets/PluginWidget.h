#pragma once
#include <QWidget.h>
#include <qopenglwidget.h>
#include <QOpenGLExtraFunctions>
#include <qtimer.h>
#include "../util/PluginLoader.h"
#include <qevent.h>

class PluginWidget : public QOpenGLWidget, private QOpenGLExtraFunctions
{
public:
	PluginWidget(QWidget* parent, PluginClass* plClass);
	~PluginWidget();



private:

	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void paintGL() override;

	void resizeEvent(QResizeEvent* event) override;


	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual void keyReleaseEvent(QKeyEvent* event) override;


	PB_MouseData mouseData;
	PluginClass* plugin = nullptr;
	QTimer frameTimer;
	bool isInitialized = false;
	bool hovered = false;
};