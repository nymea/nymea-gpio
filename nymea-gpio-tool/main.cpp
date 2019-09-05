#include <QCommandLineParser>

#include "application.h"
#include "gpiomonitor.h"

int main(int argc, char *argv[])
{
    Application application(argc, argv);
    application.setOrganizationName("guh");
    application.setApplicationName("nymea-gpio-tool");
    application.setApplicationVersion(VERSION_STRING);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QString applicationDescription = QString("\nnymea-gpio-tool is a command line tool which allowes to interact with GPIOs.\n"
                                             "Version: %1\n"
                                             "Copyright %2 2019 Simon Stürz <simon.stuerz@nymea.io>\n\n"
                                             "Released under the GNU GENERAL PUBLIC LICENSE Version 3.\n").arg(application.applicationVersion()).arg(QChar(0xA9));
    parser.setApplicationDescription(applicationDescription);

    QCommandLineOption gpioOption(QStringList() << "g" << "gpio", "The gpio number to use.", "GPIO");
    parser.addOption(gpioOption);

    QCommandLineOption interruptOption(QStringList() << "i" << "interrupt", "Configure the input GPIO to the given interrupt. This option is only allowed for monitoring. Allowerd interrupts are: [rising, falling, both, none]. Default is \"both\".", "INTERRUPT");
    parser.addOption(interruptOption);

    QCommandLineOption valueOption(QStringList() << "s" << "set-value", "Configure the GPIO to output and set the value. Allowerd values are: [0, 1].", "VALUE");
    parser.addOption(valueOption);

    QCommandLineOption monitorOption(QStringList() << "m" << "monitor", "Monitor the given GPIO. The GPIO will automatically configured as input and print any change regarding to the given interrupt behaviour.");
    parser.addOption(monitorOption);

    QCommandLineOption activeLowOption(QStringList() << "a" << "active-low", "Set the GPIO to active low. Allowerd values are: [0, 1]", "VALUE");
    parser.addOption(activeLowOption);

    parser.process(application);

    // Make sure there is a GPIO number passed
    if (!parser.isSet(gpioOption)) {
        qCritical() << "No GPIO number specified. Please specify a valid GPIO number using -g, --gpio GPIO";
        parser.showHelp(EXIT_FAILURE);
    }

    // Verify GPIO number
    bool gpioNumberOk;
    int gpioNumber = parser.value(gpioOption).toInt(&gpioNumberOk);
    if (!gpioNumberOk || gpioNumber < 0) {
        qCritical() << "Invalid GPIO number" << parser.value(gpioOption) << "passed. The GPIO number has to be a positiv integer.";
        return EXIT_FAILURE;
    }

    // Verify input output operations
    if ((parser.isSet(interruptOption) || parser.isSet(monitorOption)) && parser.isSet(valueOption)) {
        qCritical() << "Invalid parameter combination. The set value can only be used for output GPIO, the monitor and interrupt parameter can only be used for input GPIO.";
        return EXIT_FAILURE;
    }

    Gpio::Edge edge = Gpio::EdgeBoth;
    if (parser.isSet(interruptOption)) {
        if (parser.value(interruptOption).toLower() == "rising") {
            edge = Gpio::EdgeRising;
        } else if (parser.value(interruptOption).toLower() == "falling") {
            edge = Gpio::EdgeFalling;
        } else if (parser.value(interruptOption).toLower() == "none") {
            edge = Gpio::EdgeNone;
        } else if (parser.value(interruptOption).toLower() == "both") {
            edge = Gpio::EdgeBoth;
        } else {
            qCritical() << "Invalid interrupt parameter" << parser.value(interruptOption) << "passed. Valid options are [rising, falling, both, none].";
            return EXIT_FAILURE;
        }
    }

    bool activeLow = true;
    if (parser.isSet(activeLowOption)) {
        if (parser.value(activeLowOption) == "1") {
            activeLow = false;
        } else if (parser.value(activeLowOption) == "0") {
            activeLow = true;
        } else {
            qCritical() << "Invalid active low parameter" << parser.value(activeLowOption) << "passed. Valid options are [0, 1].";
            return EXIT_FAILURE;
        }
    }

    Gpio::Value value = Gpio::ValueInvalid;
    if (parser.isSet(valueOption)) {
        if (parser.value(valueOption) == "1") {
            value = Gpio::ValueHigh;
        } else if (parser.value(valueOption) == "0") {
            value = Gpio::ValueLow;
        } else {
            qCritical() << "Invalid set value parameter" << parser.value(valueOption) << "passed. Valid options are [0, 1].";
            return EXIT_FAILURE;
        }
    }

    if (!Gpio::isAvailable()) {
        qCritical() << "There are no GPIOs available on this platform.";
        return EXIT_FAILURE;
    }

    // Configure the GPIO
    if (parser.isSet(valueOption)) {
        Gpio *gpio = new Gpio(gpioNumber);
        if (!gpio->exportGpio()) {
            qCritical() << "Could not export GPIO" << gpioNumber;
            return EXIT_FAILURE;
        }

        if (!gpio->setDirection(Gpio::DirectionOutput)) {
            qCritical() << "Could not configure GPIO" << gpioNumber << "as output.";
            return EXIT_FAILURE;
        }

        if (parser.isSet(activeLowOption)) {
            if (!gpio->setActiveLow(activeLow)) {
                qCritical() << "Could not set GPIO" << gpioNumber << "to active low" << activeLow;
                return EXIT_FAILURE;
            }
        }

        // Finally set the value
        if (!gpio->setValue(value)) {
            qCritical() << "Could not set GPIO" << gpioNumber << "value to" << value;
            return EXIT_FAILURE;
        }

        delete gpio;
        return EXIT_SUCCESS;
    } else {
        GpioMonitor *monitor = new GpioMonitor(gpioNumber);
        monitor->setEdge(edge);
        monitor->setActiveLow(activeLow);

        // Inform about enabled changed
        QObject::connect(monitor, &GpioMonitor::enabledChanged, [gpioNumber](bool enabled) {
            qDebug() << "GPIO" << gpioNumber << "monitor" << (enabled ? "enabled" : "disabled");
        });

        // Inform about interrupt
        QObject::connect(monitor, &GpioMonitor::interruptOccured, [gpioNumber](bool value) {
            qDebug() << "GPIO" << gpioNumber << "interrupt occured. Current value:" << (value ? "1" : "0");
        });

        // Enable the monitor
        if (!monitor->enable()) {
            qCritical() << "Could not enable GPIO" << gpioNumber << "monitor.";
            return EXIT_FAILURE;
        }

        // Clean up the gpio once done
        QObject::connect(&application, &Application::aboutToQuit, [monitor](){
            delete monitor;
        });
    }

    return application.exec();
}
