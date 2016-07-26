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

#include <QDesktopWidget>
#include <QLocalSocket>
#include <QFileDialog>
#include <QMessageBox>
#include "singleton.h"
#include "x11.h"

namespace FeatherPad {

FPsingleton::FPsingleton (int &argc, char *argv[], const QString uniqueKey)
             : QApplication (argc, argv), _uniqueKey (uniqueKey)
{
    config_.readConfig();
    sharedMemory.setKey (_uniqueKey);
    if (sharedMemory.attach())
        _isRunning = true;
    else
    {
        _isRunning = false;
        // create shared memory
        if (!sharedMemory.create (1))
        {
            qDebug ("Unable to create single instance.");
            return;
        }
        // create local server and listen to incomming messages from other instances
        localServer = new QLocalServer (this);
        connect (localServer, &QLocalServer::newConnection, this, &FPsingleton::receiveMessage);
        if (!localServer->listen (_uniqueKey))
        {
            if (localServer->removeServer (_uniqueKey))
                localServer->listen (_uniqueKey);
            else
                qDebug ("Unable to remove server instance (from previous crash).");
        }
    }
}
/*************************/
FPsingleton::~FPsingleton()
{
    config_.writeConfig();
}
/*************************/
void FPsingleton::receiveMessage()
{
    QLocalSocket *localSocket = localServer->nextPendingConnection();
    if (!localSocket->waitForReadyRead (timeout))
    {
        qDebug (localSocket->errorString().toLatin1());
        return;
    }
    QByteArray byteArray = localSocket->readAll();
    QString message = QString::fromUtf8 (byteArray.constData());
    emit messageReceived (message);
    localSocket->disconnectFromServer();
}
/*************************/
bool FPsingleton::sendMessage (const QString &message)
{
    if (!_isRunning)
        return false;
    QLocalSocket localSocket (this);
    localSocket.connectToServer (_uniqueKey, QIODevice::WriteOnly);
    if (!localSocket.waitForConnected (timeout))
    {
        qDebug (localSocket.errorString().toLatin1());
        return false;
    }
    localSocket.write (message.toUtf8());
    if (!localSocket.waitForBytesWritten (timeout))
    {
        qDebug (localSocket.errorString().toLatin1());
        return false;
    }
    localSocket.disconnectFromServer();
    return true;
}
/*************************/
FPwin* FPsingleton::newWin (const QString& message)
{
    FPwin *fp = new FPwin;
    fp->show();
    Wins.append (fp);

    /* open all files in new tabs */
    QStringList sl = message.split ("\n");
    if (!message.isEmpty() && !sl.at (1).isEmpty())
    {
        for (int i = 1; i < sl.count(); ++i)
        {
            QString sli = sl.at (i);
            if (sli.startsWith ("file://"))
                sli = QUrl (sli).toLocalFile();
            fp->newTabFromName (sli);
        }
    }

    return fp;
}
/*************************/
void FPsingleton::handleMessage (const QString& message)
{
    /* get all parts of the message */
    QStringList sl = message.split ("\n");
    /* get the desktop the command is issued from */
    long d = sl.at (0).toInt();
    bool found = false;
    const QRect sr = QApplication::desktop()->screenGeometry();
    for (int i = 0; i < Wins.count(); ++i)
    {
        WId id = Wins.at (i)->winId();
        /* if the command is issued from where a FeatherPad
           window exists and if that window isn't minimized
           and doesn't have a modal dialog... */
        if (onWhichDesktop (id) == d
            && (!Wins.at (i)->isMinimized() || isWindowShaded (id))
            && Wins.at (i)->findChildren<QFileDialog*>().isEmpty()
            && Wins.at (i)->findChildren<QDialog*>().isEmpty()
            && Wins.at (i)->findChildren<QMessageBox*>().isEmpty())
        {
            /* consider viewports too, so that if more than half of the width as well as the height
               of the window is inside the current viewport (of the current desktop), open a new tab */
            QRect g = Wins.at (i)->geometry();
            if (g.x() + g.width()/2 >= 0 && g.x() + g.width()/2 < sr.width()
                && g.y() + g.height()/2 >= 0 && g.y() + g.height()/2 < sr.height())
            {
                if (d >= 0) // it may be -1 for some DEs that don't support _NET_CURRENT_DESKTOP
                {
                    /* first, pretend to KDE that a new window is created
                       (without this, the next new window would open on a wrong desktop) */
                    Wins.at (i)->dummyWidget->showMinimized();
                    QTimer::singleShot (0, Wins.at (i)->dummyWidget, SLOT (close()));
                }

                /* and then, open tab(s) in the current FeatherPad window... */
                if (sl.at (1).isEmpty())
                    Wins.at (i)->newTab();
                else
                {
                    for (int j = 1; j < sl.count(); ++j)
                    {
                        QString slj = sl.at (j);
                        if (slj.startsWith ("file://"))
                            slj = QUrl (slj).toLocalFile();
                        Wins.at (i)->newTabFromName (slj);
                    }
                }
                found = true;
                break;
            }
        }
    }
    if (!found)
        /* ... otherwise, make a new window */
        newWin (message);
}
/*************************/
/*void FPsingleton::quitting ()
{
    qDebug() << "About to quit";
}*/

}
