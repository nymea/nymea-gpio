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

#ifndef GPIO_H
#define GPIO_H

#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(dcGpio)

class GpioMonitor;

struct gpiod_chip;
struct gpiod_line;

class Gpio : public QObject
{
    Q_OBJECT

    friend class GpioMonitor;

public:
    enum Direction {
        DirectionInvalid,
        DirectionInput,
        DirectionOutput
    };
    Q_ENUM(Direction)

    enum Value {
        ValueInvalid = -1,
        ValueLow = 0,
        ValueHigh = 1
    };
    Q_ENUM(Value)

    enum Edge {
        EdgeFalling,
        EdgeRising,
        EdgeBoth,
        EdgeNone
    };
    Q_ENUM(Edge)

    explicit Gpio(int gpio, QObject *parent = nullptr);
    ~Gpio();

    static bool isAvailable();

    QString gpioDirectory() const;
    int gpioNumber() const;

    bool exportGpio();
    bool unexportGpio();

    bool setDirection(Gpio::Direction direction);
    Gpio::Direction direction();

    bool setValue(Gpio::Value value);
    Gpio::Value value();

    bool setActiveLow(bool activeLow);
    bool activeLow();

    bool setEdgeInterrupt(Gpio::Edge edge);
    Gpio::Edge edgeInterrupt();

private:
#ifndef NYMEA_GPIO_USE_SYSFS
    bool resolveLine();
    bool requestLine(Gpio::Direction direction, Gpio::Edge edge, int outputValue);
    int logicalToPhysicalValue(Gpio::Value value) const;
    Gpio::Value physicalToLogicalValue(int value) const;
    int eventFd() const;

    QString m_chipName;
    unsigned int m_lineOffset = 0;
    gpiod_chip *m_chip = nullptr;
    gpiod_line *m_line = nullptr;
    bool m_activeLow = false;
    Gpio::Edge m_edge = Gpio::EdgeNone;
#endif
    int m_gpio = 0;
    Gpio::Direction m_direction = Gpio::DirectionOutput;
#ifdef NYMEA_GPIO_USE_SYSFS
    QDir m_gpioDirectory;
#endif
};

QDebug operator<<(QDebug debug, Gpio *gpio);

#endif // GPIO_H
