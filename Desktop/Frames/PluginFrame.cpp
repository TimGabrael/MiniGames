#include "PluginFrame.h"
#include <iostream>
#include <qevent.h>
#include <qopenglframebufferobject.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qboxlayout.h>
#include "../CustomWidgets/PluginWidget.h"

PluginClass* PluginFrame::activePlugin = nullptr;

PluginFrame::PluginFrame(QMainWindow* parent) : StateFrame(parent)
{
	QVBoxLayout* lay = new QVBoxLayout(this);

	PluginWidget* pl = new PluginWidget(this, activePlugin);

	lay->addWidget(pl);
	lay->setSpacing(0);
	lay->setContentsMargins(0, 0, 0, 0);


	setLayout(lay);
	parent->setCentralWidget(this);
}
PluginFrame::~PluginFrame()
{


}