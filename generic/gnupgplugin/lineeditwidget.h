#ifndef LINEEDITWIDGET_H
#define LINEEDITWIDGET_H

#include <QLineEdit>
#include <QToolButton>
#include <QList>

class QFrame;
class QHBoxLayout;

class LineEditWidget : public QLineEdit
{
	Q_OBJECT
	Q_PROPERTY(int optimalLength READ optimalLenth WRITE setOptimalLength)
	Q_PROPERTY(QString rxValidator READ rxValidator WRITE setRxValidator)
public:
	explicit LineEditWidget(QWidget *parent = 0);
	~LineEditWidget();

	// reimplemented
	QSize sizeHint() const;
	void showEvent(QShowEvent *e);
	bool eventFilter(QObject *o, QEvent *e);

	// Properties
	int optimalLenth() const { return _optimalLength; }
	void setOptimalLength(int optimalLength) { _optimalLength = optimalLength; }

	QString rxValidator() const { return _rxValidator; }
	void setRxValidator(const QString &str);

protected:
	void addWidget(QWidget *w);
	void setPopup(QWidget* w);
	QFrame *popup() const { return _popup; };

protected slots:
	virtual void showPopup();
	virtual void hidePopup();

private:

	QHBoxLayout *_layout;
	QList<QWidget*> _toolbuttons;
	QFrame *_popup;

	// Properties
	int _optimalLength;
	QString _rxValidator;
};

#endif // LINEEDITWIDGET_H
