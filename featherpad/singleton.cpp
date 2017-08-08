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
#include <QDialog>
#if defined Q_WS_X11 || defined Q_OS_LINUX || defined Q_OS_FREEBSD
#include <QX11Info>
#endif
#include "singleton.h"
#include "x11.h"

namespace FeatherPad {

FPsingleton::FPsingleton (int &argc, char *argv[], const QString uniqueKey)
             : QApplication (argc, argv), uniqueKey_ (uniqueKey)
{
  // For now, the lack of x11 is seen as wayland.
#if defined Q_WS_X11 || defined Q_OS_LINUX || defined Q_OS_FREEBSD
#if QT_VERSION < 0x050200
    isX11_ = true;
#else
    isX11_ = QX11Info::isPlatformX11();
#endif
#else
    isX11_ = false;
#endif

    socketFailure_ = false;
    config_.readConfig();
    lastFiles_ = config_.getLastFiles();
    if (config_.getIconless())
        setAttribute (Qt::AA_DontShowIconsInMenus);

    sharedMemory.setKey (uniqueKey_);
    if (sharedMemory.attach())
        isRunning_ = true;
    else
    {
        isRunning_ = false;
        // create shared memory
        if (!sharedMemory.create (1))
        {
            qDebug ("Unable to create a single instance.");
            return;
        }
        // create local server and listen to incomming messages from other instances
        localServer = new QLocalServer (this);
        connect (localServer, &QLocalServer::newConnection, this, &FPsingleton::receiveMessage);
        if (!localServer->listen (uniqueKey_))
        {
            if (localServer->removeServer (uniqueKey_))
                localServer->listen (uniqueKey_);
            else
                qDebug ("Unable to remove server instance (from a previous crash).");
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
        qDebug ("%s", (const char *) localSocket->errorString().toLatin1());
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
    if (!isRunning_)
        return false;
    QLocalSocket localSocket (this);
    localSocket.connectToServer (uniqueKey_, QIODevice::WriteOnly);
    if (!localSocket.waitForConnected (timeout))
    {
        socketFailure_ = true;
        qDebug ("%s", (const char *) localSocket.errorString().toLatin1());
        return false;
    }
    localSocket.write (message.toUtf8());
    if (!localSocket.waitForBytesWritten (timeout))
    {
        socketFailure_ = true;
        qDebug ("%s", (const char *) localSocket.errorString().toLatin1());
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
    if (socketFailure_)
        fp->showCrashWarning();
    Wins.append (fp);

    /* open all files in new tabs ("\n\r" was used as the splitter) */
    QStringList sl = message.split ("\n\r");
    if (!message.isEmpty() && !sl.at (1).isEmpty())
    {
        bool multiple (sl.count() > 2 || fp->isLoading());
        for (int i = 1; i < sl.count(); ++i)
        {
            QString sli = sl.at (i);
            if (sli.startsWith ("file://"))
                sli = QUrl (sli).toLocalFile();
            /* always an absolute path (works around KDE double slash bug too) */
            QFileInfo fInfo (sli);
            fp->newTabFromName (fInfo.absoluteFilePath(), multiple);
        }
    }
    else if (!lastFiles_.isEmpty())
    {
        bool multiple (lastFiles_.count() > 1 || fp->isLoading());
        for (int i = 0; i < lastFiles_.count(); ++i)
            fp->newTabFromName (lastFiles_.at (i), multiple);
    }

    lastFiles_ = QStringList();
    return fp;
}
/*************************/
void FPsingleton::removeWin (FPwin *win)
{
    Wins.removeOne (win);
    win->deleteLater();
}
/*************************/
void FPsingleton::handleMessage (const QString& message)
{
    /* get all parts of the message */
    QStringList sl = message.split ("\n\r");
    if (sl.at (1) == "--win" || sl.at (1) == "-w")
    {
        newWin (message);
        return;
    }
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
        if (!isX11_ // always open a new tab on wayland
            || (onWhichDesktop (id) == d
                && (!Wins.at (i)->isMinimized() || isWindowShaded (id))))
        {
            bool hasDialog = false;
            QList<QDialog*> dialogs = Wins.at (i)->findChildren<QDialog*>();
            for (int j = 0; j < dialogs.count(); ++j)
            {
                if (dialogs.at (j)->objectName() !=  "processDialog"
                    && dialogs.at (j)->objectName() !=  "sessionDialog")
                {
                    hasDialog = true;
                    break;
                }
            }
            if (hasDialog) continue;
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
                    QCoreApplication::processEvents();
                    Wins.at (i)->dummyWidget->close();
                }

                /* and then, open tab(s) in the current FeatherPad window... */
                if (sl.at (1).isEmpty())
                    Wins.at (i)->newTab();
                else
                {
                    bool multiple (sl.count() > 2 || Wins.at (i)->isLoading());
                    for (int j = 1; j < sl.count(); ++j)
                    {
                        QString slj = sl.at (j);
                        if (slj.startsWith ("file://"))
                            slj = QUrl (slj).toLocalFile();
                        QFileInfo fInfo (slj);
                        Wins.at (i)->newTabFromName (fInfo.absoluteFilePath(), multiple);
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
