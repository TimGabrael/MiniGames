#include "MainMenuFrame.h"
#include "../CustomWidgets/CustomButton.h"
#include "../CustomWidgets/CustomTextInput.h"
#include "SettingsFrame.h"
#include "LobbyFrame.h"
#include "CommonCollection.h"
#include "../Application.h"
#include "qmessagebox.h"
#include "qtooltip.h"
#include "../CustomWidgets/InfoPopup.h"
#include <future>
#include "../UtilFuncs.h"
#include "Network/Messages/join.pb.h"
#include "Validation.h"
#include "../util/PluginLoader.h"

MainMenuFrame::MainMenuFrame(QMainWindow* MainWindow) : StateFrame(MainWindow)
{
    verticalLayout = new QVBoxLayout(this);
    {
        verticalLayout->setSpacing(4);
        verticalLayout->addStretch(4);

        header = new QLabel("Main Menu", this);
        QFont font1;
        font1.setPointSize(40);
        font1.setBold(true);
        font1.setUnderline(true);
        header->setFont(font1);
        verticalLayout->addWidget(header, 0, Qt::AlignHCenter);


        verticalLayout->addStretch(2);

        QFont font2;
        font2.setPointSize(14);
        font2.setUnderline(true);
        horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addStretch(10);
        {
            nametxt = new QLabel("Username:", this);
            nametxt->setFont(font2);
            nametxt->setIndent(18);
            horizontalLayout->addWidget(nametxt, 0, Qt::AlignRight);

            nameIn = new CustomTextInput(this);
            nameIn->setMinimumSize(QSize(200, 0));
            nameIn->setPlaceholderText("Name");
            horizontalLayout->addWidget(nameIn, 0, Qt::AlignRight);
            horizontalLayout->addStretch(10);
        }
        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_1 = new QHBoxLayout(this);
        horizontalLayout_1->addStretch(10);
        {
            lobbytxt = new QLabel("Lobby Name:", this);
            lobbytxt->setFont(font2);

            horizontalLayout_1->addWidget(lobbytxt, 0, Qt::AlignLeft);

            lobbyIn = new CustomTextInput(this);
            lobbyIn->setMinimumSize(QSize(200, 0));
            lobbyIn->setPlaceholderText("LobbyID");
            horizontalLayout_1->addWidget(lobbyIn, 0, Qt::AlignRight);
            horizontalLayout_1->addStretch(10);
        }
        verticalLayout->addLayout(horizontalLayout_1);
        {
            errortxt = new QLabel("INVALID\nSECONDLINE", this);
            auto szPol = errortxt->sizePolicy();
            szPol.setRetainSizeWhenHidden(true);
            //errortxt->setSizePolicy(szPol);
            QPalette pal = errortxt->palette();
            pal.setColor(QPalette::ColorRole::WindowText, 0xFFFF0000);
            auto errFont = errortxt->font(); errFont.setUnderline(true);
            errortxt->setFont(errFont);
            errortxt->setPalette(pal);
            verticalLayout->addWidget(errortxt, 0, Qt::AlignCenter);
            errortxt->hide();
        }
        verticalLayout->addSpacing(10);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->addStretch(10);
        horizontalLayout_2->setSpacing(10);
        {
            joinBtn = new CustomButton("Join", this);
            joinBtn->setMinimumSize(120, 32);
            
            horizontalLayout_2->addWidget(joinBtn);
            
            createBtn = new CustomButton("Create", this);
            createBtn->setMinimumSize(120, 32);
            horizontalLayout_2->addWidget(createBtn);

            horizontalLayout_2->addStretch(10);

        }
        verticalLayout->addLayout(horizontalLayout_2);

        verticalLayout->addSpacing(20);
        settingsBtn = new CustomButton("Settings", this);
        settingsBtn->setMinimumSize(260, 32);
        settingsBtn->setMaximumSize(260, 32);
        
        verticalLayout->addWidget(settingsBtn, 0, Qt::AlignmentFlag::AlignHCenter);

        verticalLayout->addStretch(1);

        exitBtn = new CustomButton("Exit", this);
        exitBtn->setMinimumSize(260, 32);
        exitBtn->setMaximumSize(260, 32);
        exitBtn->SetBackgroundColor(0xA02020);
        exitBtn->SetBackgroundHighlightColor(0xD04040);

        verticalLayout->addWidget(exitBtn, 0, Qt::AlignmentFlag::AlignHCenter);

        verticalLayout->addStretch(6);
    }


    connect(joinBtn, &QPushButton::clicked, this, &MainMenuFrame::OnJoinClick);
    connect(createBtn, &QPushButton::clicked, this, &MainMenuFrame::OnCreateClick);
    connect(settingsBtn, &QPushButton::clicked, this, &MainMenuFrame::OnSettingsClick);
    connect(exitBtn, &QPushButton::clicked, this, &MainMenuFrame::OnExitClick);

    auto function = []() {
        MainApplication* app = MainApplication::GetInstance();
        NetError err = app->socket.Connect(DEBUG_IP, DEBUG_PORT);
        app->isConnected = (err == NetError::OK) ? true : false;
        if (!app->isConnected)
        {
            // POP UP MESSAGE FAILED TO CONNECT TO SERVER
            //LOG("COULD NOT ESTABLISH A CONNECTION TO THE SERVER: %p\n", wnd);
            //auto rect = wnd->geometry();
            //InfoPopup* popUp = new InfoPopup(wnd, "TestContent", QPoint(rect.width() / 2, rect.height() - 100), 40, 0xFFFFFFFF, 100000);
            //LOG("HIERBINICHSF\n");
        }
    };

    MainWindow->setCentralWidget(this);
}
MainMenuFrame::~MainMenuFrame()
{
}

