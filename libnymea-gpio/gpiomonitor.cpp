// SPDX-License-Identifier: LGPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* nymea-gpio
* GPIO library for nymea
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-gpio.
*
* nymea-gpio is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation, either version 3
* of the License, or (at your option) any later version.
*
* nymea-gpio is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with nymea-gpio. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \class GpioMonitor
    \brief The GpioMonitor class allows to monitor GPIOs.
    \ingroup hardware
    \inmodule libnymea
    An instance of this class creates a thread, which monitors each of the added GPIOs. The object
    emits a signal if one of the added GPIOs changes its value. The GpioMonitor configures a GPIO as an
    input, with the edge interrupt EDGE_BOTH (\l{Gpio::setEdgeInterrupt()}{setEdgeInterrupt}).
    \chapter Example
    Following example shows how to use the GpioMonitor class for a button on the Raspberry Pi. There are two possibilitys
    to connect a button. Following picture shows the schematics:
    \image Raspberry_Pi_Button_Example.png "Raspberry Pi button example"
    Button A represents a clean solutin with a 10 k\unicode{0x2126} resistor (set up as activeLow = false).
    Button B represents a "dirty" solution, were the 3.3V will be directly connected to the GPIO if the button is pressed (set activeLow = true).
    Here is the code example for a button class:
    \code
          Button::Button(QObject *parent) :
              QObject(parent)
          {
              m_button  = new GpioMonitor(110, this);
              connect(m_button, &GpioMonitor::valueChanged, this, &Button::stateChanged);
          }
          bool Button::init()
          {
              return m_button->enable();
          }
          void Button::stateChanged(const bool &value)
          {
              if (m_pressed != value) {
                  m_pressed = value;
                  if (value) {
                      emit buttonPressed();
                  } else {
                      emit buttonReleased();
                  }
              }
          }
    \endcode
*/

/*! \fn void GpioMonitor::valueChanged(const bool &value);
 *  This signal will be emitted, if the monitored \l{Gpio}{Gpios} changed his \a value. */

#include "gpiomonitor.h"

#ifndef NYMEA_GPIO_USE_SYSFS
#include <errno.h>
#include <gpiod.h>
#include <string.h>
#endif

/*! Constructs a \l{GpioMonitor} object with the given \a gpio number and \a parent. */
GpioMonitor::GpioMonitor(int gpio, QObject *parent)
    : QObject(parent)
    , m_gpioNumber(gpio)
{
#ifdef NYMEA_GPIO_USE_SYSFS
    m_valueFile.setFileName("/sys/class/gpio/gpio" + QString::number(m_gpioNumber) + "/value");
#endif
}

/*! Returns true if this \l{GpioMonitor} could be enabled successfully. With the \a activeLow parameter the values can be inverted.
    With the \a edgeInterrupt parameter the interrupt type can be specified. */
