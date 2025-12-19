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

#ifndef GPIOBUTTON_H
#define GPIOBUTTON_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class GpioMonitor;

class GpioButton : public QObject
{
    Q_OBJECT
public:
    explicit GpioButton(int gpio, QObject *parent = nullptr);

    int gpioNumber() const;

    bool activeLow() const;
    void setActiveLow(bool activeLow);

    bool repeateLongPressed() const;
    void setRepeateLongPressed(bool repeateLongPressed);

    int longPressedTimeout() const;
    void setLongPressedTimeout(int longPressedTimeout);

    QString name() const;
    void setName(const QString &name);

private:
    int m_gpioNumber;
    bool m_activeLow = false;
    bool m_repeateLongPressed = false;
    int m_longPressedTimeout = 250;
    QString m_name;

    GpioMonitor *m_monitor = nullptr;
    QTimer *m_timer = nullptr;

    QElapsedTimer m_time;

signals:
    void clicked();
    void pressed();
    void released();
    void longPressed();

private slots:
    void onTimeout();
    void onValueChanged(bool value);

public slots:
    bool enable();
    void disable();
};

QDebug operator<<(QDebug debug, GpioButton *gpioButton);

#endif // GPIOBUTTON_H
