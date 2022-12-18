#include "MiniGames.h"

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


    LoadAllPlugins();

    auto& pl = GetPlugins();
    //if (!pl.empty()) {
    //    PluginFrame::activePlugin = pl.at(0);
    //    SetState(MAIN_WINDOW_STATE::STATE_PLUGIN);
    //}
    //else
    {
        SetState(MAIN_WINDOW_STATE::STATE_MENU);  
    }
}

MainWindow::~MainWindow()
{
    
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

    //FILE* f = NULL;
    //AllocConsole();
    //freopen_s(&f, "CONOUT$", "w", stdout);

    int argc = 0;
    char* argv[1] = { 0 };
    MainApplication app(argc, argv);
    MainWindow w;
    w.show();

    return app.exec();

}
#endif