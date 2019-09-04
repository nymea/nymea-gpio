#include <QCoreApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    application.setOrganizationName("guh");
    application.setApplicationName("nymea-gpio-tool");
    application.setApplicationVersion(VERSION_STRING);

    QCommandLineParser parser;
    parser.addHelpOption();
    QString applicationDescription = QString("\nnymea-gpio-tool is a command line tool which allowes to interact with GPIOs.\n"
                                             "Version: %1\n"
                                             "Copyright %2 2019 Simon Stürz <simon.stuerz@nymea.io>\n"
                                             "Released under the GNU GENERAL PUBLIC LICENSE Version 3.").arg(application.applicationVersion()).arg(QChar(0xA9));
    parser.setApplicationDescription(applicationDescription);
    parser.process(application);

    return application.exec();
}
