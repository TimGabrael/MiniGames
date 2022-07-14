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
#include "Network/Messages/lobby.pb.h"
#include <qtimer.h>


VoteData LobbyFrame::data;
constexpr QSize gamePopUpSize = { 400, 300 };
class GameInfo : public QWidget
{
public:
	GameInfo(QWidget* parent, const QString& imgPath, const std::string& plID) : QWidget(parent), img(imgPath), voteCount(0), anim(this), plugID(plID) { 
		setMinimumSize(gamePopUpSize); setMaximumSize(gamePopUpSize); anim.SetDuration(300); selected = nullptr;
	}

	static void Unselect()
	{
		if (GameInfo::selected)
		{
			MainApplication* app = MainApplication::GetInstance();
			for (int i = 0; i < LobbyFrame::data.votes.size(); i++)
			{
				auto& v = LobbyFrame::data.votes.at(i);
				if (v.username == app->appData.localPlayer.name)
					v.pluginID = "";
			}
			Base::Vote voteData;
			voteData.set_clientname(app->appData.localPlayer.name);
			app->socket.SendData(PacketID::VOTE, LISTEN_GROUP_ALL, 0, voteData.SerializeAsString());

			GameInfo* cpy = GameInfo::selected;
			cpy->voteCount--;
			GameInfo::selected = nullptr;
		}
	}
	void SetCount(int count)
	{
		voteCount = count;
		this->update();
	}

	static GameInfo* selected;
private:

	LobbyFrame* GetLobbyFrame()
	{
		return (LobbyFrame*)this->parentWidget()->parentWidget();	// this->ContentWidget->LobbyFrame
	}

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

			Base::Vote voteMessage;
			voteMessage.set_plugin(plugID);
			voteMessage.set_clientname(app->appData.localPlayer.name);
			app->socket.SendData(PacketID::VOTE, 0xFFFFFFFF, 0, voteMessage.SerializeAsString());


			for (auto& v : LobbyFrame::data.votes)
			{
				if (v.username == app->appData.localPlayer.name) {
					v.pluginID = plugID;
					return;
				}
			}
			LobbyFrame::data.votes.push_back({ plugID, app->appData.localPlayer.name });

			//GetLobbyFrame()->StartPlugin(0);
		}
	}
	virtual void mouseDoubleClickEvent(QMouseEvent* e) override
	{
		
	}
	GrowingCircleAnim anim;
	QPixmap img;
	std::string plugID;
	int voteCount;
};
GameInfo* GameInfo::selected = nullptr;



class ContentWidget : public QScrollArea
{
public:
	ContentWidget(QWidget* parent) : QScrollArea(parent)
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setMinimumSize(100, 100);
		this->setWidgetResizable(true);
		setVerticalScrollBar(new CustomScrollBar(this));

		//setPalette(QPalette(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF));
		contentArea = new QWidget(this);
		QVBoxLayout* vertical_layout1 = new QVBoxLayout(this);
		{
			vertical_layout1->setSpacing(30);
			vertical_layout1->setContentsMargins(30, 30, 30, 30);

			QSize gamePrevSize(400, 300);

			{
				const std::vector<PluginClass*>& plugs = GetPlugins();
				for (int i = 0; i < plugs.size(); i++)
				{
					PLUGIN_INFO info = plugs.at(i)->GetPluginInfos();
					LOG("RESOURCE: %s %s\n", info.previewResource, info.ID);
					
					GameInfo* gInf = new GameInfo(this, info.previewResource, info.ID);
					contentList.push_back({gInf, info.ID});
					vertical_layout1->addWidget(gInf);
				}
			}
			//GameInfo* unoLabel = new GameInfo(this, "Assets/Uno.png");
			//vertical_layout1->addWidget(unoLabel);

		}
		contentArea->setLayout(vertical_layout1);
		this->setWidget(contentArea);
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
					if (v.username == app.localPlayer.name)
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
		std::string pluginID;
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
		
		ApplicationData& data = MainApplication::GetInstance()->appData;
		
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

