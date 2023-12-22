#include  "LobbyFrame.h"
#include "logging.h"
#include <qscrollarea.h>
#include <qpushbutton.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <qpainter.h>
#include <qlabel.h>
#include "../Animations/GrowingCircle.h"

#include "../CustomWidgets/CustomButton.h"
#include "../CustomWidgets/CustomCheckBox.h"
#include "../CustomWidgets/CustomTextInput.h"
#include "../CustomWidgets/CustomScrollbar.h"

#include "../Widgets/Header.h"
#include <qevent.h>

#include "../util/PluginLoader.h"
#include "../UtilFuncs.h"
#include "PluginFrame.h"
#include "../MiniGames.h"
#include <qtimer.h>


VoteData LobbyFrame::data;
constexpr QSize gamePopUpSize = { 400, 300 };
class GameInfo : public QWidget
{
public:
	GameInfo(QWidget* parent, const QString& imgPath, uint16_t plID) : QWidget(parent), anim(this), img(imgPath), plugID(plID), voteCount(0) { 
		setMinimumSize(gamePopUpSize); setMaximumSize(gamePopUpSize); anim.SetDuration(300); selected = nullptr;
	}
    virtual ~GameInfo() {
    }

	static void Unselect()
	{
		if (GameInfo::selected)
		{
			MainApplication* app = MainApplication::GetInstance();
			for (int i = 0; i < LobbyFrame::data.votes.size(); i++)
			{
				auto& v = LobbyFrame::data.votes.at(i);
				if (v.clientID == app->appData.localPlayer.clientID)
				{
					v.pluginID = INVALID_ID;

					break;
				}
			}

			GameInfo* cpy = GameInfo::selected;
			cpy->voteCount--;
			GameInfo::selected = nullptr;

			base::ClientLobbyVote netVote;
			netVote.set_plugin_id(INVALID_ID);
			app->client->SendData(Client_LobbyVote, &netVote, SendFlags::Send_Reliable);

		}
	}
	void SetCount(int count)
	{
		voteCount = count;
		this->update();
	}

	static GameInfo* selected;
private:

	virtual void paintEvent(QPaintEvent* e) override
	{
		bool isSelected = selected == this;
		QPainter painter(this);
		QRect rec = rect();
		painter.drawPixmap(rec, img);
		rec.setRight(rec.right() - 1);
		rec.setBottom(rec.bottom() - 1);

		QPen outlinePen(Qt::white, 50.0);
		painter.setPen(isSelected ? Qt::white : Qt::black);
		painter.setBrush(Qt::transparent);
		painter.drawRect(rec);

		if (voteCount > 0)
		{
			QString voteString = QString::number(voteCount);
			QString testString;
			for (int i = 0; i < voteString.count(); i++) { testString.push_back('9'); }
			QRect bound = fontMetrics().boundingRect(testString);
			int max = std::max(bound.width()/2, bound.height()/2);

			int padding = 4;
			int circlePadding = 4;
			max += circlePadding;

			bound.moveCenter(QPoint(rec.width() - max, max));

			bound.setLeft(rec.right() - max*2 - padding);
			bound.setTop(rec.top() + padding);
			bound.setRight(rec.right() - padding);
			bound.setHeight(max*2);


			painter.setRenderHint(QPainter::RenderHint::Antialiasing);
			painter.setPen(Qt::black);
			QGradient gradien(QGradient::Preset::PhoenixStart);
			QBrush brush(gradien);
			
			painter.setBrush(brush);
			painter.drawEllipse(bound);


			painter.setPen(Qt::black);
			painter.setBrush(Qt::black);
			painter.drawText(bound, Qt::AlignmentFlag::AlignHCenter | Qt::AlignmentFlag::AlignVCenter, voteString);
		}

		anim.Draw(&painter);
	}
	virtual void mouseReleaseEvent(QMouseEvent* e) override
	{
		QRect rec = rect();
		QPoint p = e->pos();

		if ((selected != this) && (rec.left() < p.x() && p.x() < rec.right() && rec.top() < p.y() && p.y() < rec.bottom())) {
			if (selected)
			{
				selected->voteCount--;
				selected->update();
			}
			selected = this;
			voteCount++;
			anim.Start();
			update();

			MainApplication* app = MainApplication::GetInstance();

			VoteInfo* voteInfo = nullptr;
			for (VoteInfo& v : LobbyFrame::data.votes)
			{
				if (v.clientID == app->appData.localPlayer.clientID) {
					v.pluginID = plugID;
					voteInfo = &v;
					break;
				}
			}

			if(!voteInfo)
			{
				LobbyFrame::data.votes.push_back({ app->appData.localPlayer.clientID, plugID });
			}
			
			base::ClientLobbyVote netVote;
			netVote.set_plugin_id(plugID);
			app->client->SendData(Client_LobbyVote, &netVote, SendFlags::Send_Reliable);

		}
	}
	virtual void mouseDoubleClickEvent(QMouseEvent* e) override
	{
		
	}
	GrowingCircleAnim anim;
	QPixmap img;
	uint16_t plugID;
	int voteCount;
};
GameInfo* GameInfo::selected = nullptr;



