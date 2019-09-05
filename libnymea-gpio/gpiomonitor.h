#ifndef GPIOMONITOR_H
#define GPIOMONITOR_H

#include <QMutex>
#include <QThread>
#include <QObject>

#include "gpio.h"

class GpioMonitor : public QThread
{
    Q_OBJECT
public:
    explicit GpioMonitor(int gpio, QObject *parent = nullptr);
    ~GpioMonitor() override;

    Gpio::Edge edge() const;
    void setEdge(Gpio::Edge edge);

    bool activeLow() const;
    void setActiveLow(bool activeLow);

    Gpio::Value value();

    bool enabled() const;

private:
    int m_gpioNumber = -1;
    Gpio::Edge m_edge = Gpio::EdgeBoth;
    bool m_activeLow = true;
    bool m_enabled = false;

    // Thread stuff
    QMutex m_valueMutex;
    Gpio::Value  m_value = Gpio::ValueInvalid;

    QMutex m_stopMutex;
    bool m_stop = false;

    void setValue(Gpio::Value value);
    void setEnabled(bool enabled);

protected:
    void run() override;

signals:
    void interruptOccured(bool value);
    void enabledChanged(bool enabled);

private slots:
    void onThreadStarted();
    void onThreadFinished();

public slots:
    bool enable();
    void disable();

};

#endif // GPIOMONITOR_H
