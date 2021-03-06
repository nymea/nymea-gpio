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
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