	void AdminSyncData()
	{
		MainApplication* app = MainApplication::GetInstance();
		if (app->appData.localPlayer.groupMask & ADMIN_GROUP_MASK)
		{
			Base::SyncData data;
			data.set_remainingtime(maxTimer);
			for (int i = 0; i < LobbyFrame::data.votes.size(); i++)
			{
				Base::Vote* v = data.add_votes();
				const auto& vote = LobbyFrame::data.votes.at(i);
				v->set_plugin(vote.pluginID); v->set_clientname(vote.username);
			}
			data.set_running(LobbyFrame::data.isRunning);
			app->socket.SendData(PacketID::VOTE_SYNC, LISTEN_GROUP_ALL, ADDITIONAL_DATA_FLAG_ADMIN, data.SerializeAsString());
		}
	}
	void AdminStart()
	{
		MainApplication* app = MainApplication::GetInstance();
		if (app->appData.localPlayer.groupMask & ADMIN_GROUP_MASK)
		{
			Base::StartPlugin starting;
			std::string start = GetPluginByVotes();
			starting.set_pluginid(start);
			app->socket.SendData(PacketID::START, LISTEN_GROUP_ALL, ADDITIONAL_DATA_FLAG_ADMIN, starting.SerializeAsString());

			LobbyFrame* frame = (LobbyFrame*)this->parentWidget();
			frame->pluginCache = start;
			frame->StartPlugin();
		}
	}
	std::string GetPluginByVotes()
	{
		std::string outVal;
		int highestCount = -1;
		const std::vector<PluginClass*>& pl = GetPlugins();
		for (int i = 0; i < pl.size(); i++)
		{
			int curCount = 0;
			std::string id = pl.at(i)->GetPluginInfos().ID;
			for (int j = 0; j < LobbyFrame::data.votes.size(); j++)
			{
				auto& v = LobbyFrame::data.votes.at(j);
				if (v.pluginID == id)
				{
					curCount++;
				}
			}
			if (curCount > 0 && highestCount < curCount)
			{
				outVal = id;
				highestCount = curCount;
			}
		}

		if (highestCount != -1 && !outVal.empty())
		{
			return outVal;
		}

		int randIDX = rand() % pl.size();
		if (randIDX < pl.size())
		{
			return pl.at(randIDX)->GetPluginInfos().ID;
		}

		return "";
	}

