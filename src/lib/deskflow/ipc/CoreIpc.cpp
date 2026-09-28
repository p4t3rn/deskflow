/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreIpc.h"

#include "CoreIpcServer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>

void ipcSendToClient(const QString &command, const QString &args)
{
  // Queued because callers may not be on the main thread,
  // and QLocalSocket can only be written to from its owning thread.
  auto &server = deskflow::core::ipc::CoreIpcServer::instance();
  QMetaObject::invokeMethod(
      &server, [command, args] { deskflow::core::ipc::CoreIpcServer::instance().broadcastCommand(command, args); },
      Qt::QueuedConnection
  );
}

void ipcSendConnectionState(deskflow::core::ConnectionState state)
{
  const auto metaEnum = QMetaEnum::fromType<deskflow::core::ConnectionState>();
  ipcSendToClient(QStringLiteral("connectionState"), metaEnum.valueToKey(static_cast<int>(state)));
}

void ipcSendManagedTargetResult(
    const QString &requestId, const QString &target, const QString &state, const QString &activeTarget,
    const QString &error
)
{
  QJsonObject result{
      {QStringLiteral("requestId"), requestId},
      {QStringLiteral("target"), target},
      {QStringLiteral("state"), state},
  };
  if (!activeTarget.isEmpty())
    result.insert(QStringLiteral("activeTarget"), activeTarget);
  if (!error.isEmpty())
    result.insert(QStringLiteral("error"), error);
  ipcSendToClient(
      QStringLiteral("managedTargetResult"), QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))
  );
}