class ContentWidget : public QScrollArea
{
public:
	ContentWidget(QWidget* parent) : QScrollArea(parent)
	{
		contentArea = nullptr;
		Build();
	}
	~ContentWidget()
	{
	}

	void Build()
	{
		QVBoxLayout* verticalLayout1 = nullptr;
        if (contentArea) 
        {
            GameInfo::selected = nullptr;
            ClearLayout(this->contentArea->layout());
            verticalLayout1 = (QVBoxLayout*)this->contentArea->layout();
        }
        else 
        {
            contentArea = new QWidget(this);
            verticalLayout1 = new QVBoxLayout(this);
            contentArea->setLayout(verticalLayout1);
            this->setWidget(contentArea);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setMinimumSize(100, 100);
            this->setWidgetResizable(true);
            this->setVerticalScrollBar(new CustomScrollBar(this));
            verticalLayout1->setSpacing(30);
            verticalLayout1->setContentsMargins(30, 30, 30, 30);
        }
        contentList.clear();

		{

			{
				MainApplication* app = MainApplication::GetInstance();

				const std::vector<PluginClass*>& plugs = GetPlugins();
				for (int i = 0; i < plugs.size(); i++)
				{
					PLUGIN_INFO info = plugs.at(i)->GetPluginInfos();
					std::string infoStr(info.ID, 19);
					uint16_t found = INVALID_ID;
					for (int j = 0; j < app->serverPlugins.size(); j++)
					{
						const PluginInfo& serverInfo = app->serverPlugins.at(j);
						if (serverInfo.id == infoStr)
						{
							found = serverInfo.sessionID;
							break;
						}
					}
					if (found != INVALID_ID)
					{
						GameInfo* gInf = new GameInfo(this, info.previewResource, found);
						contentList.push_back({ gInf, found });
						verticalLayout1->addWidget(gInf);
					}
					
				}
			}
		}
	}

	void UpdateFromData()
	{
		ApplicationData& app = MainApplication::GetInstance()->appData;
		GameInfo::selected = nullptr;
		for (auto& c : contentList)
		{
			int count = 0;
			for (const auto& v : LobbyFrame::data.votes)
			{
				if (v.pluginID == c.pluginID)
				{
					count++;
					if (v.clientID == app.localPlayer.clientID)
					{
						GameInfo::selected = c.element;
					}
				}
			}
			c.element->SetCount(count);
		}
	}

private:
	struct GameInfoData
	{
		GameInfo* element;
		uint16_t pluginID;
	};

