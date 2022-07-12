#pragma once

#include <qwidget.h>
#include <qmainwindow.h>
#include <string>



struct ClientData;
struct Packet;
class StateFrame : public QWidget
{
public:
	StateFrame(QMainWindow* parent) : QWidget(parent) {}
	virtual void FetchSyncData(std::string& str) = 0;		// FUNCTION NOT IN UI THREAD
	virtual void HandleAddClient(const ClientData* added) = 0;
	virtual void HandleRemovedClient(const ClientData* removed) = 0;

	virtual void HandleNetworkMessage(Packet* packet) = 0;	// FUNCTION NOT IN UI THREAD
	virtual void HandleSync(const std::string& syncData) = 0;

};