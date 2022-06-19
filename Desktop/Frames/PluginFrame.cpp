#include "PluginFrame.h"
#include <iostream>
#include <qevent.h>
#include <qopenglframebufferobject.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qboxlayout.h>
#include "../CustomWidgets/PluginWidget.h"

PluginFrame::PluginFrame(QMainWindow* parent, PluginClass* plClass) : QWidget(parent)
{
	QVBoxLayout* lay = new QVBoxLayout(this);

	PluginWidget* pl = new PluginWidget(this, plClass);
	QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
	fmt.setSwapInterval(1);
	//pl->setFormat(fmt);

	lay->addWidget(pl);
	lay->setSpacing(0);
	lay->setContentsMargins(0, 0, 0, 0);


	setLayout(lay);
	parent->setCentralWidget(this);
}
PluginFrame::~PluginFrame()
{


}

