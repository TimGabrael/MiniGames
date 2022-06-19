#include "PluginWidget.h"
#include <iostream>
#include <qevent.h>
#include <qtimer.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>

PluginWidget::PluginWidget(QWidget* parent, PluginClass* plClass) : QOpenGLWidget(parent), plugin(plClass)
{
	QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
	fmt.setSwapInterval(0);
	this->setFormat(fmt);
	
	connect(this, &QOpenGLWidget::frameSwapped, this, &PluginWidget::HandleTimer);
}

PluginWidget::~PluginWidget()
{
}

void PluginWidget::initializeGL()
{
	plugin->Init(nullptr);

	isInitialized = true;
}

void PluginWidget::resizeGL(int w, int h)
{
	plugin->sizeX = w; plugin->sizeY = h;
	plugin->framebufferX = w; plugin->framebufferY = h;
	
	plugin->Resize(nullptr);
}

void PluginWidget::paintGL()
{
	plugin->Render(nullptr);
}

void PluginWidget::resizeEvent(QResizeEvent* event)
{
	QOpenGLWidget::resizeEvent(event);
	this->resizeGL(event->size().width(), event->size().height());
}

void PluginWidget::HandleTimer()
{
	//static auto prev = std::chrono::high_resolution_clock::now();
	//auto now = std::chrono::high_resolution_clock::now();
	//std::cout << "frameTime: " << std::chrono::duration<float>(now - prev).count() << std::endl;
	//prev = now;
	this->update();
}