	virtual void mouseReleaseEvent(QMouseEvent* e) override
	{
		QRect rec = rect();
		QPoint p = e->pos();

		if ((rec.left() < p.x() && p.x() < rec.right() && rec.top() < p.y() && p.y() < rec.bottom())) {
			GameInfo::Unselect();
			contentArea->update();

		}
	}
	QWidget* contentArea = nullptr;
	std::vector<GameInfoData> contentList;
};


class AdminWidget : public QWidget
{
public:
	AdminWidget(QWidget* parent) : QWidget(parent)
	{
		//this->setPalette(QPalette(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
		QGridLayout* grid_layout = new QGridLayout(this);
		
		startBtn = new CustomButton("START", this);
		time = new QLabel("Remaining Time: -1", this);
		adminChooses = new CustomCheckBox(this);
		adminChooses->setCheckState(Qt::CheckState::Unchecked);
		adminChooses->setText("Test");

		countDown.setParent(this);
		countDown.setInterval(1000);
		countDown.callOnTimeout(this, &AdminWidget::OnTimer);
		
		if (LobbyFrame::data.isRunning)
		{
			countDown.start();
		}

		
		grid_layout->addWidget(time, 0, 0);
		grid_layout->addWidget(startBtn, 1, 0);
		grid_layout->addWidget(adminChooses, 1, 1);

		startBtn->setDisabled(false);

		
		this->setLayout(grid_layout);

		connect(startBtn, &QPushButton::pressed, this, &AdminWidget::OnPressStart);
	}


	void SetTimer(float time)
	{
		countDown.start();
		LobbyFrame::data.remainingTime = time;
		LobbyFrame::data.isRunning = true;
	}

	void OnPressStart()
	{
		MainApplication* app = MainApplication::GetInstance();
		if (app->appData.localPlayer.isAdmin)
		{
			if (!countDown.isActive())
			{
				float waitTime = adminChooses->checkState() == Qt::Checked ? 0.0f : maxTimer;
				base::ClientLobbyAdminTimer timerMsg;
				timerMsg.set_time(waitTime);
				app->client->SendData(Client_LobbyAdminTimer, &timerMsg, SendFlags::Send_Reliable);
			}
		}
	}
	void UpdateWithRemainingTime()
	{
		std::string remTxt = "Remaining Time: " + std::to_string(LobbyFrame::data.remainingTime);
		time->setText(remTxt.c_str());
		time->update();
	}

	void OnTimer()
	{
		if (LobbyFrame::data.remainingTime > 0)
		{
			LobbyFrame::data.remainingTime -= 1;
			if (LobbyFrame::data.remainingTime <= 0) {
				LobbyFrame::data.remainingTime = 0;
				LobbyFrame::data.isRunning = false;
			}
			UpdateWithRemainingTime();
		}
		else
		{
			countDown.stop();
		}
	}

	void UpdateFromData()
	{
		if (LobbyFrame::data.isRunning && !countDown.isActive())
		{
			countDown.start();
		}
		else if (!LobbyFrame::data.isRunning && countDown.isActive())
		{
			UpdateWithRemainingTime();
			countDown.stop();
		}
	}


private:
	CustomButton* startBtn = nullptr;
	CustomCheckBox* adminChooses = nullptr;
	QLabel* time = nullptr;
	int maxTimer = 30;
	QTimer countDown;
};


bool IsValidName(const std::string& name)
{
	MainApplication* app = MainApplication::GetInstance();
	if (app->appData.localPlayer.name == name)
	{
		return true;
	}
	for (auto& p : app->appData.players)
	{
		if (p.name == name) {
			return true;
		}
	}
	return false;
}








