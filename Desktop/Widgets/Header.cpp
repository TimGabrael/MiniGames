#include "Header.h"


#include <qtoolbutton.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <qstyle.h>
#include <qpushbutton.h>
#include "../UtilFuncs.h"

#include "../Frames/MainMenuFrame.h"
#include "../Frames/SettingsFrame.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <qframe.h>
#include <qstatictext.h>

#include <iostream>
#include "../Application.h"
#include <qevent.h>
#include "../MiniGames.h"


HeaderWidget::HeaderWidget(QWidget* parent, const QColor& stylebg, const QColor& stylehighlight, const QString& headline) : QWidget(parent)
{	
	QMainWindow* mainWindow = GetMainWindow();

	QPalette pal = palette();
	pal.setColor(QPalette::ColorRole::Window, stylebg);
	pal.setColor(QPalette::ColorRole::Text, stylehighlight);
	parent->setPalette(pal);
	setPalette(pal);

	QHBoxLayout* horizontal_layout = new QHBoxLayout(this);
	{
		QSize BtnSize(40, 30);
		QString btnStyles = QString("QPushButton { background-color: #%1; } QPushButton:pressed{ background-color: #%2; }").arg(stylebg.lighter().rgb(), 1, 16).arg(stylebg.darker().rgb(), 1, 16);

		hBtn = new QPushButton(this);
		hBtn->setStyleSheet(btnStyles);
		hBtn->setMinimumSize(BtnSize);
		QIcon home = LoadSvgWithColor("Assets/Home.svg", stylehighlight, BtnSize);
		hBtn->setIcon(home);
		hBtn->setIconSize(BtnSize);

		horizontal_layout->addWidget(hBtn, 0, Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignTop);


		backBtn = new QPushButton(this);
		backBtn->setStyleSheet(btnStyles);
		backBtn->setMinimumSize(BtnSize);
		QIcon back = LoadSvgWithColor("Assets/BackArrow.svg", stylehighlight, BtnSize);
		backBtn->setIcon(back);
		backBtn->setIconSize(BtnSize);

		horizontal_layout->addWidget(backBtn, 0);


		horizontal_layout->addStretch(1);

		QLabel* text = new QLabel(headline, this);
		text->setPalette(pal);
		QFont f = text->font();
		f.setPixelSize(BtnSize.height());
		f.setUnderline(true);
		text->setFont(f);
		text->setPalette(QPalette(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
		horizontal_layout->addWidget(text, 1, Qt::AlignmentFlag::AlignHCenter);

		QLabel* currentLobby = new QLabel(("ROOM: " + MainApplication::GetInstance()->appData.roomName).c_str(), this);
		text->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
		currentLobby->setMinimumWidth(100);
		
		horizontal_layout->addWidget(currentLobby, 1, Qt::AlignmentFlag::AlignRight);



		//horizontal_layout->addStretch();

		sBtn = new QPushButton(this);
		sBtn->setStyleSheet(btnStyles);
		sBtn->setMinimumSize(BtnSize);
		QIcon cog = LoadSvgWithColor("Assets/SettingsCog.svg", stylehighlight, BtnSize);
		sBtn->setIcon(cog);
		sBtn->setIconSize(BtnSize);
		horizontal_layout->addWidget(sBtn, 0, Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignTop);

		
		fullscreenCloseBtn = new QPushButton(this);
		fullscreenCloseBtn->setStyleSheet(btnStyles);
		fullscreenCloseBtn->setMinimumSize(BtnSize);
		QIcon close = LoadSvgWithColor("Assets/Close.svg", 0xFF0000, BtnSize);
		fullscreenCloseBtn->setIcon(close);
		fullscreenCloseBtn->setIconSize(BtnSize);
		

		horizontal_layout->addWidget(fullscreenCloseBtn);
		connect(fullscreenCloseBtn, &QPushButton::clicked, this, &HeaderWidget::OnFullscreenClosePress);
		if (!mainWindow->isFullScreen()) fullscreenCloseBtn->hide();
		


	}
	
	this->setLayout(horizontal_layout);

	setAutoFillBackground(true);
	
	connect(hBtn, &QPushButton::clicked, this, &HeaderWidget::OnHomePress);
	connect(backBtn, &QPushButton::clicked, this, &HeaderWidget::OnBackPress);
	connect(sBtn, &QPushButton::clicked, this, &HeaderWidget::OnCogPress);
}

void HeaderWidget::OnCogPress()
{
	MainWindow* main = (MainWindow*)GetMainWindow();
	main->SetState(MAIN_WINDOW_STATE::STATE_SETTINGS);
}
void HeaderWidget::OnHomePress()
{
	MainWindow* main = (MainWindow*)GetMainWindow();
	
	main->SetState(MAIN_WINDOW_STATE::STATE_MENU);
	
}
void HeaderWidget::OnBackPress()
{
	MainWindow* main = (MainWindow*)GetMainWindow();
	main->SetPreviousState();
}

void HeaderWidget::OnFullscreenClosePress()
{
	QMainWindow* main = GetMainWindow();
	main->close();
}

void HeaderWidget::showEvent(QShowEvent* e)
{
	MainWindow* main = (MainWindow*)GetMainWindow();
	if (e->type() == e->Show)
	{
		if (main->isFullScreen()) fullscreenCloseBtn->show();
		else fullscreenCloseBtn->hide();
	}
}