#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H

#include <QWidget>

class QSlider;
class QPushButton;
class QVBoxLayout;
class QGroupBox;

class ParamWidget : public QWidget
{
	Q_OBJECT
public:
	ParamWidget();

	QGroupBox* cannyGroup;
	QSlider* cannyThreshold1;
	QSlider* cannyThreshold2;
	
	QGroupBox* houghGroup;;
	QSlider* houghThreshold;
	QSlider* houghParam1;
	QSlider* houghParam2;

signals:
	void process();

private slots:
	void buttonPressed();

private:
	enum{
		SLIDER_WIDTH = 200,
		LABEL_WIDTH = 80,
		EDIT_WIDTH = 40,
		BUTTON_WIDTH = 100
	};
	QGroupBox* addGroup(QString groupName);
	QSlider* addParam(QString paramName, int min, int max, int value);
	QPushButton* addButton(QString buttonName);

	QVBoxLayout* mainLayout;
	QGroupBox* group;
	QVBoxLayout* groupLayout;

	QPushButton* buttonProcess;

};

#endif //PARAMWIDGET_H