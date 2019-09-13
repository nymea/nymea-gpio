/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea-gpio.                                       *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \class GpioMonitor
    \brief Monitor for GPIO interrupts.
    \inmodule nymea-gpio

    This class allows to monitor an input GPIO for the interrupts depending on the \l{Gpio:Edge} configuration.

    \code
        GpioMonitor *monitor = new GpioMonitor(112, this);

        if (!monitor->enable()) {
            qWarning() << "Could not enable GPIO monitor";
            monitor->deleteLater();
            return;
        }

        connect(monitor, &GpioMonitor::interruptOccured, this, [this, monitor](bool value){
            qDebug() << "GPIO value changed" << value;
        });

    \endcode

*/

/*! \fn void GpioMonitor::interruptOccured(bool value);
    This signal will be emitted, if an interrupt on the monitored Gpio occured with the new \a value. This event depends on the Gpio::Edge configuration of the Gpio.

    \sa edge(), setEdge()
*/

/*! \fn void GpioMonitor::enabledChanged(bool enabled);
    This signal will be emitted when the GpioMonitor \a enabled changed.
*/

#include "gpiomonitor.h"

#include <poll.h>
#include <QMutexLocker>

/*! Constructs a \l{GpioMonitor} object with the given \a gpio number and \a parent. */
GpioMonitor::GpioMonitor(int gpio, QObject *parent) :
    QThread(parent),
    m_gpioNumber(gpio)
{
    // Inform about the thread status
    connect(this, &GpioMonitor::started, this, &GpioMonitor::onThreadStarted, Qt::DirectConnection);
    connect(this, &GpioMonitor::finished, this, &GpioMonitor::onThreadFinished, Qt::DirectConnection);
}

/*! Destroys and unexports the Gpio. */
GpioMonitor::~GpioMonitor()
{
    disable();
    wait(200);
}

/*! Returns the edge interrupt configuration for this GpioMonitor.

  \sa Gpio::Edge

*/
Gpio::Edge GpioMonitor::edge() const
{
    return m_edge;
}

/*! Sets the edge interrupt configuration for this GpioMonitor to the given \a edge.

  \sa Gpio::Edge

*/
void GpioMonitor::setEdge(Gpio::Edge edge)
{
    if (m_edge == edge)
        return;

    m_edge = edge;
}

/*! Returns true, if the monitor is configured as active low. If active low is true, the GPIO values and interrupt behavior will be inverted. */
bool GpioMonitor::activeLow() const
{
    return m_activeLow;
}

/*! Sets the the monitor to \a activeLow. If active low is true, the GPIO values and interrupt behavior will be inverted. */
void GpioMonitor::setActiveLow(bool activeLow)
{
    if (m_activeLow == activeLow)
        return;

    m_activeLow = activeLow;
}

/*! Returns the current value of the Gpio. */
Gpio::Value GpioMonitor::value()
{
    QMutexLocker valueLocker(&m_valueMutex);
    return m_value;
}

/*! Returns true if this GpioMonitor is enabled. */
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

/*! Reimplementation of the QThread run() method. Within the thread the Gpio value will be polled using poll() 2. */
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

/*! Returns true, if this GpioMonitor was enabled successfully. */
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

/*! Disables this GpioMonitor. The \l{interruptOccured()} signal will not be emitted any more and the Gpio will be unexported. */
void GpioMonitor::disable()
{
    qCDebug(dcGpio()) << "Disable gpio monitor";
    // Stop the thread if not already disabled
    QMutexLocker locker(&m_stopMutex);
    if (m_stop) return;
    m_stop = true;
}