	void OnPressStart()
	{
		if (!countDown.isActive())
		{
			MainApplication* app = MainApplication::GetInstance();
			if (adminChooses->checkState() == Qt::CheckState::Checked)
			{
				AdminStart();
			}
			else
			{
				countDown.start();
				LobbyFrame::data.remainingTime = maxTimer;
				LobbyFrame::data.isRunning = true;
				UpdateWithRemainingTime();
				AdminSyncData();
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
				AdminSyncData();
			}
			UpdateWithRemainingTime();
		}
		else
		{
			countDown.stop();
			AdminStart();
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
	CustomTextInput* timeIn = nullptr;
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
		QHBoxLayout* horizontal_layout = new QHBoxLayout(this);
		{
			horizontal_layout->addSpacing(10);
			QScrollArea* area = new QScrollArea(this);
			area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			area->setMinimumSize(80, 100);
			area->setMaximumSize(600, 800);
			area->setWidgetResizable(true);
			area->setVerticalScrollBar(new CustomScrollBar(this));
			{
				QVBoxLayout* vertical_layout1 = new QVBoxLayout(this);

				playerScrollContent = new QWidget(this);
				ApplicationData& data = MainApplication::GetInstance()->appData;
				
				QLabel* lab = new QLabel(data.localPlayer.name.c_str(), this);
				vertical_layout1->addWidget(lab);
				if(data.localPlayer.groupMask & ADMIN_GROUP_MASK) lab->setPalette(QPalette(0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0xFFFF00));
				else lab->setPalette(QPalette(0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00));

				for (int i = 0; i < data.players.size(); i++)
				{
					auto& p = data.players.at(i);
					QLabel* lab = new QLabel(p.name.c_str(), this);
					vertical_layout1->addWidget(lab);
					if (p.groupMask & ADMIN_GROUP_MASK) lab->setPalette(QPalette(0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700, 0xffd700));
				}
				vertical_layout1->addStretch(1);
				playerScrollContent->setLayout(vertical_layout1);
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

void LobbyFrame::FetchSyncData(std::string& str)
{
}

void LobbyFrame::HandleAddClient(const ClientData* added)
{
	AddPlayer(added->name);
}

void LobbyFrame::HandleRemovedClient(const ClientData* removed)
{
	RemovePlayer(removed->name);
}

void LobbyFrame::HandleNetworkMessage(Packet* packet)
{
	if (VoteDataHandleNetworkMessage(packet))
	{
		QTimer::singleShot(0, this, &LobbyFrame::UpdateFromData);
	}
	else if (packet->header.type == (uint32_t)PacketID::START && (packet->header.additionalData == ADDITIONAL_DATA_FLAG_ADMIN))
	{
		Base::StartPlugin startInfo;
		startInfo.ParseFromArray(packet->body.data(), packet->body.size());
		this->pluginCache = startInfo.pluginid();
		QTimer::singleShot(0, this, &LobbyFrame::StartPlugin);
	}
}

void LobbyFrame::HandleSync(const std::string& syncData)
{
	data.votes.clear();
	Base::SyncData sync;
	sync.ParseFromString(syncData);
	for (int i = 0; i < sync.votes_size(); i++)
	{
		const Base::Vote& vote = sync.votes(i);
		if (IsValidName(vote.clientname()))
		{
			if (vote.has_plugin()) data.votes.push_back({ vote.plugin(), vote.clientname() });
			else data.votes.push_back({ "", vote.clientname() });
		}
	}
	data.remainingTime = sync.remainingtime();
}




void LobbyFrame::AddPlayer(const std::string& name)
{
	QVBoxLayout* lay = (QVBoxLayout*)playerScrollContent->layout();
	QLabel* label = new QLabel(name.c_str(), this);
	int wdgtCount = lay->count();
	lay->insertWidget(wdgtCount - 1, label);
	label->show();
}
void LobbyFrame::RemovePlayer(const std::string& name)
{
	QLayout* lay = playerScrollContent->layout();
	for (int i = 0; i < lay->count(); i++)
	{
		QLabel* lab = (QLabel*)lay->itemAt(i)->widget();
		if (lab->text().toStdString() == name)
		{
			QLayoutItem* item = lay->takeAt(i);
			delete item->widget();
			delete item;
			return;
		}
	}
}

void LobbyFrame::ReSync()
{
	ApplicationData& data = MainApplication::GetInstance()->appData;
	{
		QVBoxLayout* pLay = (QVBoxLayout*)playerScrollContent->layout();
		ClearLayout(pLay);
		for (int i = 0; i < data.players.size(); i++)
		{
			pLay->addWidget(new QLabel(data.players.at(i).name.c_str(), playerScrollContent));
		}
		pLay->addStretch(1);
	}
}

void LobbyFrame::UpdateFromData()
{
	gamesContent->UpdateFromData();
	adminWidget->UpdateFromData();
}





bool LobbyFrame::VoteDataHandleNetworkMessage(Packet* packet)
{
	ApplicationData& appData = MainApplication::GetInstance()->appData;
	if (packet->header.type == (uint32_t)PacketID::VOTE)
	{
		Base::Vote added;
		added.ParseFromArray(packet->body.data(), packet->body.size());
		const std::string& plug = added.plugin();
		const std::string& add = added.clientname();
		
		if (!IsValidName(add)) return true;

		for (int i = 0; i < data.votes.size(); i++)
		{
			VoteInfo& info = data.votes.at(i);
			if(add == info.username)
			{
				data.votes.erase(data.votes.begin() + i);
				i--;
			}
		}
		if (added.has_plugin())
			data.votes.push_back({ plug, add });
		else
			data.votes.push_back({ "", add });


		return true;
	}
	else if (packet->header.type == (uint32_t)PacketID::VOTE_SYNC)
	{
		data.votes.clear();
		Base::SyncData sync;
		sync.ParseFromArray(packet->body.data(), packet->body.size());
		for (int i = 0; i < sync.votes_size(); i++)
		{
			const Base::Vote& vote = sync.votes(i);
			if (IsValidName(vote.clientname()))
			{
				if (vote.has_plugin()) data.votes.push_back({ vote.plugin(), vote.clientname() });
				else data.votes.push_back({ "", vote.clientname() });
			}
		}
		data.remainingTime = sync.remainingtime();
		data.isRunning = sync.running();
		return true;
	}

	return false;
}