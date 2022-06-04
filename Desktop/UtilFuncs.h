#pragma once
#include <QMainWindow>

QColor AlphaBlend(const QColor& top, const QColor& bottom, float topalpha);


QIcon LoadSvgWithColor(const QString& path, const QColor& c, const QSize& sz);

QMainWindow* GetMainWindow(QWidget* wdgt);

QMainWindow* OverdrawMainWindow(QWidget* wdgt);		// makes the window blank, the children are not deleted, only hidden
QMainWindow* DeleteAndShowBehind(QWidget* wdgt);	// shows the hidden children and deletes the wdgt that was given as an arg

QMainWindow* BlankMainWindow(QWidget* wdgt);	// deletes all children