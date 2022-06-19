#include "MainMenuFrame.h"
#include "../CustomWidgets/CustomButton.h"
#include "../CustomWidgets/CustomTextInput.h"
#include "SettingsFrame.h"
#include "LobbyFrame.h"
#include "CommonCollection.h"
#include "../Application.h"



MainMenuFrame::MainMenuFrame(QMainWindow* MainWindow) : QWidget(MainWindow)
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


        verticalLayout->addStretch(1);

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
        verticalLayout->addSpacing(20);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->addStretch(10);
        horizontalLayout_2->setSpacing(20);
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


    connect(createBtn, &QPushButton::clicked, this, &MainMenuFrame::OnCreateClick);
    connect(settingsBtn, &QPushButton::clicked, this, &MainMenuFrame::OnSettingsClick);
    connect(exitBtn, &QPushButton::clicked, this, &MainMenuFrame::OnExitClick);
    MainWindow->setCentralWidget(this);
}
MainMenuFrame::~MainMenuFrame()
{
}



void MainMenuFrame::OnCreateClick()
{
    QMainWindow* main = (QMainWindow*)parentWidget();
    main->layout()->removeWidget(this);
    delete this;

    auto app = MainApplication::GetInstance();
    TestPacket pack("Hallo Ich Bins aus der funktion");
    app->socket.SendPacket(&pack);


    LobbyFrame* lobby = new LobbyFrame(main);
}
void MainMenuFrame::OnSettingsClick()
{
    QMainWindow* main = (QMainWindow*)parentWidget();
    main->layout()->removeWidget(this);
    delete this;

    SettingsFrame* settings = new SettingsFrame(main);
}
void MainMenuFrame::OnExitClick()
{
    this->parentWidget()->close();
}