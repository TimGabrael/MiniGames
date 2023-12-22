#include "CustomScrollBar.h"

#include <QStyleOptionSlider>
#include <qpainter.h>

CustomScrollBar::CustomScrollBar(QWidget* parent) : QScrollBar(parent)
{
    setStyleSheet("QScrollBar:vertical {\
        background: #2f2f2f;\
width: 10px;\
    margin : 0;\
}\
\
QScrollBar::handle:vertical{\
background: #5b5b5b;\
}\
\
QScrollBar::add-line:vertical{\
height: 0px;\
}\
\
QScrollBar::sub-line:vertical{\
height: 0px;\
}\
\
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical{\
height: 0px;\
}");
}

CustomScrollBar::~CustomScrollBar() {
}

void CustomScrollBar::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    QRect wnd = rect();
    
    int sliderPos = sliderPosition();
    painter.setPen(Qt::gray);
    painter.setBrush(Qt::gray);
    painter.drawRect(wnd);


    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect slider = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);

    slider.setLeft(slider.left() + 1);
    slider.setRight(slider.right());

    painter.setRenderHints(QPainter::RenderHint::Antialiasing);
    QBrush brush(0x404080);
    painter.setBrush(brush);
    painter.drawRoundedRect(slider, 10.0, 10.0);

}
