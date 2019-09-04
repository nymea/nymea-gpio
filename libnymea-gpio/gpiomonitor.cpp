#include "gpiomonitor.h"

#include <QMutexLocker>

GpioMonitor::GpioMonitor(int gpio, Gpio::Edge edge, QObject *parent) :
    QThread(parent),
    m_edge(edge)
{
    m_gpio = new Gpio(gpio, this);
    connect(this, &GpioMonitor::started, this, &GpioMonitor::onThreadStarted);
    connect(this, &GpioMonitor::finished, this, &GpioMonitor::onThreadFinished);
}

GpioMonitor::~GpioMonitor()
{
    disable();
    wait();
}

Gpio::Value GpioMonitor::value()
{
    QMutexLocker valueLocker(&m_valueMutex);
    return m_value;
}

bool GpioMonitor::enabled() const
{
    return m_enabled;
}

void GpioMonitor::setValue(Gpio::Value value)
{
    QMutexLocker valueLocker(&m_valueMutex);
    if (m_value == value)
        return;

    m_value = value;
    emit valueChanged(m_value);
}

void GpioMonitor::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

void GpioMonitor::run()
{
    // Initialize the current value
    setValue(m_gpio->value());

    // Poll the GPIO value until the stop is true
    while (true) {

        // Note: the setValue() method takes care about the mutex locking
        setValue(m_gpio->value());

        // Check if we should stop the thread
        QMutexLocker stopLocker(&m_stopMutex);
        if (m_stop) break;
        msleep(50);
    }
}

void GpioMonitor::onThreadStarted()
{
    setEnabled(true);
}

void GpioMonitor::onThreadFinished()
{
    setEnabled(false);
}

bool GpioMonitor::enable()
{
    if (isRunning()) {
        qCWarning(dcGpio()) << "This GPIO monitor is already running.";
        return true;
    }

    // Init the GPIO
    if (!m_gpio->isAvailable()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor.";
        return false;
    }

    if (!m_gpio->exportGpio()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor.";
        return false;
    }

    if (!m_gpio->setDirection(Gpio::DirectionInput)) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor.";
        return false;
    }

    if (!m_gpio->setEdgeInterrupt(m_edge)) {
        qCWarning(dcGpio()) << "Could not set interrupt for the GPIO monitor.";
        return false;
    }

    QMutexLocker locker(&m_stopMutex);
    m_stop = false;

    // Everything went fine, lets start the poll thread and inform about the result
    start();
    return true;
}

void GpioMonitor::disable()
{
    // Stop the thread if not already disabled
    QMutexLocker locker(&m_stopMutex);
    if (m_stop) return;
    m_stop = true;
}
