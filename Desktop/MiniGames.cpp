#include "MiniGames.h"

#include "Network/Networking.h"
#include "NFDriver/NFDriver.h"
#undef min
#undef max
#include <algorithm>

#ifdef _WIN32
#include "DarkModeUtil.h"
#include <Windows.h>
#include <iostream>
#endif
#include <qapplication.h>
#include <qpushbutton.h>
#include <qpropertyanimation.h>
#include <qgraphicseffect.h>

#include "CustomWidgets/CustomButton.h"
#include "Frames/MainMenuFrame.h"
#include "Frames/LobbyFrame.h"
#include "Frames/PluginFrame.h"
#include "Frames/SettingsFrame.h"

#include "CommonCollection.h"
#include "Application.h"
#include "util/PluginLoader.h"
#include "Frames/PluginFrame.h"

#include "Audio/AudioBuffer.h"
#include "Audio/WavFile.h"

#include "Network/Messages/join.pb.h"
#include "Network/Messages/sync.pb.h"
#include <qwindow.h>
#include <qtimer.h>
#include <qthread.h>
#include "UtilFuncs.h"
#include "CustomWidgets/InfoPopup.h"
#include "util/PluginLoader.h"


AudioBuffer<400000> testAudioBuffer;




void StutterCallback(void* userData)
{
}
int RenderCallback(void* userData, float* frames, int numberOfFrames)
{
    static double curSample = 0;
    static constexpr double SAMPLE_INV = 1.0 / 44100.0;
    for (int i = 0; i < numberOfFrames * 2; i+=2)
    {
        // Perfect sine wave
        //frames[i] = sinf(curSample * M_PI * 440 * 2) * 0.1f;
        //frames[i+1] = sinf(curSample * M_PI * 440 * 2) * 0.1f;
        //curSample += SAMPLE_INV;
        //Data curData = testAudioBuffer.GetNext();
        //frames[i] = curData.channel1*1;
        //frames[i+1] = curData.channel2*1;
    }
    return 0;
}
void ErrorCallback(void* userData, const char* errorMessage, int errorCode) 
{
}
void WillRenderCallback(void* userData)
{
}
void DidRenderCallback(void* userData)
{
}


void PacketCB(void* userData, Packet* pack)
{
    TCPSocket* sock = (TCPSocket*)userData;
    MainApplication* app = MainApplication::GetInstance();

    if (pack->header.type == (uint32_t)PacketID::SYNC_RESPONSE)
    {
        Base::SyncResponse res;
        res.ParseFromArray(pack->body.data(), pack->body.size());
        app->appData.roomName = res.serverid();
        if (res.id().size() == 16)
        {
            memcpy(app->appData.localPlayerID, res.id().data(), 16);
        }
        app->appData.players.clear();
        for (int i = 0; i < res.connectedclients_size(); i++)
        {
            const Base::ClientInfo& c = res.connectedclients(i);
            app->appData.players.push_back({ c.name(), c.listengroup() });
        }
        std::vector<std::string> avPlugins;
        for (int i = 0; i < res.availableplugins_size(); i++)
        {
            avPlugins.push_back(res.availableplugins(i));
        }
        LoadAndFilterPlugins(avPlugins);
    }
    else if (pack->header.type == (uint32_t)PacketID::JOIN)
    {
        Base::JoinResponse j;
        j.ParseFromArray(pack->body.data(), pack->body.size());
        if (j.error() == Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_OK)
        {
            app->appData.localPlayer.name = j.info().client().name();
            app->appData.localPlayer.groupMask = j.info().client().listengroup();
            app->appData.roomName = j.info().serverid();
            memcpy(app->appData.localPlayerID, j.id().c_str(), std::min(16, (int)j.id().size()));

            app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);

            sock->SendData(PacketID::SYNC_REQUEST, 0, nullptr);
        }
        else if (j.error() == Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_UNAVAILABLE)
        {
            SafeAsyncUI([](MainWindow* main) {
                QRect rec = main->rect();
                new InfoPopup(main, "ROOM NOT AVAILABLE", QPoint(rec.center().x(), rec.height() - 100), 16, 0xFFFF0000, 3000);
            });
        }
        else if (j.error() == Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_INVALID_MESSAGE)
        {
            SafeAsyncUI([](MainWindow* main) {
                QRect rec = main->rect();
                new InfoPopup(main, "INVALID MESSAGE SEND", QPoint(rec.center().x(), rec.height() - 100), 16, 0xFFFF0000, 3000);
            });
        }
    }
    else if (pack->header.type == (uint32_t)PacketID::CREATE)
    {
        MainApplication* app = MainApplication::GetInstance();
        Base::CreateResponse c;
        c.ParseFromArray(pack->body.data(), pack->body.size());
        if (c.error() == Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_OK)
        {
            app->appData.localPlayer.name = c.info().client().name();
            app->appData.localPlayer.groupMask = c.info().client().listengroup();
            app->appData.roomName = c.info().serverid();
            memcpy(app->appData.localPlayerID, c.id().c_str(), std::min(16, (int)c.id().size()));
            
            app->mainWindow->SetState(MAIN_WINDOW_STATE::STATE_LOBBY);
        }
        else if(c.error() == Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_COLLISION)
        {
            SafeAsyncUI([](MainWindow* main) {
                QRect rec = main->rect();
                new InfoPopup(main, "ROOM NAME ALREADY EXISTS", QPoint(rec.center().x(), rec.height() - 100), 16, 0xFFFF0000, 3000);
            });
        }
        else if (c.error() == Base::SERVER_ROOM_CREATE_INFO::ROOM_CREATE_INVALID_MESSAGE)
        {
            SafeAsyncUI([](MainWindow* main) {
                QRect rec = main->rect();
                new InfoPopup(main, "INVALID MESSAGE SEND", QPoint(rec.center().x(), rec.height() - 100), 16, 0xFFFF0000, 3000);
            });
        }
    }

}



