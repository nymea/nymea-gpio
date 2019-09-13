#ifndef GPIOBUTTON_H
#define GPIOBUTTON_H

#include <QTime>
#include <QTimer>
#include <QObject>

#include "gpiomonitor.h"

class GpioButton : public QObject
{
    Q_OBJECT
public:
    explicit GpioButton(int gpioNumber, QObject *parent = nullptr);

    int gpioNumber() const;

    bool activeLow() const;
    void setActiveLow(bool activeLow);

    bool repeateLongPressed() const;
    void setRepeateLongPressed(bool repeateLongPressed);

    int longPressedTimeout() const;
    void setLongPressedTimeout(int longPressedTimeout);

    QString name() const;
    void setName(const QString &name);

private:
    int m_gpioNumber;
    bool m_activeLow = true;
    bool m_repeateLongPressed = false;
    int m_longPressedTimeout = 250;
    QString m_name;

    GpioMonitor *m_monitor = nullptr;
    QTimer *m_timer = nullptr;

    QTime m_time;

signals:
    void clicked();
    void pressed();
    void released();
    void longPressed();

private slots:
    void onTimeout();
    void onInterruptOccured(bool value);

public slots:
    bool enable();
    void disable();

};

QDebug operator<< (QDebug debug, GpioButton *gpioButton);


#endif // GPIOBUTTON_H
