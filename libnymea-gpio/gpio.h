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

#ifndef GPIO_H
#define GPIO_H

#include <QDebug>
#include <QObject>
#include <QLoggingCategory>
#include <QDir>

Q_DECLARE_LOGGING_CATEGORY(dcGpio)

class Gpio : public QObject
{
    Q_OBJECT

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

    int fileDescriptor() const;

private:
    bool lookupChip(int gpioNumber, QString &devicePath, int &lineOffset);
    bool ensureLine();
    bool requestLine(Gpio::Direction direction, Gpio::Edge edge);
    void releaseLine();

    int m_gpio = 0;
    Gpio::Direction m_direction = Gpio::DirectionInvalid;
    Gpio::Value m_value = Gpio::ValueLow;
    Gpio::Edge m_edge = Gpio::EdgeNone;
    bool m_activeLow = false;

    QString m_chipDevice;
    int m_lineOffset = -1;

    struct gpiod_chip *m_chip = nullptr;
    struct gpiod_line *m_line = nullptr;
    QDir m_gpioDirectory;

};

QDebug operator<< (QDebug debug, Gpio *gpio);

#endif // GPIO_H
