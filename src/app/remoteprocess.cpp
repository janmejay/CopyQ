/*
    Copyright (c) 2013, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "remoteprocess.h"

#include "common/client_server.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>

RemoteProcess::RemoteProcess(QObject *parent)
    : QObject(parent)
    , m_process()
    , m_server(NULL)
    , m_socket(NULL)
    , m_timerPing()
    , m_timerPongTimeout()
{
    m_timerPing.setInterval(8000);
    m_timerPing.setSingleShot(true);
    connect( &m_timerPing, SIGNAL(timeout()),
             this, SLOT(ping()) );

    m_timerPongTimeout.setInterval(4000);
    m_timerPongTimeout.setSingleShot(true);
    connect( &m_timerPongTimeout, SIGNAL(timeout()),
             this, SLOT(pongTimeout()) );
}

RemoteProcess::~RemoteProcess()
{
    closeConnection();
}

void RemoteProcess::start(const QString &newServerName, const QStringList &arguments)
{
    Q_ASSERT(m_server == NULL);
    Q_ASSERT(m_socket == NULL);
    Q_ASSERT(!isConnected());
    if ( isConnected() )
        return;

    m_server = newServer(newServerName, &m_process);

    COPYQ_LOG( QString("Remote process: Starting new remote process \"%1 %2\".")
               .arg(QCoreApplication::applicationFilePath())
               .arg(arguments.join(" ")) );

    m_process.start( QCoreApplication::applicationFilePath(), arguments );

    if ( !m_process.waitForStarted(16000) ) {
        log( "Remote process: Failed to start new remote process!", LogError );
    } else if ( !m_server->waitForNewConnection(16000) ) {
        log( "Remote process: Failed to connect to new remote process!", LogError );
    } else {
        COPYQ_LOG("Remote process: Started.");
        m_socket = m_server->nextPendingConnection();
        connect( m_socket, SIGNAL(readyRead()),
                 this, SLOT(readyRead()) );
        ping();
    }
}

bool RemoteProcess::writeMessage(const QByteArray &msg)
{
    Q_ASSERT(m_server != NULL);
    Q_ASSERT(m_socket != NULL);
    Q_ASSERT(isConnected());
    if ( !isConnected() ) {
        COPYQ_LOG("Remote process: Cannot write message!");
        return false;
    }

    if ( !::writeMessage(m_socket, msg) ) {
        emit connectionError();
        return false;
    }

    return true;
}

bool RemoteProcess::isConnected() const
{
    return m_socket != NULL && m_socket->state() == QLocalSocket::ConnectedState &&
            m_process.state() == QProcess::Running;
}

void RemoteProcess::closeConnection()
{
    if (m_server != NULL) {
        if (m_socket != NULL) {
            m_socket->disconnectFromServer();
            m_socket->deleteLater();
            m_socket = NULL;
        }

        m_server->close();
        m_server->deleteLater();
        m_server = NULL;

        if ( m_process.state() != QProcess::NotRunning && !m_process.waitForFinished(1000) ) {
            log( "Remote process: Close connection unsucessful!", LogError );
            m_process.terminate();
            if ( m_process.state() != QProcess::NotRunning && !m_process.waitForFinished(1000) ) {
                log( "Remote process: Cannot terminate process!", LogError );
                m_process.kill();
                if ( m_process.state() != QProcess::NotRunning ) {
                    log( "Remote process: Cannot kill process!!!", LogError );
                }
            }
        }
    }
}

void RemoteProcess::readyRead()
{
    Q_ASSERT(m_server != NULL);
    Q_ASSERT(m_socket != NULL);
    Q_ASSERT(isConnected());

    m_socket->blockSignals(true);

    while ( m_socket->bytesAvailable() > 0 ) {
        QByteArray msg;
        if( !::readMessage(m_socket, &msg) ) {
            log( "Remote process: Incorrect message from remote process.", LogError );
            emit connectionError();
            return;
        }

        if (msg == "pong") {
            m_timerPing.start();
            m_timerPongTimeout.stop();
        } else {
            emit newMessage(msg);
        }
    }

    m_socket->blockSignals(false);
}

void RemoteProcess::ping()
{
    writeMessage("ping");
    m_timerPongTimeout.start();
}

void RemoteProcess::pongTimeout()
{
    log( "Remote process: Connection timeout!", LogError );
    emit connectionError();
}
