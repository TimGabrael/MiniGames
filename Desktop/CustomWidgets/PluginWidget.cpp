#include "PluginWidget.h"
#include "Plugins/PluginCommon.h"
#include <iostream>
#include <qevent.h>
#include <qtimer.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qnamespace.h>
#include "../util/QImGui.h"
#include <imgui.h>
#include "../MiniGames.h"

PluginWidget::PluginWidget(QWidget* parent, PluginClass* plClass) : QOpenGLWidget(parent), plugin(plClass)
{
	QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
	fmt.setSwapInterval(0);
	this->setFormat(fmt);
	setMouseTracking(true);
	this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);


	QObject::connect(&frameTimer, SIGNAL(timeout()), this, SLOT(update()));
	frameTimer.start(0);
}

PluginWidget::~PluginWidget()
{
	
}



#ifdef _WIN32
#define _CURRENT_PLATFORM_ID PLATFORM_ID::PLATFORM_ID_WINDOWS
#elif defined(OSX)
#define _CURRENT_PLATFORM_ID PLATFORM_ID::PLATFORM_ID_OSX
#else
#define _CURRENT_PLATFORM_ID PLATFORM_ID::PLATFORM_ID_LINUX
#endif



void PluginWidget::initializeGL()
{

	initializeOpenGLFunctions();
	QImGui::initialize(this);
	MainApplication* app = MainApplication::GetInstance();
	

	app->appData.imGuiCtx = ImGui::GetCurrentContext();
	app->appData._sendDataFunction = nullptr;
	app->appData.platform = _CURRENT_PLATFORM_ID;
	plugin->Init(&app->appData);
	isInitialized = true;
	
	if (app->appData.localPlayer.isAdmin)
	{

	}
	else
	{

	}
}

void PluginWidget::resizeGL(int w, int h)
{
	plugin->sizeX = w; plugin->sizeY = h;
	plugin->framebufferX = w; plugin->framebufferY = h;

	MainApplication* app = MainApplication::GetInstance();
	plugin->Resize(&app->appData);
}

void PluginWidget::paintGL()
{
	QImGui::newFrame();

	if(hovered) hovered = underMouse();
	
	MainApplication* app = MainApplication::GetInstance();
	plugin->Render(&app->appData);


	ImGui::Render();
	QImGui::render();
}

void PluginWidget::resizeEvent(QResizeEvent* event)
{
	QOpenGLWidget::resizeEvent(event);
	this->resizeGL(event->size().width(), event->size().height());
}

void PluginWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint pos = event->pos();
	MainApplication* app = MainApplication::GetInstance();
	if (centerMouse && app->mainWindow->isActiveWindow())
	{
		//QCursor cursor(Qt::BlankCursor);
		//QApplication::setOverrideCursor(cursor);
		//QApplication::changeOverrideCursor(cursor);

		memset(&mouseData.lPressed, 0, 6);
		QPoint glob = mapToGlobal(QPoint(width() / 2, height() / 2));
		QCursor::setPos(glob);
		mouseData.xPos = width() / 2;
		mouseData.yPos = height() / 2;

		mouseData.dx = mouseData.xPos - pos.x();
		mouseData.dy = mouseData.yPos - pos.y();
		if (mouseData.dx == 0 && mouseData.dy == 0) {
			return;
		}
	}
	else
	{
		memset(&mouseData.lPressed, 0, 6);
		if (!hovered)
		{
			hovered = true;
			mouseData.xPos = pos.x(); mouseData.yPos = pos.y();
			return;
		}
		mouseData.dx = mouseData.xPos - pos.x();
		mouseData.dy = mouseData.yPos - pos.y();

		mouseData.xPos = pos.x(); mouseData.yPos = pos.y();


	}

	plugin->MouseCallback(&this->mouseData);
	QOpenGLWidget::mouseMoveEvent(event);
}
void PluginWidget::mousePressEvent(QMouseEvent* event)
{
	mouseData.dx = 0;mouseData.dy = 0;
	memset(&mouseData.lPressed, 0, 6);
	Qt::MouseButton btn = event->button();
	if (btn == Qt::MouseButton::LeftButton) { mouseData.lPressed = true; mouseData.lDown = true; }
	else if (btn == Qt::MouseButton::RightButton) { mouseData.rPressed = true; mouseData.rDown = true; }
	else if (btn == Qt::MouseButton::MiddleButton) { mouseData.mPressed = true; mouseData.mDown = true; }

	plugin->MouseCallback(&this->mouseData);
}
void PluginWidget::mouseReleaseEvent(QMouseEvent* event)
{
	mouseData.dx = 0; mouseData.dy = 0;
	memset(&mouseData.lPressed, 0, 6);
	Qt::MouseButton btn = event->button();
	if (btn == Qt::MouseButton::LeftButton) { mouseData.lDown = false; mouseData.lUp = true; }
	else if (btn == Qt::MouseButton::RightButton) { mouseData.rDown = false; mouseData.rUp = true; }
	else if (btn == Qt::MouseButton::MiddleButton) { mouseData.mDown = false; mouseData.mUp = true; }

	plugin->MouseCallback(&this->mouseData);
}

void PluginWidget::keyPressEvent(QKeyEvent* event)
{
	plugin->KeyDownCallback((Key)event->key(), event->isAutoRepeat());
}
void PluginWidget::keyReleaseEvent(QKeyEvent* event)
{
	plugin->KeyUpCallback((Key)event->key(), event->isAutoRepeat());
}