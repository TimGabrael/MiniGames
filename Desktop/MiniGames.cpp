#include "MiniGames.h"

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
#include "CommonCollection.h"
#include "Application.h"
#include "util/PluginLoader.h"
#include "Frames/PluginFrame.h"

#include "Audio/AudioBuffer.h"
#include "Audio/WavFile.h"

#include "Network/Messages/join.pb.h"


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

    if (pack->header.type == (uint32_t)PacketID::SYNC_RESPONSE)
    {
        
        
    }
    else if (pack->header.type == (uint32_t)PacketID::JOIN)
    {
        Base::JoinResponse j;
        if (j.error() == Base::SERVER_ROOM_JOIN_INFO::ROOM_JOIN_OK)
        {
            j.ParseFromArray(pack->body.data(), pack->body.size());
            j.info().client().name();

            LOG("GOT A JOIN MESSAGE\n");
        }
    }

    // LOG("GOT A MESSAGE FROM THE SERVER\n");

}




MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
#ifdef _WIN32
    SetDarkMode(this->winId());
#endif
    QPalette pal(0xD0D0D0, 0x404040, 0x606060, 0x202020, 0x505050, 0xD0D0D0, 0x202020);
    this->setPalette(pal);

    audioDriver = nativeformat::driver::NFDriver::createNFDriver(nullptr, StutterCallback, RenderCallback, ErrorCallback,
        WillRenderCallback, DidRenderCallback, nativeformat::driver::OutputType::OutputTypeSoundCard);

    
    //audioDriver->setPlaying(true);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    QFont font;
    font.setPointSize(14);
    this->setFont(font);
    this->setSizePolicy(sizePolicy);
    this->resize(1200, 800);

    auto app = MainApplication::GetInstance();
    NetError err = app->socket.Connect(DEBUG_IP, DEBUG_PORT);
    app->isConnected = (err == NetError::OK) ? true : false;
    if (err == NetError::OK) {
        app->socket.SetPacketCallback(PacketCB, (void*)&app->socket);

        
        Base::JoinRequest req;
        req.mutable_info()->mutable_client()->set_name("Test Name");
        req.mutable_info()->set_serverid("Test Server");
        req.mutable_info()->add_availableplugins("Test Plugin");
        

        std::string serialized = req.SerializeAsString();
        app->socket.SendData(PacketID::JOIN, serialized.size(), (const uint8_t*)serialized.data());
    }

    LoadAllPlugins();

    auto& pl = GetPlugins();
    if (!pl.empty() && false) {
        PluginFrame* f = new PluginFrame(this, pl.at(0));
    }
    else
    {
        MainMenuFrame* f = new MainMenuFrame(this);
    }
}

MainWindow::~MainWindow()
{
    auto app = MainApplication::GetInstance();
    app->socket.Disconnect();
    
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