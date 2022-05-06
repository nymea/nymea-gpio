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

#ifndef GPIOBUTTON_H
#define GPIOBUTTON_H

#include <QElapsedTimer>
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

QDebug operator<< (QDebug debug, GpioButton *gpioButton);


#endif // GPIOBUTTON_H
