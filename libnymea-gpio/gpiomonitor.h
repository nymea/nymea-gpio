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
    Gpio::Value m_value = Gpio::ValueInvalid;

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
