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

#ifndef GPIOBUTTON_H
#define GPIOBUTTON_H

#include <QTime>
#include <QTimer>
#include <QObject>

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
    bool m_activeLow = true;
    bool m_repeateLongPressed = false;
    int m_longPressedTimeout = 250;
    QString m_name;

    GpioMonitor *m_monitor = nullptr;
    QTimer *m_timer = nullptr;

    QTime m_time;

signals:
    void clicked();
    void pressed();
    void released();
    void longPressed();

private slots:
    void onTimeout();
    void onInterruptOccured(bool value);

public slots:
    bool enable();
    void disable();

};

QDebug operator<< (QDebug debug, GpioButton *gpioButton);


#endif // GPIOBUTTON_H
