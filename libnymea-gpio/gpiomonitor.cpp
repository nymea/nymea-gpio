#include "gpiomonitor.h"

#include <poll.h>
#include <QMutexLocker>

GpioMonitor::GpioMonitor(int gpio, QObject *parent) :
    QThread(parent),
    m_gpioNumber(gpio)
{
    // Inform about the thread status
    connect(this, &GpioMonitor::started, this, &GpioMonitor::onThreadStarted, Qt::DirectConnection);
    connect(this, &GpioMonitor::finished, this, &GpioMonitor::onThreadFinished, Qt::DirectConnection);
}

GpioMonitor::~GpioMonitor()
{
    disable();
    wait(200);
}

Gpio::Edge GpioMonitor::edge() const
{
    return m_edge;
}

void GpioMonitor::setEdge(Gpio::Edge edge)
{
    if (m_edge == edge)
        return;

    m_edge = edge;
}

bool GpioMonitor::activeLow() const
{
    return m_activeLow;
}

void GpioMonitor::setActiveLow(bool activeLow)
{
    if (m_activeLow == activeLow)
        return;

    m_activeLow = activeLow;
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
    m_value = value;

    switch (m_value) {
    case Gpio::ValueLow:
        emit interruptOccured(false);
        break;
    case Gpio::ValueHigh:
        emit interruptOccured(true);
        break;
    default:
        break;
    }
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
    // Create GPIO in the thread for initialisation
    Gpio *inputGpio = new Gpio(m_gpioNumber);
    if (!inputGpio->exportGpio()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor.";
        delete inputGpio;
        return;
    }

    if (!inputGpio->setDirection(Gpio::DirectionInput)) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor.";
        delete inputGpio;
        return;
    }

    if (!inputGpio->setEdgeInterrupt(m_edge)) {
        qCWarning(dcGpio()) << "Could not set interrupt for the GPIO monitor.";
        delete inputGpio;
        return;
    }

    if (!inputGpio->setActiveLow(m_activeLow)) {
        qCWarning(dcGpio()) << "Could not set active low for the GPIO monitor.";
        delete inputGpio;
        return;
    }


    // In order to do correctly, use poll (2) according to the kernel documentation
    // https://www.kernel.org/doc/Documentation/gpio/sysfs.txt
    QFile valueFile(inputGpio->gpioDirectory() + QDir::separator() + "value");
    if (!valueFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << inputGpio << "value file:" << valueFile.errorString();
        delete inputGpio;
        return;
    }

    struct pollfd fdset[1];
    int rc = -1;
    uint nfds = 1;
    int timeout = 100; // ms
    fdset[0].fd = valueFile.handle();
    fdset[0].events = POLLPRI;

    // Poll the GPIO value until stop is true
    while (true) {
        // Poll the value file
        rc = poll(fdset, nfds, timeout);

        // Poll failed...
        if (rc < 0) {
            qCWarning(dcGpio()) << "Failed to poll" << inputGpio;
            break;
        }

        // Check if we should stop the thread
        QMutexLocker stopLocker(&m_stopMutex);
        if (m_stop) break;

        // No interrupt occured
        if (rc == 0)
            continue;

        // Interrupt occured
        if (fdset[0].revents & POLLPRI) {
            QString valueString;
            QTextStream readStream(&valueFile);
            if (!readStream.seek(0)) {
                qCWarning(dcGpio()) << "Failed to seek value file of" << inputGpio;
                continue;
            }

            // Notify the main thread about the interrupt
            readStream >> valueString;
            if (valueString == "0") {
                setValue(Gpio::ValueLow);
            } else {
                setValue(Gpio::ValueHigh);
            }
        }
    }

    // Clean up once done
    valueFile.close();
    delete inputGpio;
}

void GpioMonitor::onThreadStarted()
{
    qCDebug(dcGpio()) << "Monitor thread started";
    setEnabled(true);
}

void GpioMonitor::onThreadFinished()
{
    qCDebug(dcGpio()) << "Monitor thread finished";
    setEnabled(false);
}

bool GpioMonitor::enable()
{
    qCDebug(dcGpio()) << "Enable gpio monitor";
    if (isRunning()) {
        qCWarning(dcGpio()) << "This GPIO monitor is already running.";
        return true;
    }

    // Init the GPIO
    if (!Gpio::isAvailable()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor. There are no GPIOs available on this platform.";
        return false;
    }

    QMutexLocker locker(&m_stopMutex);
    m_stop = false;

    // Everything looks good, lets start the poll thread and inform about the result
    start();
    return true;
}

void GpioMonitor::disable()
{
    qCDebug(dcGpio()) << "Disable gpio monitor";
    // Stop the thread if not already disabled
    QMutexLocker locker(&m_stopMutex);
    if (m_stop) return;
    m_stop = true;
}
