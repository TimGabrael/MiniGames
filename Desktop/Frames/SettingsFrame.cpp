#include "SettingsFrame.h"

#include "../Widgets/Header.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/qcheckbox.h>
#include <qlabel.h>
#include "../CustomWidgets/CustomCheckBox.h"
#include "../UtilFuncs.h"
#include <qevent.h>
#include <iostream>

SettingsFrame::SettingsFrame(QMainWindow* parent) : StateFrame(parent)
{
	Create();

	parent->setCentralWidget(this);
}
SettingsFrame::~SettingsFrame()
{
}
void SettingsFrame::Create()
{
	QFont usedFont = font();
	usedFont.setPixelSize(28);

	QVBoxLayout* vertical_layout = new QVBoxLayout(this);
	vertical_layout->setContentsMargins(0, 0, 0, 0);
	{
		HeaderWidget* header = new HeaderWidget(this, 0x202020, 0xB0C0D0, "Settings");
		vertical_layout->addWidget(header, 0, Qt::AlignmentFlag::AlignTop);

		vertical_layout->addStretch(2);

		QHBoxLayout* horizontal_layout = new QHBoxLayout(this);
		{
			horizontal_layout->addStretch(1);
			QLabel* label = new QLabel("Fullscreen:");
			label->setFont(usedFont);
			horizontal_layout->addWidget(label, 0, Qt::AlignmentFlag::AlignHCenter);

			fullscreen = new CustomCheckBox(this);
			fullscreen->setCheckState(parentWidget()->isFullScreen() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
			fullscreen->SetTopPadding(10);
			fullscreen->setMinimumSize(28, 38);
			horizontal_layout->addWidget(fullscreen, 0, Qt::AlignmentFlag::AlignHCenter | Qt::AlignmentFlag::AlignVCenter);
			horizontal_layout->addStretch(1);
		}
		vertical_layout->addLayout(horizontal_layout);


		vertical_layout->addStretch(80);

	}

	connect(fullscreen, &QCheckBox::stateChanged, this, &SettingsFrame::OnFullSceen);
	this->setLayout(vertical_layout);
}
void SettingsFrame::FetchSyncData(std::string& str)
{
}
void SettingsFrame::HandleAddClient(const ClientData* added)
{
}
void SettingsFrame::HandleRemovedClient(const ClientData* removed)
{
}
void SettingsFrame::HandleNetworkMessage(Packet* packet)
{
}
void SettingsFrame::HandleSync(const std::string& syncData)
{
}
void SettingsFrame::OnFullSceen()
{
	QMainWindow* main = GetMainWindow();
	if (fullscreen->isChecked())
	{
		main->showFullScreen();
	}
	else
	{
		main->showNormal();
	}

	this->hide();
	this->show();
}


