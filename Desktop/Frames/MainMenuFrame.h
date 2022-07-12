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

    virtual void FetchSyncData(std::string& str) override;
    virtual void HandleAddClient(const ClientData* added) override;
    virtual void HandleRemovedClient(const ClientData* removed) override;

    virtual void HandleNetworkMessage(Packet* packet) override;
    virtual void HandleSync(const std::string& syncData) override;

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