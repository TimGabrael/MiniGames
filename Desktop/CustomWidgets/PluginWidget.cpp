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
	setMouseTracking(true);
	
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
	if(hovered) hovered = underMouse();
	plugin->Render(nullptr);
}

void PluginWidget::resizeEvent(QResizeEvent* event)
{
	QOpenGLWidget::resizeEvent(event);
	this->resizeGL(event->size().width(), event->size().height());
}

void PluginWidget::HandleTimer()
{
	//hovered = underMouse();
	//static auto prev = std::chrono::high_resolution_clock::now();
	//auto now = std::chrono::high_resolution_clock::now();
	//std::cout << "frameTime: " << std::chrono::duration<float>(now - prev).count() << std::endl;
	//prev = now;
	this->update();
}

void PluginWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint pos = event->pos();
	memset(&mouseData.lDown, 0, 6);
	if (!hovered)
	{
		hovered = true;
		mouseData.xPos = pos.x(); mouseData.yPos = pos.y();
		return;
	}
	mouseData.dx = mouseData.xPos - pos.x();
	mouseData.dy = mouseData.yPos - pos.y();

	mouseData.xPos = pos.x(); mouseData.yPos = pos.y();

	plugin->MouseCallback(&this->mouseData);
}
void PluginWidget::mousePressEvent(QMouseEvent* event)
{
	mouseData.dx = 0;mouseData.dy = 0;
	memset(&mouseData.lDown, 0, 6);
	Qt::MouseButton btn = event->button();
	if (btn == Qt::MouseButton::LeftButton) { mouseData.lPressed = true; mouseData.lDown = true; }
	else if (btn == Qt::MouseButton::RightButton) { mouseData.rPressed = true; mouseData.rDown = true; }
	else if (btn == Qt::MouseButton::MiddleButton) { mouseData.mPressed = true; mouseData.mDown = true; }

	plugin->MouseCallback(&this->mouseData);
}
void PluginWidget::mouseReleaseEvent(QMouseEvent* event)
{
	mouseData.dx = 0; mouseData.dy = 0;
	memset(&mouseData.lDown, 0, 6);
	Qt::MouseButton btn = event->button();
	if (btn == Qt::MouseButton::LeftButton) { mouseData.lPressed = false; mouseData.lUp = true; }
	else if (btn == Qt::MouseButton::RightButton) { mouseData.rPressed = false; mouseData.rUp = true; }
	else if (btn == Qt::MouseButton::MiddleButton) { mouseData.mPressed = false; mouseData.mUp = true; }

	plugin->MouseCallback(&this->mouseData);
}