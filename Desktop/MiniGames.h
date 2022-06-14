#pragma once

#include <QMainWindow>
#include "Application.h"
#include "Network/Networking.h"



class MainWindow : public QMainWindow
{

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

};