static std::future<void> asyncConnectFunc;
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
#ifdef _WIN32
    SetDarkMode(this->winId());
#endif
    QPalette pal(0xD0D0D0, 0x404040, 0x606060, 0x202020, 0x505050, 0xD0D0D0, 0x202020);
    this->setPalette(pal);
    MainApplication* app = MainApplication::GetInstance();
    app->mainWindow = this;

    audioDriver = nativeformat::driver::NFDriver::createNFDriver(nullptr, StutterCallback, RenderCallback, ErrorCallback,
        WillRenderCallback, DidRenderCallback, nativeformat::driver::OutputType::OutputTypeSoundCard);

    this->state = MAIN_WINDOW_STATE::STATE_INVALID;
    //audioDriver->setPlaying(true);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    QFont font;
    font.setPointSize(14);
    this->setFont(font);
    this->setSizePolicy(sizePolicy);
    this->resize(1200, 800);

    app->isConnected = false;

    asyncConnectFunc = std::async([]()
    {
        MainApplication* app = MainApplication::GetInstance();
        app->socket.SetPacketCallback(PacketCB, (void*)&app->socket);
        NetError err = app->socket.Connect(DEBUG_IP, DEBUG_PORT);
        app->isConnected = (err == NetError::OK) ? true : false;
    });

    LoadAllPlugins();

    auto& pl = GetPlugins();
    if (!pl.empty() && false) {
        PluginFrame::activePlugin = pl.at(0);
        PluginFrame* f = new PluginFrame(this);
    }
    else
    {
        SetState(MAIN_WINDOW_STATE::STATE_MENU);
    }
}

MainWindow::~MainWindow()
{
    auto app = MainApplication::GetInstance();
    app->socket.Disconnect();
    
}



void MainWindow::SetState(MAIN_WINDOW_STATE s)
{
    if (this->state == s) return;
    state = s;
    if (this->thread() == QThread::currentThread())
    {
        InternalSetState(s);
    }
    else
    {
        QTimer::singleShot(0, this, [this]() { InternalSetState(this->state); });
    }
    if (stackPtr < 16)
    {
        stateStack[stackPtr] = s;
        stackPtr++;
    }
    else
    {
        stackPtr = 16;
        for (uint8_t i = 0; i < 15; i++) stateStack[i] = stateStack[i+1];
        stateStack[stackPtr - 1] = s;
    }

}
void MainWindow::SetPreviousState()
{
    if (stackPtr > 1)
    {
        stackPtr--;
        stateStack[stackPtr] = stateStack[stackPtr-1];
        state = stateStack[stackPtr];
        InternalSetState(state);
    }
}
MAIN_WINDOW_STATE MainWindow::GetState()
{
    return state;
}

void MainWindow::InternalSetState(MAIN_WINDOW_STATE state)
{
    this->state = state;
    switch (state)
    {
    case MAIN_WINDOW_STATE::STATE_MENU:
        new MainMenuFrame(this);
        break;
    case MAIN_WINDOW_STATE::STATE_LOBBY:
        new LobbyFrame(this);
        break;
    case MAIN_WINDOW_STATE::STATE_SETTINGS:
        new SettingsFrame(this);
        break;
    case MAIN_WINDOW_STATE::STATE_PLUGIN:
        new PluginFrame(this);
        break;


    case MAIN_WINDOW_STATE::STATE_INVALID:
    default:
        break;
    }
}




#ifndef _WIN32
int main(int argc, char* argv[]) {
    MainApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
#else
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){

    FILE* f = NULL;
    AllocConsole();
    freopen_s(&f, "CONOUT$", "w", stdout);

    int argc = 0;
    char* argv[1] = { 0 };
    MainApplication app(argc, argv);
    MainWindow w;
    w.show();

    return app.exec();

}
#endif