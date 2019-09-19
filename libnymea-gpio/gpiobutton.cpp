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
    \class GpioButton
    \brief Represents a GPIO Button with some helper methods.
    \inmodule nymea-gpio
    \ingroup gpio

    This class represents a Button based on a GPIO. The class takes care about the \l{clicked()} signal handling, debounces the GPIO signal
    and offers a nice interface for \l{longPressed()} behaviour.

    In order to get the button signals, the button has to be enabled using \l{enable()}.

    \code
        GpioButton *button = new GpioButton(15, this);
        button->setName("User button");
        if (!button->enable()) {
            qWarning() << "Could not enable the" << this;
            button->deleteLater();
            return;
        }

        connect(button, &GpioButton::clicked, this, [this, button](){
            qDebug() << button << "clicked";
        });

    \endcode

*/

/*!
    \fn void GpioButton::clicked();
    This signal will be emitted when the button gets clicked. A button will has been clicked, if it was pressed at leased for 10 ms and at most 500 ms.
*/

/*!
    \fn void GpioButton::pressed();
    This signal will be emitted when the button gets pressed.
*/

/*!
    \fn void GpioButton::released();
    This signal will be emitted whenever the button gets released.
*/

/*!
    \fn void GpioButton::longPressed();
    This signal will be emitted whenever the button gets pressed for a certain time.

    \sa longPressedTimeout(), repeateLongPressed()
*/

#include "gpiobutton.h"
#include "gpiomonitor.h"

/*! Constructs a \l{GpioButton} object with the given \a gpio number and \a parent. */
GpioButton::GpioButton(int gpio, QObject *parent) :
    QObject(parent),
    m_gpioNumber(gpio)
{

}

/*! Returns the gpio number for this GpioButton. */
int GpioButton::gpioNumber() const
{
    return m_gpioNumber;
}

/*! Returns \c true the gpio button is configured as active low.

  \sa Gpio::activeLow()
*/
bool GpioButton::activeLow() const
{
    return m_activeLow;
}

/*! Sets the gpio button active low configuration to \a activeLow for this GpioButton.

  \sa Gpio::setActiveLow()
*/
void GpioButton::setActiveLow(bool activeLow)
{
    m_activeLow = activeLow;
}

/*! Returns \c true, if the \l{longPressed()} signal will be emited again if the button will be hold down. */
bool GpioButton::repeateLongPressed() const
{
    return m_repeateLongPressed;
}

/*! Sets repeate long pressed configuration to \a repeateLongPressed. If \a repeateLongPressed is true, the longPressed() signal will be repeated as long the button will be hold down.

    \sa longPressedTimeout()
*/
void GpioButton::setRepeateLongPressed(bool repeateLongPressed)
{
    m_repeateLongPressed = repeateLongPressed;
}

/*! Returns the long pressed timout duration in milliseconds. If the button gets hold down for this duration, the longPressed() signal will be emitted.

  \sa longPressed()
*/
int GpioButton::longPressedTimeout() const
{
    return m_longPressedTimeout;
}

/*! Sets the long pressed timout duration to \a longPressedTimeout in milliseconds. If the button gets hold down for this duration, the longPressed() signal will be emitted.

  \sa longPressed()
*/
void GpioButton::setLongPressedTimeout(int longPressedTimeout)
{
    m_longPressedTimeout = longPressedTimeout;
}

/*! Returns the \c name for this GpioButton. This is optional, but will be printed in the debug operator. */
QString GpioButton::name() const
{
    return m_name;
}

/*! Sets the \a name for this GpioButton. This is optional, but will be printed in the debug operator. */
void GpioButton::setName(const QString &name)
{
    m_name = name;
}

void GpioButton::onTimeout()
{
    qCDebug(dcGpio()) << this << "long pressed";
    emit longPressed();
}

void GpioButton::onInterruptOccured(bool value)
{
    if (value) {
        // Pressed
        qCDebug(dcGpio()) << this << "pressed";
        emit pressed();

        m_timer->setSingleShot(!m_repeateLongPressed);
        m_timer->start(m_longPressedTimeout);
        m_time.restart();
    } else {
        // Released
        qCDebug(dcGpio()) << this << "released";
        emit released();

        m_timer->stop();
        int duration = m_time.elapsed();

        // Debounce and limit to 500 ms
        if (duration >= 10 && duration <= 500) {
            qCDebug(dcGpio()) << this << "clicked";
            emit clicked();
        }
    }
}

/*! Returns \c true, if this GpioButton was enabled successfully. */
bool GpioButton::enable()
{
    // Make sure we have a clean start
    disable();

    m_monitor = new GpioMonitor(m_gpioNumber, this);
    m_monitor->setEdge(Gpio::EdgeBoth);
    m_monitor->setActiveLow(m_activeLow);

    if (!m_monitor->enable()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor for" << this;
        delete m_monitor;
        m_monitor = nullptr;
        return false;
    }
    connect(m_monitor, &GpioMonitor::interruptOccured, this, &GpioButton::onInterruptOccured);

    // Setup timer, if this timer reaches timeout, a long pressed happend
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setSingleShot(!m_repeateLongPressed);
    m_timer->setInterval(m_longPressedTimeout);
    connect(m_timer, &QTimer::timeout, this, &GpioButton::onTimeout);
    return true;
}

/*! Disable this GpioButton. The Gpio will be unexported. */
void GpioButton::disable()
{
    if (m_monitor) {
        delete m_monitor;
        m_monitor = nullptr;
    }

    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
}

/*! Prints the given \a gpioButton to \a debug. */
QDebug operator<<(QDebug debug, GpioButton *gpioButton)
{
    debug.nospace() << "GpioButton(" << gpioButton->gpioNumber() << ", ";
    debug.nospace() << "name: " << gpioButton->name() << ")";
    return debug.space();
}