bool GpioMonitor::enable(bool activeLow, Gpio::Edge edgeInterrupt)
{
    if (!Gpio::isAvailable())
        return false;

    m_gpio = new Gpio(m_gpioNumber, this);
    if (!m_gpio->exportGpio() || !m_gpio->setDirection(Gpio::DirectionInput) || !m_gpio->setActiveLow(activeLow) || !m_gpio->setEdgeInterrupt(edgeInterrupt)) {
        qCWarning(dcGpio()) << "GpioMonitor: Error while initializing GPIO" << m_gpio->gpioNumber();
        return false;
    }

#ifdef NYMEA_GPIO_USE_SYSFS
    if (!m_valueFile.open(QFile::ReadOnly)) {
        qWarning(dcGpio()) << "GpioMonitor: Could not open value file for gpio monitor" << m_gpio->gpioNumber();
        return false;
    }

    m_notifier = new QSocketNotifier(m_valueFile.handle(), QSocketNotifier::Exception);
    connect(m_notifier, &QSocketNotifier::activated, this, &GpioMonitor::readyReady);
#else
    m_eventFd = m_gpio->eventFd();
    if (m_eventFd < 0) {
        qCWarning(dcGpio()) << "GpioMonitor: Could not get event file descriptor for GPIO" << m_gpio->gpioNumber();
        return false;
    }

#if defined(NYMEA_GPIO_LIBGPIOD_V2)
    m_eventBuffer = gpiod_edge_event_buffer_new(1);
    if (!m_eventBuffer) {
        qCWarning(dcGpio()) << "GpioMonitor: Could not allocate edge event buffer for GPIO" << m_gpio->gpioNumber();
        return false;
    }
#endif

    m_notifier = new QSocketNotifier(m_eventFd, QSocketNotifier::Read);
    connect(m_notifier, &QSocketNotifier::activated, this, &GpioMonitor::readyReady);

    const Gpio::Value initialValue = m_gpio->value();
    if (initialValue == Gpio::ValueInvalid) {
        qCWarning(dcGpio()) << "GpioMonitor: Could not read initial value for GPIO" << m_gpio->gpioNumber();
        return false;
    }
    m_currentValue = (initialValue == Gpio::ValueHigh);
#endif

    qCDebug(dcGpio()) << "Socket notififier started";
    m_notifier->setEnabled(true);
    return true;
}

/*! Disables this \l{GpioMonitor}. */
void GpioMonitor::disable()
{
    delete m_notifier;
    delete m_gpio;

    m_notifier = 0;
    m_gpio = 0;

#ifdef NYMEA_GPIO_USE_SYSFS
    m_valueFile.close();
#else
    m_eventFd = -1;
#if defined(NYMEA_GPIO_LIBGPIOD_V2)
    if (m_eventBuffer) {
        gpiod_edge_event_buffer_free(m_eventBuffer);
        m_eventBuffer = nullptr;
    }
#endif
#endif
}

/*! Returns true if this \l{GpioMonitor} is running. */
bool GpioMonitor::isRunning() const
{
    if (!m_notifier)
        return false;

    return m_notifier->isEnabled();
}

/*! Returns the current value of this \l{GpioMonitor}. */
bool GpioMonitor::value() const
{
    return m_currentValue;
}

/*! Returns the \l{Gpio} of this \l{GpioMonitor}. */
Gpio *GpioMonitor::gpio()
{
    return m_gpio;
}

void GpioMonitor::readyReady(const int &ready)
{
    Q_UNUSED(ready)

#ifdef NYMEA_GPIO_USE_SYSFS
    m_valueFile.seek(0);
    QByteArray data = m_valueFile.readAll();

    bool value = false;
    if (data[0] == '1') {
        value = true;
    } else if (data[0] == '0') {
        value = false;
    } else {
        return;
    }

    m_currentValue = value;
    emit valueChanged(value);
#else
    if (m_eventFd < 0)
        return;

#if defined(NYMEA_GPIO_LIBGPIOD_V2)
    if (!m_gpio || !m_gpio->m_request || !m_eventBuffer)
        return;

    const int eventsRead = gpiod_line_request_read_edge_events(m_gpio->m_request, m_eventBuffer, 1);
    if (eventsRead < 0) {
        qCWarning(dcGpio()) << "GpioMonitor: Could not read GPIO edge events:" << strerror(errno);
        return;
    }
#else
    gpiod_line_event event;
    if (gpiod_line_event_read_fd(m_eventFd, &event) < 0) {
        qCWarning(dcGpio()) << "GpioMonitor: Could not read GPIO event:" << strerror(errno);
        return;
    }
#endif

    const Gpio::Value current = m_gpio ? m_gpio->value() : Gpio::ValueInvalid;
    if (current == Gpio::ValueInvalid)
        return;

    const bool value = current == Gpio::ValueHigh;
    m_currentValue = value;
    emit valueChanged(value);
#endif
}
