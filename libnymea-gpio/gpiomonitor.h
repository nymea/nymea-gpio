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

#ifndef GPIOMONITOR_H
#define GPIOMONITOR_H

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QSocketNotifier>

#include "gpio.h"

class GpioMonitor : public QObject
{
    Q_OBJECT

public:
    explicit GpioMonitor(int gpio, QObject *parent = nullptr);

    bool enable(bool activeLow = false, Gpio::Edge edgeInterrupt = Gpio::EdgeBoth);
    void disable();

    bool isRunning() const;
    bool value() const;

    Gpio *gpio();

private:
    int m_gpioNumber;
    Gpio *m_gpio;
    QSocketNotifier *m_notifier;
    QFile m_valueFile;
    bool m_currentValue;

signals:
    void valueChanged(const bool &value);

private slots:
    void readyReady(const int &ready);
};

#endif // GPIOMONITOR_H
