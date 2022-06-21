#pragma once
#include <QWidget.h>
#include <qopenglwidget.h>
#include "../util/PluginLoader.h"


class PluginWidget : public QOpenGLWidget
{
public:
	PluginWidget(QWidget* parent, PluginClass* plClass);
	~PluginWidget();


private:

	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void paintGL() override;

	void resizeEvent(QResizeEvent* event) override;

	void HandleTimer();


	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;


	PB_MouseData mouseData;
	PluginClass* plugin = nullptr;
	bool isInitialized = false;
	bool hovered = false;
};