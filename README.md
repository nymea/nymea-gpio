# nymea-gpio

nymea-gpio is a small Qt/C++ GPIO helper library for Linux and a matching CLI tool. The library is used by nymea but can be embedded in other Qt applications, while the CLI is handy for scripting, bring-up, and diagnostics.

## Components

- `libnymea-gpio`: Qt library with `Gpio`, `GpioMonitor`, and `GpioButton` helpers.
- `nymea-gpio-tool`: CLI wrapper around the same API.

## Supported GPIO backends

- Linux libgpiod API v1 or v2 (auto-detected via pkg-config).
- Linux sysfs GPIO (deprecated upstream, build-time opt-in).

## Requirements

- Linux
- Qt 5 or Qt 6 (qmake)
- `pkg-config` and `libgpiod` (unless building the legacy sysfs backend)
- `dpkg-parsechangelog` (from `dpkg-dev`) to populate `VERSION_STRING` during build

## Build

Build both the library and the CLI:

```sh
qmake nymea-gpio.pro
make -j
```

If you are building with Qt 6, use `qmake6` instead of `qmake`.

To force the legacy sysfs backend:

```sh
qmake "CONFIG+=nymea_gpio_sysfs" nymea-gpio.pro
make -j
```

Install (optional):

```sh
make install
```

The install step drops the library, headers, and a `nymea-gpio` pkg-config file into your Qt prefix.

## CLI usage

`nymea-gpio-tool` requires a GPIO number and then either sets an output value or monitors input changes.

Set a GPIO output:

```sh
nymea-gpio-tool --gpio 17 --set-value 1
```

Monitor input changes (default edge is `both`):

```sh
nymea-gpio-tool --gpio 17 --monitor --interrupt both
```

Active-low input or output:

```sh
nymea-gpio-tool --gpio 17 --set-value 0 --active-low
```

## Library examples

Output GPIO:

```cpp
Gpio *gpioOut = new Gpio(23, this);

// Export Gpio
if (!gpioOut->exportGpio()) {
    qWarning() << "Could not export Gpio" << gpioOut->gpioNumber();
    gpioOut->deleteLater();
    return;
}

// Configure Gpio direction
if (!gpioOut->setDirection(Gpio::DirectionOutput)) {
    qWarning() << "Could not set direction of Gpio" << gpioOut->gpioNumber();
    gpioOut->deleteLater();
    return;
}

gpioOut->setValue(Gpio::ValueHigh);
```

Input GPIO:

```cpp
Gpio *gpioIn = new Gpio(24, this);

// Export Gpio
if (!gpioIn->exportGpio()) {
    qWarning() << "Could not export Gpio" << gpioIn->gpioNumber();
    gpioIn->deleteLater();
    return;
}

// Configure Gpio direction
if (!gpioIn->setDirection(Gpio::DirectionInput)) {
    qWarning() << "Could not set direction of Gpio" << gpioIn->gpioNumber();
    gpioIn->deleteLater();
    return;
}

qDebug() << "Current value" << gpioIn->value();
```

Monitor a button with GpioMonitor:

```cpp
Button::Button(QObject *parent) :
    QObject(parent)
{
    m_button = new GpioMonitor(110, this);
    connect(m_button, &GpioMonitor::valueChanged, this, &Button::stateChanged);
}
bool Button::init()
{
    return m_button->enable();
}
void Button::stateChanged(const bool &value)
{
    if (m_pressed != value) {
        m_pressed = value;
        if (value) {
            emit buttonPressed();
        } else {
            emit buttonReleased();
        }
    }
}
```

Button helper with debounced GpioButton:

```cpp
GpioButton *button = new GpioButton(15, this);
button->setName("User button");
if (!button->enable()) {
    qWarning() << "Could not enable the" << this;
    button->deleteLater();
    return;
}

connect(button, &GpioButton::clicked, this, [this, button](){
    qDebug() << button << "clicked";
});
```

## License

- `libnymea-gpio` is licensed under the GNU Lesser General Public License v3.0 or (at your option) any later version. See `LICENSE.LGPL3`.
- `nymea-gpio-tool` is licensed under the GNU General Public License v3.0 or (at your option) any later version. See `LICENSE.GPL3`.
