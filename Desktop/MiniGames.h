#pragma once

#include <QMainWindow>
#include "Application.h"



enum class MAIN_WINDOW_STATE
{
    STATE_MENU,
    STATE_LOBBY,
    STATE_SETTINGS,
    STATE_PLUGIN,
    STATE_INVALID,
};

class MainWindow : public QMainWindow
{

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();


    void SetState(MAIN_WINDOW_STATE state);
    void SetPreviousState();
    MAIN_WINDOW_STATE GetState();

    void* stateWidget = nullptr;
private:

    void InternalSetState(MAIN_WINDOW_STATE state);

    static constexpr uint32_t MAX_STACK_SIZE = 16;
    MAIN_WINDOW_STATE state = MAIN_WINDOW_STATE::STATE_INVALID;
    MAIN_WINDOW_STATE stateStack[MAX_STACK_SIZE];
    uint8_t stackPtr;
};
