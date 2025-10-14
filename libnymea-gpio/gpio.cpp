/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

#include <QFile>
#include <QTextStream>

#include <gpiod.h>

#include <cstring>

Q_LOGGING_CATEGORY(dcGpio, "Gpio")

static QString gpioDirectoryPath(int gpio)
{
    return QString("/sys/class/gpio/gpio%1").arg(gpio);
}

/*! Constructs a Gpio object to represent a GPIO with the given \a gpio number and \a parent. */
Gpio::Gpio(int gpio, QObject *parent) :
    QObject(parent),
    m_gpio(gpio),
    m_gpioDirectory(QDir(gpioDirectoryPath(gpio)))
{
    qRegisterMetaType<Gpio::Value>();
}

/*! Destroys and unexports the Gpio. */
Gpio::~Gpio()
{
    unexportGpio();
}

/*! Returns true if at least one gpiochip device is available. */
bool Gpio::isAvailable()
{
    return QFile::exists("/dev/gpiochip0");
}

/*! Returns the directory \tt {/sys/class/gpio/gpio<number>} of this Gpio. */
QString Gpio::gpioDirectory() const
{
    return m_gpioDirectory.path();
}

/*! Returns the number of this Gpio.
    \note The Gpio number is mostly not equivalent with the pin number.
*/
int Gpio::gpioNumber() const
{
    return m_gpio;
}

