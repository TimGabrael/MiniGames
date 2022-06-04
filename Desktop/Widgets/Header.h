#include <qwidget.h>



class HeaderWidget : public QWidget
{
public:
	HeaderWidget(QWidget* parent, const QColor& stylebg, const QColor& stylehighlight, const QString& headline = "");
	

private:
	class QPushButton* hBtn;
	class QPushButton* backBtn;
	class QPushButton* sBtn;
	class QPushButton* fullscreenCloseBtn;

	void OnCogPress();
	void OnHomePress();
	void OnBackPress();
	void OnFullscreenClosePress();


	virtual void showEvent(QShowEvent* e) override;

};