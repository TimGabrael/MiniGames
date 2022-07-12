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
			GameInfo* cpy = GameInfo::selected;
			cpy->voteCount--;
			GameInfo::selected = nullptr;
			// cpy->update();
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



void ScrollbarExample(QWidget* frame, QMainWindow* parent)
{
	QScrollArea* area = new QScrollArea(frame);
	area->setMinimumSize(1200, 250);

	QVBoxLayout* vbox = new QVBoxLayout(frame);

	QWidget* content = new QWidget(frame);
	content->setLayout(vbox);

	QPushButton* img = new CustomButton("test1", frame);
	img->setMinimumSize(1000, 400);

	QPushButton* img1 = new CustomButton("test", frame);
	img1->setMinimumSize(1000, 400);

	vbox->addWidget(img);
	vbox->addWidget(img1);

	CustomScrollBar* scrl = new CustomScrollBar(frame);
	area->setVerticalScrollBar(scrl);
	area->setWidget(content);


	parent->setCentralWidget(frame);
}

void onMouseReleaseScrollArea()
{
	GameInfo::Unselect();
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
				lab->setPalette(QPalette(0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00));
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
			horizontal_layout->addWidget(area, 1);
			horizontal_layout->addStretch(1);

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


void LobbyFrame::StartPlugin(int idx)
{
	MainWindow* main = (MainWindow*)GetMainWindow();
	main->layout()->removeWidget(this);
	delete this;

	auto& pl = GetPlugins();
	if (idx < pl.size()) {
		PluginFrame::activePlugin = pl.at(idx);
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
		QTimer::singleShot(0, gamesContent, &ContentWidget::UpdateFromData);
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
	//QLayout* gLay = gamesContent->layout();
	//ClearLayout(gLay);
	//const std::vector<PluginClass*>& plugs = GetPlugins();
	//for (int i = 0; i < plugs.size(); i++)
	//{
	//	PLUGIN_INFO info = plugs.at(i)->GetPluginInfos();
	//	gLay->addWidget(new GameInfo(this, info.previewResource, info.ID));
	//}
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
		return true;
	}
	return false;
}