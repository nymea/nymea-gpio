#include "gpiobutton.h"

GpioButton::GpioButton(int gpioNumber, QObject *parent) :
    QObject(parent),
    m_gpioNumber(gpioNumber)
{

}

int GpioButton::gpioNumber() const
{
    return m_gpioNumber;
}

bool GpioButton::activeLow() const
{
    return m_activeLow;
}

void GpioButton::setActiveLow(bool activeLow)
{
    m_activeLow = activeLow;
}

bool GpioButton::repeateLongPressed() const
{
    return m_repeateLongPressed;
}

void GpioButton::setRepeateLongPressed(bool repeateLongPressed)
{
    m_repeateLongPressed = repeateLongPressed;
}

int GpioButton::longPressedTimeout() const
{
    return m_longPressedTimeout;
}

void GpioButton::setLongPressedTimeout(int longPressedTimeout)
{
    m_longPressedTimeout = longPressedTimeout;
}

QString GpioButton::name() const
{
    return m_name;
}

void GpioButton::setName(const QString &name)
{
    m_name = name;
}

void GpioButton::onTimeout()
{
    qCDebug(dcGpio()) << this << "long pressed";
    emit longPressed();
}

void GpioButton::onInterruptOccured(bool value)
{
    if (value) {
        // Pressed
        qCDebug(dcGpio()) << this << "pressed";
        emit pressed();

        m_timer->setSingleShot(!m_repeateLongPressed);
        m_timer->start(m_longPressedTimeout);
        m_time.restart();
    } else {
        // Released
        qCDebug(dcGpio()) << this << "released";
        emit released();

        m_timer->stop();
        int duration = m_time.elapsed();

        // Debounce and limit to 500 ms
        if (duration >= 10 && duration <= 500) {
            qCDebug(dcGpio()) << this << "clicked";
            emit clicked();
        }
    }
}

bool GpioButton::enable()
{
    // Make sure we have a clean start
    disable();

    m_monitor = new GpioMonitor(m_gpioNumber, this);
    m_monitor->setEdge(Gpio::EdgeBoth);
    m_monitor->setActiveLow(m_activeLow);

    if (!m_monitor->enable()) {
        qCWarning(dcGpio()) << "Could not enable GPIO monitor for" << this;
        delete m_monitor;
        m_monitor = nullptr;
        return false;
    }
    connect(m_monitor, &GpioMonitor::interruptOccured, this, &GpioButton::onInterruptOccured, Qt::DirectConnection);

    // Setup timer, if this timer reaches timeout, a long pressed happend
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setSingleShot(!m_repeateLongPressed);
    m_timer->setInterval(m_longPressedTimeout);
    connect(m_timer, &QTimer::timeout, this, &GpioButton::onTimeout, Qt::DirectConnection);
    return true;
}

void GpioButton::disable()
{
    if (m_monitor) {
        delete m_monitor;
        m_monitor = nullptr;
    }

    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
}

QDebug operator<<(QDebug debug, GpioButton *gpioButton)
{
    debug.nospace() << "GpioButton(" << gpioButton->gpioNumber() << ", ";
    debug.nospace() << "name: " << gpioButton->name() << ")";
    return debug.space();
}
