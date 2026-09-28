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
#include <QJsonObject>
#include <QLocalSocket>
#include <QRegularExpression>

namespace deskflow::core::ipc {

static CoreIpcServer *s_instance = nullptr;

CoreIpcServer::CoreIpcServer(QObject *parent, const QString &role)
    : IpcServer(parent, kCoreIpcName, QStringLiteral("core")),
      m_role(role)
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
  if (command == QStringLiteral("deskmatrixStatus")) {
    const auto json = QJsonDocument(m_status.snapshot(m_role, kDisplayVersion, QCoreApplication::applicationPid()));
    writeToClientSocket(
        clientSocket, QStringLiteral("deskmatrixStatus=%1").arg(QString::fromUtf8(json.toJson(QJsonDocument::Compact)))
    );
    return;
  }
  if (command == QStringLiteral("deskmatrixSwitchTarget")) {
    if (m_role != QStringLiteral("server")) {
      writeManagedTargetResult(
          clientSocket, parts.value(1), parts.value(2), QStringLiteral("rejected"),
          QStringLiteral("engine role is not server")
      );
      return;
    }
    static const QRegularExpression validId(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$"));
    if (parts.size() != 3 || !validId.match(parts.at(1)).hasMatch() || !validId.match(parts.at(2)).hasMatch()) {
      writeManagedTargetResult(
          clientSocket, parts.value(1), parts.value(2), QStringLiteral("rejected"),
          QStringLiteral("invalid request id or target")
      );
      return;
    }
    Q_EMIT managedSwitchTargetRequested(parts.at(1), parts.at(2));
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
  writeToClientSocket(clientSocket, QStringLiteral("error=unknown command"));
}

void CoreIpcServer::writeManagedTargetResult(
    QLocalSocket *clientSocket, const QString &requestId, const QString &target, const QString &state,
    const QString &error
)
{
  QJsonObject result{
      {QStringLiteral("requestId"), requestId},
      {QStringLiteral("target"), target},
      {QStringLiteral("state"), state},
  };
  if (!error.isEmpty())
    result.insert(QStringLiteral("error"), error);
  const auto json = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
  m_status.observe(QStringLiteral("managedTargetResult"), json);
  writeToClientSocket(clientSocket, QStringLiteral("managedTargetResult=%1").arg(json));
}

} // namespace deskflow::core::ipc
