/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpcServer.h"

#include "base/Log.h"
#include "common/Constants.h"
#include "common/VersionInfo.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QLocalSocket>

namespace deskflow::core::ipc {

static CoreIpcServer *s_instance = nullptr;

CoreIpcServer::CoreIpcServer(QObject *parent, const QString &role)
    : IpcServer(parent, kCoreIpcName, QStringLiteral("core")), m_role(role)
{
  assert(s_instance == nullptr);
  s_instance = this;
}

CoreIpcServer &CoreIpcServer::instance()
{
  assert(s_instance != nullptr);
  return *s_instance;
}

void CoreIpcServer::broadcastCommand(const QString &command, const QString &args)
{
  m_status.observe(command, args);
  IpcServer::broadcastCommand(command, args);
}

void CoreIpcServer::processCommand(QLocalSocket *clientSocket, const QString &command, const QStringList &parts)
{
  Q_UNUSED(parts)
  if (command == QStringLiteral("deskmatrixStatus")) {
    const auto json = QJsonDocument(m_status.snapshot(m_role, kDisplayVersion, QCoreApplication::applicationPid()));
    writeToClientSocket(
        clientSocket, QStringLiteral("deskmatrixStatus=%1").arg(QString::fromUtf8(json.toJson(QJsonDocument::Compact)))
    );
    return;
  }
  if (command == QStringLiteral("stop")) {
    LOG_DEBUG("core ipc server got stop message");
    writeToClientSocket(clientSocket, QStringLiteral("ok"));
    broadcastCommand(QStringLiteral("bye"));
    Q_EMIT stopProcessRequested();
    return;
  }
  LOG_WARN("core ipc server got unknown command: %s", command.toUtf8().constData());
}

} // namespace deskflow::core::ipc
