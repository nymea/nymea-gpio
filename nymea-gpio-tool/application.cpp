// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-gpio.
*
* nymea-gpio is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-gpio is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-gpio. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "application.h"

#include <signal.h>
#include <QDebug>

static void catchUnixSignals(const std::vector<int> &quitSignals, const std::vector<int> &ignoreSignals = std::vector<int>())
{
    auto handler = [](int sig) -> void {
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

Application::Application(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP, SIGSEGV});
}
