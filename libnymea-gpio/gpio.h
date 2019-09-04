#ifndef GPIO_H
#define GPIO_H

#include <QDir>
#include <QDebug>
#include <QObject>
#include <QLoggingCategory>

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

private:
    int m_gpio = 0;
    Gpio::Direction m_direction = Gpio::DirectionOutput;
    QDir m_gpioDirectory;

};

QDebug operator<< (QDebug debug, Gpio *gpio);

#endif // GPIO_H