void MainMenuFrame::FetchSyncData(std::string& str)
{
}

void MainMenuFrame::HandleAddClient(const ClientData* added)
{
}

void MainMenuFrame::HandleRemovedClient(const ClientData* removed)
{
}

void MainMenuFrame::HandleNetworkMessage(Packet* packet)
{
}

void MainMenuFrame::HandleSync(const std::string& syncData)
{
}


void MainMenuFrame::OnJoinClick()
{
    std::string name = this->nameIn->text().toStdString();
    std::string server = this->lobbyIn->text().toStdString();

    bool canShow = true;
    std::string errorMsg = "";
    if (ValidateName(name))
    {
        this->nameIn->setText(name.c_str());
    }
    else
    {
        canShow = false;
        errorMsg = "INVALID NAME LENGHT(3-30)";
    }
    if (ValidateServerID(server))
    {
        this->lobbyIn->setText(server.c_str());
    }
    else
    {
        if (!errorMsg.empty()) errorMsg += "\n";
        canShow = false;
        errorMsg += "INVALID LOBBYID LENGTH(5-30)";
    }

    if (!canShow) {
        errortxt->setText(errorMsg.c_str());
        errortxt->show();
        return;
    }
    else errortxt->hide();


    static std::future<void> fut;

    if (fut.valid() && fut._Is_ready()) fut.get();
    if (!fut.valid())
    {
        fut = std::async([this]() {
            MainApplication* app = MainApplication::GetInstance();
            MainWindow* main = app->mainWindow;
            if (TryConnectToServer())
            {
                QString name = this->nameIn->text();
                QString server = this->lobbyIn->text();
                Base::JoinRequest req;
                req.mutable_info()->mutable_client()->set_name(name.toStdString());
                req.mutable_info()->mutable_client()->set_listengroup(STANDARD_GROUP_MASK);
                req.mutable_info()->set_serverid(server.toStdString());
                const std::vector<PluginClass*>& plugs = GetPlugins();
                for (int i = 0; i < plugs.size(); i++)
                {
                    req.mutable_info()->add_availableplugins(plugs.at(i)->GetPluginInfos().ID);
                }
                req.set_id(app->appData.localPlayerID, 16);
                std::string serializedData = req.SerializeAsString();
                app->socket.SendData(PacketID::JOIN, STANDARD_GROUP_MASK, 0, 0, serializedData);
            }
        });
    }
}
void MainMenuFrame::OnCreateClick()
{
    std::string name = this->nameIn->text().toStdString();
    std::string server = this->lobbyIn->text().toStdString();
    
    bool canShow = true;
    std::string errorMsg = "";
    if (ValidateName(name))
    {
        this->nameIn->setText(name.c_str());
    }
    else
    {
        canShow = false;
        errorMsg = "INVALID NAME LENGHT(3-30)";
    }
    if (ValidateServerID(server))
    {
        this->lobbyIn->setText(server.c_str());
    }
    else
    {
        if(!errorMsg.empty()) errorMsg += "\n";
        canShow = false;
        errorMsg += "INVALID LOBBYID LENGTH(5-30)";
    }

    if (!canShow) {
        errortxt->setText(errorMsg.c_str());
        errortxt->show();
        return;
    }
    else errortxt->hide();
  
    
    static std::future<void> fut;
    if (fut.valid() && fut._Is_ready()) fut.get();
    if (!fut.valid())
    {
        fut = std::async([this]() {
            MainApplication* app = MainApplication::GetInstance();
            MainWindow* main = (MainWindow*)GetMainWindow();
            QString name = this->nameIn->text();
            QString server = this->lobbyIn->text();
            if (TryConnectToServer())
            {
                Base::CreateRequest req;
                req.mutable_info()->mutable_client()->set_name(name.toStdString());
                req.mutable_info()->mutable_client()->set_listengroup(STANDARD_GROUP_MASK);
                req.mutable_info()->set_serverid(server.toStdString());

                const std::vector<PluginClass*>& plugs = GetPlugins();
                for (int i = 0; i < plugs.size(); i++)
                {
                    req.mutable_info()->add_availableplugins(plugs.at(i)->GetPluginInfos().ID);
                }

                std::string serializedData = req.SerializeAsString();
                app->socket.SendData(PacketID::CREATE, STANDARD_GROUP_MASK, 0, 0, serializedData);
            }
        });
    }
}
void MainMenuFrame::OnSettingsClick()
{
    MainWindow* main = (MainWindow*)GetMainWindow();
    main->SetState(MAIN_WINDOW_STATE::STATE_SETTINGS);
}
void MainMenuFrame::OnExitClick()
{
    this->parentWidget()->close();
}