LobbyFrame::LobbyFrame(QMainWindow* parent) : StateFrame(parent)
{
	QVBoxLayout* vertical_layout = new QVBoxLayout(this);
	vertical_layout->setSpacing(0);
	vertical_layout->setContentsMargins(0, 0, 0, 0);
	{
		HeaderWidget* header = new HeaderWidget(this, 0x101010, 0x4040f0, "Lobby");
		vertical_layout->addWidget(header, 0 , Qt::AlignmentFlag::AlignTop);

		vertical_layout->addSpacing(10);
		QHBoxLayout* horizontal_layout = new QHBoxLayout();
		{
			horizontal_layout->addSpacing(10);
			QScrollArea* area = new QScrollArea(this);
			area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			area->setMinimumSize(80, 100);
			area->setMaximumSize(600, 800);
			area->setWidgetResizable(true);
			area->setVerticalScrollBar(new CustomScrollBar(this));
			{
				this->nameList = new QVBoxLayout();

				playerScrollContent = new QWidget(this);
				ApplicationData& data = MainApplication::GetInstance()->appData;
				
				QLabel* lab = new QLabel(data.localPlayer.name.c_str(), this);
				this->nameList->addWidget(lab);
				if(data.localPlayer.isAdmin) lab->setPalette(QPalette(0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00));
				else lab->setPalette(QPalette(0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00));

				for (int i = 0; i < data.players.size(); i++)
				{
					auto& p = data.players.at(i);
					QLabel* lab = new QLabel(p.name.c_str(), this);
					this->nameList->addWidget(lab);
					if (p.isAdmin) lab->setPalette(QPalette(0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700));
				}
				this->nameList->addStretch(1);
				playerScrollContent->setLayout(this->nameList);
				area->setWidget(playerScrollContent);
			}
			horizontal_layout->addWidget(area, 4);
			horizontal_layout->addStretch(1);


			adminWidget = new AdminWidget(this);
			horizontal_layout->addWidget(adminWidget, 4, Qt::AlignTop | Qt::AlignLeft);
		}
		vertical_layout->addLayout(horizontal_layout, 20);
	
		gamesContent = new ContentWidget(this);
		vertical_layout->addWidget(gamesContent, 25);
	}

	this->setLayout(vertical_layout);
	parent->setCentralWidget(this);


	UpdateFromData();
}
LobbyFrame::~LobbyFrame()
{

}


void LobbyFrame::StartPlugin()
{
	MainWindow* main = (MainWindow*)GetMainWindow();

	PluginFrame::activePlugin = nullptr;
	auto& pl = GetPlugins();
	for (int i = 0; i < pl.size(); i++)
	{
		std::string avPlug(pl.at(i)->GetPluginInfos().ID, 19);
		if (avPlug == pluginCache)
		{
			PluginFrame::activePlugin = pl.at(i);
			break;
		}
	}
	if (PluginFrame::activePlugin) {
		main->layout()->removeWidget(this);
		delete this;
		main->SetState(MAIN_WINDOW_STATE::STATE_PLUGIN);
	}
}
void LobbyFrame::Rebuild()
{
    ApplicationData& data = MainApplication::GetInstance()->appData;
    ClearLayout(this->nameList);

    QLabel* lab = new QLabel(data.localPlayer.name.c_str(), this);
    this->nameList->addWidget(lab);
    if(data.localPlayer.isAdmin) lab->setPalette(QPalette(0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00));
    else lab->setPalette(QPalette(0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00));

    for (int i = 0; i < data.players.size(); i++)
    {
        auto& p = data.players.at(i);
        QLabel* lab = new QLabel(p.name.c_str(), this);
        this->nameList->addWidget(lab);
        if (p.isAdmin) lab->setPalette(QPalette(0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700));
    }
    this->nameList->addStretch(1);
    this->playerScrollContent->setLayout(this->nameList);
    this->gamesContent->Build();

	this->gamesContent->UpdateFromData();
	this->adminWidget->UpdateFromData();
}


void LobbyFrame::UpdateFromData()
{
	gamesContent->UpdateFromData();
	adminWidget->UpdateFromData();
}

void LobbyFrame::SetTimer(float timer)
{
	timerCache = timer;
	SafeAsyncUI([](MainWindow* wnd) {
		LobbyFrame* frame = (LobbyFrame*)wnd->stateWidget;
		frame->adminWidget->SetTimer(frame->timerCache);
	});
}
