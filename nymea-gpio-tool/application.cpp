#include "application.h"

#include <QDebug>
#include <signal.h>

static void catchUnixSignals(const std::vector<int>& quitSignals, const std::vector<int>& ignoreSignals = std::vector<int>())
{
    auto handler = [](int sig) ->void {
        switch (sig) {
        case SIGQUIT:
            qDebug() << "Cought SIGQUIT quit signal...";
            break;
        case SIGINT:
            qDebug() << "Cought SIGINT quit signal...";
            break;
        case SIGTERM:
            qDebug() << "Cought SIGTERM quit signal...";
            break;
        case SIGHUP:
            qDebug() << "Cought SIGHUP quit signal...";
            break;
        case SIGSEGV: {
            qCritical() << "Cought SIGSEGV signal. Segmentation fault!";
            exit(EXIT_FAILURE);
        }
        default:
            break;
        }

        Application::quit();
    };

    // all these signals will be ignored.
    for (int sig : ignoreSignals)
        signal(sig, SIG_IGN);

    for (int sig : quitSignals)
        signal(sig, handler);

}

Application::Application(int &argc, char **argv) :
    QCoreApplication(argc, argv)
{
    catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP, SIGSEGV});
}
