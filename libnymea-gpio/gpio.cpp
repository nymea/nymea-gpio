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
    \class Gpio
    \brief Represents a system GPIO in linux systems.
    \inmodule nymea-gpio
    \ingroup gpio

    A "General Purpose Input/Output" (GPIO) is a flexible software-controlled
    digital signal. They are provided from many kinds of chip, and are familiar
    to Linux developers working with embedded and custom hardware. Each GPIO
    represents a bit connected to a particular pin, or "ball" on Ball Grid Array
    (BGA) packages. Board schematics show which external hardware connects to
    which GPIOs. Drivers can be written generically, so that board setup code
    passes such pin configuration data to drivers
    (\l{https://www.kernel.org/doc/Documentation/gpio/gpio.txt}{source}).
    General Purpose Input/Output (a.k.a. GPIO) is a generic pin on a chip whose
    behavior (including whether it is an input or output pin) can be controlled
    through this class. An object of of the Gpio class represents a pin.

    \code
        Gpio *gpioOut = new Gpio(23, this);

        // Export Gpio
        if (!gpioOut->exportGpio()) {
            qWarning() << "Could not export Gpio" << gpioOut->gpioNumber();
            gpioOut->deleteLater();
            return;
        }

        // Configure Gpio direction
        if (!gpioOut->setDirection(PiGpio::DirectionOutput)) {
            qWarning() << "Could not set direction of Gpio" << gpioOut->gpioNumber();
            gpioOut->deleteLater();
            return;
        }

        gpioOut->setValue(Gpio::ValueHigh)
    \endcode

    \code
        Gpio *gpioIn = new Gpio(24, this);

        // Export Gpio
        if (!gpioIn->exportGpio()) {
            qWarning() << "Could not export Gpio" << gpioIn->gpioNumber();
            gpioIn->deleteLater();
            return;
        }

        // Configure Gpio direction
        if (!gpioIn->setDirection(PiGpio::DirectionInput)) {
            qWarning() << "Could not set direction of Gpio" << gpioIn->gpioNumber();
            gpioIn->deleteLater();
            return;
        }

        qDebug() << "Current value" << gpioIn->value();
    \endcode
    \sa GpioMonitor
*/


/*!
    \enum Gpio::Direction
    This enum type specifies the dirction a Gpio.

    \value DirectionInput
        The Gpio is configured as \b input.
    \value DirectionOutput
        The Gpio is configured as \b output.
    \value DirectionInvalid
        The direction is not valid.
*/

/*!
    \enum Gpio::Value
    This enum type specifies the value a Gpio.

    \value ValueInvalid
        The value is not valid.
    \value ValueLow
        The Gpio is low.
    \value ValueHigh
        The Gpio is high.

*/

/*!
    \enum Gpio::Edge
    This enum type specifies the edge interrupt type of a Gpio.

    \value EdgeFalling
        The Gpio reacts on falling edge interrupt.
    \value EdgeRising
        The Gpio reacts on rising edge interrupt.
    \value EdgeBoth
        The Gpio reacts on both, rising and falling edge interrupt.
    \value EdgeNone
        The Gpio does not react on interrupts.
*/

#include "gpio.h"

Q_LOGGING_CATEGORY(dcGpio, "Gpio")

/*! Constructs a Gpio object to represent a GPIO with the given \a gpio number and \a parent. */
Gpio::Gpio(int gpio, QObject *parent) :
    QObject(parent),
    m_gpio(gpio),
    m_direction(Gpio::DirectionInvalid),
    m_gpioDirectory(QDir(QString("/sys/class/gpio/gpio%1").arg(QString::number(gpio))))
{
    qRegisterMetaType<Gpio::Value>();

}

/*! Destroys and unexports the Gpio. */
Gpio::~Gpio()
{
    unexportGpio();
}

/*! Returns true if the directories \tt {/sys/class/gpio} and \tt {/sys/class/gpio/export} do exist. */
bool Gpio::isAvailable()
{
    return QFile("/sys/class/gpio/export").exists();
}

/*! Returns the directory \tt {/sys/class/gpio/gpio<number>} of this Gpio. */
QString Gpio::gpioDirectory() const
{
    return m_gpioDirectory.canonicalPath();
}

/*! Returns the number of this Gpio.
    \note The Gpio number is mostly not equivalent with the pin number.
*/
int Gpio::gpioNumber() const
{
    return m_gpio;
}

/*! Returns true if this Gpio could be exported in the system file \tt {/sys/class/gpio/export}. If this Gpio is already exported, this function will return true. */
bool Gpio::exportGpio()
{
    qCDebug(dcGpio()) << "Export GPIO" << m_gpio;
    // Check if already exported
    if (m_gpioDirectory.exists()) {
        qCDebug(dcGpio()) << "GPIO" << m_gpio << "already exported.";
        return true;
    }

    QFile exportFile("/sys/class/gpio/export");
    if (!exportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO export file:" << exportFile.errorString();
        return false;
    }

    QTextStream out(&exportFile);
    out << m_gpio;
    exportFile.close();
    return true;
}

/*! Returns true if this Gpio could be unexported in the system file \tt {/sys/class/gpio/unexport}. */
bool Gpio::unexportGpio()
{
    qCDebug(dcGpio()) << "Unexport GPIO" << m_gpio;

    QFile unexportFile("/sys/class/gpio/unexport");
    if (!unexportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO unexport file:" << unexportFile.errorString();
        return false;
    }

    QTextStream out(&unexportFile);
    out << m_gpio;
    unexportFile.close();
    return true;
}

/*! Returns true if the \a direction of this GPIO could be set. \sa Gpio::Direction, */
bool Gpio::setDirection(Gpio::Direction direction)
{
    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "direction" << direction;
    if (direction == Gpio::DirectionInvalid) {
        qCWarning(dcGpio()) << "Setting an invalid direction is forbidden.";
        return false;
    }

    QFile directionFile(m_gpioDirectory.path() + QDir::separator() + "direction");
    if (!directionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "direction file:" << directionFile.errorString();
        return false;
    }

    m_direction = direction;

    QTextStream out(&directionFile);
    switch (m_direction) {
    case DirectionInput:
        out << "in";
        break;
    case DirectionOutput:
        out << "out";
        break;
    default:
        break;
    }

    directionFile.close();
    return true;
}

/*! Returns the direction of this Gpio. */
Gpio::Direction Gpio::direction()
{
    QFile directionFile(m_gpioDirectory.path() + QDir::separator() + "direction");
    if (!directionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "direction file:" << directionFile.fileName() << directionFile.errorString();
        return Gpio::DirectionInvalid;
    }

    QString direction;
    QTextStream in(&directionFile);
    in >> direction;
    directionFile.close();

    if (direction == "in") {
        m_direction = DirectionInput;
        return Gpio::DirectionInput;
    } else if (direction == "out") {
        m_direction = DirectionOutput;
        return Gpio::DirectionOutput;
    }

    return Gpio::DirectionInvalid;
}

/*! Returns true if the digital \a value of this Gpio could be set correctly. */
bool Gpio::setValue(Gpio::Value value)
{
    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "value" << value;

    // Check given value
    if (value == Gpio::ValueInvalid) {
        qCWarning(dcGpio()) << "Setting an invalid value is forbidden.";
        return false;
    }

    // Check current direction
    if (m_direction == Gpio::DirectionInput) {
        qCWarning(dcGpio()) << "Setting the value of an input GPIO is forbidden.";
        return false;
    }

    if (m_direction == Gpio::DirectionInvalid) {
        qCWarning(dcGpio()) << "The direction of GPIO" << m_gpio << "is invalid.";
        return false;
    }

    QFile valueFile(m_gpioDirectory.path() + QDir::separator() + "value");
    if (!valueFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "value file:" << valueFile.errorString();
        return false;
    }

    QTextStream out(&valueFile);
    switch (value) {
    case ValueLow:
        out << "0";
        break;
    case ValueHigh:
        out << "1";
        break;
    default:
        valueFile.close();
        return false;
    }

    valueFile.close();
    return true;
}

/*! Returns the current digital value of this Gpio. */
Gpio::Value Gpio::value()
{
    QFile valueFile(m_gpioDirectory.path() + QDir::separator() + "value");
    if (!valueFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "value file:" << valueFile.errorString();
        return Gpio::ValueInvalid;
    }

    QString value;
    QTextStream in(&valueFile);
    in >> value;
    valueFile.close();

    if (value == "0") {
        return Gpio::ValueLow;
    } else if (value == "1") {
        return Gpio::ValueHigh;
    }

    return Gpio::ValueInvalid;
}

/*! This method allows to invert the logic of this Gpio. Returns true, if the GPIO could be set \a activeLow. */
bool Gpio::setActiveLow(bool activeLow)
{
    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "active low" << activeLow;

    QFile activeLowFile(m_gpioDirectory.path() + QDir::separator() + "active_low");
    if (!activeLowFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "active_low file:" << activeLowFile.errorString();
        return false;
    }

    QTextStream out(&activeLowFile);
    if (activeLow) {
        out << "1";
    } else {
        out << "0";
    }

    activeLowFile.close();
    return true;
}

/*! Returns true if the logic of this Gpio is inverted (1 = low, 0 = high). */
bool Gpio::activeLow()
{
    QFile activeLowFile(m_gpioDirectory.path() + QDir::separator() + "active_low");
    if (!activeLowFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "active_low file:" << activeLowFile.errorString();
        return false;
    }

    QString value;
    QTextStream in(&activeLowFile);
    in >> value;
    activeLowFile.close();

    if (value == "1")
        return true;

    return false;
}

/*! Returns true if the \a edge of this GPIO could be set correctly. The \a edge parameter specifies, when an interrupt occurs. */
bool Gpio::setEdgeInterrupt(Gpio::Edge edge)
{

    if (m_direction == Gpio::DirectionOutput) {
        qCWarning(dcGpio()) << "Could not set edge interrupt, GPIO is configured as an output.";
        return false;
    }

    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "edge interrupt" << edge;
    QFile edgeFile(m_gpioDirectory.path() + QDir::separator() + "edge");
    if (!edgeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "edge file:" << edgeFile.errorString();
        return false;
    }

    QTextStream out(&edgeFile);
    switch (edge) {
    case EdgeFalling:
        out << "falling";
        break;
    case EdgeRising:
        out << "rising";
        break;
    case EdgeBoth:
        out << "both";
        break;
    case EdgeNone:
        out << "none";
        break;
    }

    edgeFile.close();
    return true;
}

/*! Returns the edge interrupt of this Gpio. */
Gpio::Edge Gpio::edgeInterrupt()
{
    QFile edgeFile(m_gpioDirectory.path() + QDir::separator() + "edge");
    if (!edgeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcGpio()) << "Could not open GPIO" << m_gpio << "edge file:" << edgeFile.errorString();
        return Gpio::EdgeNone;
    }

    QString edge;
    QTextStream in(&edgeFile);
    in >> edge;
    edgeFile.close();

    if (edge.contains("falling")) {
        return Gpio::EdgeFalling;
    } else if (edge.contains("rising")) {
        return Gpio::EdgeRising;
    } else if (edge.contains("both")) {
        return Gpio::EdgeBoth;
    } else if (edge.contains("none")) {
        return Gpio::EdgeNone;
    }

    return Gpio::EdgeNone;
}


/*! Prints the given \a gpio to \a debug. */
QDebug operator<<(QDebug debug, Gpio *gpio)
{
    debug.nospace() << "Gpio(" << gpio->gpioNumber() << ", ";
    if (gpio->direction() == Gpio::DirectionInput) {
        debug.nospace() << "input, ";

        switch (gpio->edgeInterrupt()) {
        case Gpio::EdgeFalling:
            debug.nospace() << "edge: falling, ";
            break;
        case Gpio::EdgeRising:
            debug.nospace() << "edge: rising, ";
            break;
        case Gpio::EdgeBoth:
            debug.nospace() << "edge: both, ";
            break;
        case Gpio::EdgeNone:
            debug.nospace() << "edge: none, ";
            break;
        }
    } else if (gpio->direction() == Gpio::DirectionOutput) {
        debug.nospace() << "output, ";
    } else {
        debug.nospace() << "invalid, ";
    }

    if (gpio->activeLow()) {
        debug.nospace() << "active low: 1, ";
    } else {
        debug.nospace() << "active low: 0, ";
    }

    if (gpio->value() == Gpio::ValueHigh) {
        debug.nospace() << "value: 1";
    } else if (gpio->value() == Gpio::ValueLow) {
        debug.nospace() << "value: 0";
    } else {
        debug.nospace() << "value: invalid";
    }

    debug.nospace() << ")";

    return debug.space();
}
