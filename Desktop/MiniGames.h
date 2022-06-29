#pragma once

#include <QMainWindow>
#include "Application.h"
#include "Network/Networking.h"
#include "NFDriver/NFDriver.h"


class MainWindow : public QMainWindow
{

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();


    nativeformat::driver::NFDriver* audioDriver;
};