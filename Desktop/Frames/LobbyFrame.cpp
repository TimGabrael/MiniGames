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


constexpr QSize gamePopUpSize = { 400, 300 };
class GameInfo : public QWidget
{
public:
	GameInfo(QWidget* parent, const QString& imgPath) : QWidget(parent), img(imgPath), voteCount(0), anim(this) { 
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
			GetLobbyFrame()->StartPlugin(0);
		}
	}
	virtual void mouseDoubleClickEvent(QMouseEvent* e) override
	{
		
	}
	static GameInfo* selected;
	GrowingCircleAnim anim;
	QPixmap img;
	int voteCount;
};
GameInfo* GameInfo::selected = nullptr;



class ContentWidget : public QScrollArea
{
public:
	ContentWidget(QWidget* parent) : QScrollArea(parent)
	{
		setMinimumSize(100, 100);
		setVerticalScrollBar(new CustomScrollBar(this));

		// setPalette(QPalette(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF));
		contentArea = new QWidget(this);
		QVBoxLayout* vertical_layout1 = new QVBoxLayout(this);
		{
			vertical_layout1->setSpacing(30);
			vertical_layout1->setContentsMargins(30, 30, 30, 30);
			setLayout(vertical_layout1);

			QSize gamePrevSize(400, 300);

			GameInfo* unoLabel = new GameInfo(this, "Assets/Uno.png");
			vertical_layout1->addWidget(unoLabel);
		}
		contentArea->setLayout(vertical_layout1);
		this->setWidget(contentArea);
	}

private:

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
};





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
LobbyFrame::LobbyFrame(QMainWindow* parent) : QWidget(parent)
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
			area->setMinimumSize(80, 100);
			area->setMaximumSize(600, 800);
			area->setVerticalScrollBar(new CustomScrollBar(this));
			{
				QVBoxLayout* vertical_layout1 = new QVBoxLayout(this);

				playerScrollContent = new QWidget(area);
				playerScrollContent->setLayout(vertical_layout1);



			}
			horizontal_layout->addWidget(area, 1);
			horizontal_layout->addStretch(1);


		}
		vertical_layout->addLayout(horizontal_layout, 20);
	
	
		ContentWidget* gamesContent = new ContentWidget(this);
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
		// PluginFrame* f = new PluginFrame(main, pl.at(idx));
	}
}