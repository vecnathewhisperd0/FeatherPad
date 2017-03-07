/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SINGLETON_H
#define SINGLETON_H

#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>
#include "fpwin.h"
#include "config.h"

namespace FeatherPad {

// A single-instance approach based on QSharedMemory.
class FPsingleton : public QApplication
{
    Q_OBJECT
public:
    FPsingleton (int &argc, char *argv[], const QString uniqueKey);
    ~FPsingleton();

    bool sendMessage (const QString &message);
    FPwin* newWin (const QString &message);
    void removeWin (FPwin *win);

    QList<FPwin*> Wins; // All FeatherPad windows.

    Config& getConfig() {
        return config_;
    }
    bool isX11() {
      return isX11_;
    }

public slots:
    void receiveMessage();
    void handleMessage (const QString& message);
    //void quitting();

signals:
    void messageReceived (QString message);

private:
    bool _isRunning;
    QString _uniqueKey;
    QSharedMemory sharedMemory;
    QLocalServer *localServer;
    static const int timeout = 1000;
    Config config_;
    QString firstFile_;
    bool isX11_;
};

}

#endif // SINGLETON_H