bool Gpio::lookupChip(int gpioNumber, QString &devicePath, int &lineOffset)
{
    QDir gpioClass("/sys/class/gpio");
    const QStringList entries = gpioClass.entryList(QStringList() << "gpiochip*", QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QFile baseFile(gpioClass.filePath(entry + "/base"));
        QFile ngpioFile(gpioClass.filePath(entry + "/ngpio"));
        if (!baseFile.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        if (!ngpioFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            baseFile.close();
            continue;
        }

        QTextStream baseStream(&baseFile);
        QTextStream ngpioStream(&ngpioFile);
        int base = -1;
        int ngpio = 0;
        baseStream >> base;
        ngpioStream >> ngpio;
        baseFile.close();
        ngpioFile.close();

        if (base < 0 || ngpio <= 0)
            continue;

        if (gpioNumber >= base && gpioNumber < base + ngpio) {
            bool ok;
            const int chipIndex = entry.mid(QString("gpiochip").size()).toInt(&ok);
            if (!ok)
                return false;
            devicePath = QString("/dev/gpiochip%1").arg(chipIndex);
            lineOffset = gpioNumber - base;
            return true;
        }
    }

    qCWarning(dcGpio()) << "Could not find gpiochip for gpio" << gpioNumber;
    return false;
}

bool Gpio::ensureLine()
{
    if (m_line)
        return true;

    if (!m_chipDevice.isEmpty() && m_lineOffset >= 0) {
        m_chip = gpiod_chip_open(m_chipDevice.toLocal8Bit().constData());
        if (!m_chip) {
            qCWarning(dcGpio()) << "Failed to open gpiochip" << m_chipDevice;
            return false;
        }
        m_line = gpiod_chip_get_line(m_chip, m_lineOffset);
        if (!m_line) {
            qCWarning(dcGpio()) << "Failed to get line" << m_lineOffset << "from chip" << m_chipDevice;
            gpiod_chip_close(m_chip);
            m_chip = nullptr;
            return false;
        }
        return true;
    }

    QString devicePath;
    int offset = -1;
    if (!lookupChip(m_gpio, devicePath, offset))
        return false;

    m_chipDevice = devicePath;
    m_lineOffset = offset;

    m_chip = gpiod_chip_open(devicePath.toLocal8Bit().constData());
    if (!m_chip) {
        qCWarning(dcGpio()) << "Failed to open gpiochip" << devicePath;
        return false;
    }

    m_line = gpiod_chip_get_line(m_chip, offset);
    if (!m_line) {
        qCWarning(dcGpio()) << "Failed to get line" << offset << "from chip" << devicePath;
        gpiod_chip_close(m_chip);
        m_chip = nullptr;
        return false;
    }

    return true;
}

bool Gpio::exportGpio()
{
    qCDebug(dcGpio()) << "Export GPIO" << m_gpio;
    return ensureLine();
}

void Gpio::releaseLine()
{
    if (m_line) {
        if (gpiod_line_is_requested(m_line))
            gpiod_line_release(m_line);
        m_line = nullptr;
    }

    if (m_chip) {
        gpiod_chip_close(m_chip);
        m_chip = nullptr;
    }
}

/*! Returns true if this Gpio could be unexported. */
bool Gpio::unexportGpio()
{
    qCDebug(dcGpio()) << "Unexport GPIO" << m_gpio;

    releaseLine();
    m_chipDevice.clear();
    m_lineOffset = -1;
    m_direction = DirectionInvalid;
    m_edge = EdgeNone;

    return true;
}

bool Gpio::requestLine(Gpio::Direction direction, Gpio::Edge edge)
{
    if (!ensureLine())
        return false;

    if (gpiod_line_is_requested(m_line))
        gpiod_line_release(m_line);

    struct gpiod_line_request_config config;
    memset(&config, 0, sizeof(config));
    config.consumer = "nymea-gpio";
    config.flags = 0;
    if (m_activeLow)
        config.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

    int initialValue = (m_value == ValueHigh) ? 1 : 0;

    switch (direction) {
    case DirectionInput:
        switch (edge) {
        case EdgeRising:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
            break;
        case EdgeFalling:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
            break;
        case EdgeBoth:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
            break;
        case EdgeNone:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
            break;
        }
        if (gpiod_line_request(m_line, &config, 0) < 0) {
            qCWarning(dcGpio()) << "Failed to request line" << m_gpio << "as input";
            return false;
        }
        break;
    case DirectionOutput:
        if (edge != EdgeNone) {
            qCWarning(dcGpio()) << "Output GPIO cannot have edge interrupts";
            return false;
        }
        config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
        if (gpiod_line_request(m_line, &config, initialValue) < 0) {
            qCWarning(dcGpio()) << "Failed to request line" << m_gpio << "as output";
            return false;
        }
        break;
    default:
        return false;
    }

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

    if (!requestLine(direction, m_edge))
        return false;

    m_direction = direction;
    return true;
}

/*! Returns the direction of this Gpio. */
Gpio::Direction Gpio::direction()
{
    return m_direction;
}

/*! Returns true if the digital \a value of this Gpio could be set correctly. */
bool Gpio::setValue(Gpio::Value value)
{
    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "value" << value;

    if (value == Gpio::ValueInvalid) {
        qCWarning(dcGpio()) << "Setting an invalid value is forbidden.";
        return false;
    }

    if (m_direction != Gpio::DirectionOutput) {
        qCWarning(dcGpio()) << "Setting the value of a non-output GPIO is forbidden.";
        return false;
    }

    if (!ensureLine())
        return false;

    if (gpiod_line_set_value(m_line, value == ValueHigh ? 1 : 0) < 0) {
        qCWarning(dcGpio()) << "Failed to set GPIO" << m_gpio << "value";
        return false;
    }

    m_value = value;
    return true;
}

/*! Returns the current digital value of this Gpio. */
Gpio::Value Gpio::value()
{
    if (!ensureLine())
        return Gpio::ValueInvalid;

    int ret = gpiod_line_get_value(m_line);
    if (ret < 0) {
        qCWarning(dcGpio()) << "Failed to read GPIO" << m_gpio << "value";
        return Gpio::ValueInvalid;
    }

    return ret == 0 ? Gpio::ValueLow : Gpio::ValueHigh;
}

/*! This method allows to invert the logic of this Gpio. Returns true, if the GPIO could be set \a activeLow. */
bool Gpio::setActiveLow(bool activeLow)
{
    qCDebug(dcGpio()) << "Set GPIO" << m_gpio << "active low" << activeLow;

    if (m_activeLow == activeLow)
        return true;

    m_activeLow = activeLow;
    if (m_direction == DirectionInvalid)
        return true;

    return requestLine(m_direction, m_edge);
}

/*! Returns true if the logic of this Gpio is inverted (1 = low, 0 = high). */
bool Gpio::activeLow()
{
    return m_activeLow;
}

/*! Returns true if the \a edge of this GPIO could be set correctly. The \a edge parameter specifies, when an interrupt occurs. */
bool Gpio::setEdgeInterrupt(Gpio::Edge edge)
{
    if (m_direction == Gpio::DirectionOutput) {
        qCWarning(dcGpio()) << "Could not set edge interrupt, GPIO is configured as an output.";
        return false;
    }

    if (m_direction == DirectionInvalid) {
        qCWarning(dcGpio()) << "Direction must be set before configuring edge interrupts.";
        return false;
    }

    if (!requestLine(m_direction, edge))
        return false;

    m_edge = edge;
    return true;
}

/*! Returns the edge interrupt of this Gpio. */
Gpio::Edge Gpio::edgeInterrupt()
{
    return m_edge;
}

int Gpio::fileDescriptor() const
{
    if (!m_line || m_edge == EdgeNone)
        return -1;

    return gpiod_line_event_get_fd(m_line);
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
