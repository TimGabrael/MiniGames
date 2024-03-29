#pragma once
#include <qframe.h>

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "StateFrame.h"

class MainMenuFrame : public StateFrame
{
public:
	MainMenuFrame(QMainWindow* parent);
	~MainMenuFrame();


private:
    QVBoxLayout* verticalLayout;
    QLabel* header;
    QHBoxLayout* horizontalLayout;
    QLabel* nametxt;
    QLineEdit* nameIn;
    QHBoxLayout* horizontalLayout_1;
    QLabel* lobbytxt;
    QLineEdit* lobbyIn;
    QLabel* errortxt;
    
    QHBoxLayout* horizontalLayout_2;
    class CustomButton* createBtn;
    class CustomButton* joinBtn;
    class CustomButton* settingsBtn;
    class CustomButton* exitBtn;


    void OnJoinClick();
    void OnCreateClick();
    void OnSettingsClick();
    void OnExitClick();


};