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
#include "Network/Messages/ClientInfo.pb.h"
#include "Network/Messages/lobby.pb.h"

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



static void SendDataFunction(uint32_t packetID, uint32_t group, uint16_t flags, uint16_t clientID, size_t size, const void* data)
{
	MainApplication* app = MainApplication::GetInstance();
	app->socket.SendData((PacketID)packetID, group, flags, clientID, size, data);
}

void PluginWidget::initializeGL()
{

	initializeOpenGLFunctions();
	QImGui::initialize(this);
	MainApplication* app = MainApplication::GetInstance();
	
	Base::ClientInfo client;
	client.set_id(app->appData.localPlayer.clientID);
	client.set_listengroup(app->appData.localPlayer.groupMask);
	client.set_name(app->appData.localPlayer.name);

	app->appData.imGuiCtx = ImGui::GetCurrentContext();
	app->appData._sendDataFunction = &SendDataFunction;
	app->appData.platform = _CURRENT_PLATFORM_ID;
	plugin->Init(&app->appData);
	isInitialized = true;
	
	if (app->appData.localPlayer.groupMask & ADMIN_GROUP_MASK)
	{
		Base::StartPlugin starting;
		std::string start = std::string(plugin->GetPluginInfos().ID, 19);
		starting.set_pluginid(start);
		app->socket.SendData(PacketID::START, LISTEN_GROUP_ALL, ADDITIONAL_DATA_FLAG_ADMIN, app->appData.localPlayer.clientID, starting.SerializeAsString());
	}
	else
	{
		app->socket.SendData(PacketID::STARTED, LISTEN_GROUP_ALL, 0, app->appData.localPlayer.clientID, client.SerializeAsString());
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

	plugin->MouseCallback(&this->mouseData);
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