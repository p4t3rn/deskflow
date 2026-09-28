/*
 * SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Constants.h"
#include "common/ExitCodes.h"

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QTextStream>
#include <QUuid>

// Read-only, bounded IPC query. Never starts an engine or changes its settings.
inline int queryManagedStatus()
{
  QLocalSocket socket;
  socket.connectToServer(kCoreIpcName);
  if (!socket.waitForConnected(1000)) {
    QTextStream(stderr) << "DeskMatrix engine IPC unavailable\n";
    return s_exitFailed;
  }
  socket.write("deskmatrixStatus\n");
  socket.flush();
  QElapsedTimer timer;
  timer.start();
  constexpr int timeout = 2000;
  constexpr int maxReplyBytes = 65536;
  qsizetype received = 0;
  while (timer.elapsed() < timeout) {
    if (!socket.canReadLine() && !socket.waitForReadyRead(qMax(1, static_cast<int>(timeout - timer.elapsed()))))
      break;
    if (received + socket.bytesAvailable() > maxReplyBytes)
      break;
    while (socket.canReadLine()) {
      const auto line = socket.readLine();
      received += line.size();
      const QByteArray prefix("deskmatrixStatus=");
      if (!line.startsWith(prefix))
        continue;
      const auto json = QJsonDocument::fromJson(line.mid(prefix.size()));
      const auto object = json.object();
      if (object.value("schemaVersion").toInt() != 2 || object.value("engine") != "deskmatrix-deskflow")
        break;
      QTextStream(stdout) << json.toJson(QJsonDocument::Compact) << '\n';
      return s_exitSuccess;
    }
  }
  QTextStream(stderr) << "DeskMatrix engine status unavailable or incompatible\n";
  return s_exitFailed;
}

// Bounded managed command. Success means the running server reported the
// requested target as applied; accepting a socket write is not enough.
inline int requestManagedTarget(QString target, QString requestId)
{
  static const QRegularExpression validId(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$"));
  if (!validId.match(target).hasMatch()) {
    QTextStream(stderr) << "Invalid DeskMatrix target name\n";
    return s_exitArgs;
  }
  if (requestId.isEmpty())
    requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
  if (!validId.match(requestId).hasMatch()) {
    QTextStream(stderr) << "Invalid DeskMatrix request id\n";
    return s_exitArgs;
  }

  QLocalSocket socket;
  socket.connectToServer(kCoreIpcName);
  if (!socket.waitForConnected(1000)) {
    QTextStream(stderr) << "DeskMatrix engine IPC unavailable\n";
    return s_exitFailed;
  }
  socket.write(QStringLiteral("deskmatrixSwitchTarget=%1=%2\n").arg(requestId, target).toUtf8());
  socket.flush();

  QElapsedTimer timer;
  timer.start();
  constexpr int timeout = 5000;
  constexpr int maxReplyBytes = 65536;
  qsizetype received = 0;
  while (timer.elapsed() < timeout) {
    if (!socket.canReadLine() && !socket.waitForReadyRead(qMax(1, static_cast<int>(timeout - timer.elapsed()))))
      break;
    if (received + socket.bytesAvailable() > maxReplyBytes)
      break;
    while (socket.canReadLine()) {
      const auto line = socket.readLine();
      received += line.size();
      const QByteArray prefix("managedTargetResult=");
      if (!line.startsWith(prefix))
        continue;
      const auto json = QJsonDocument::fromJson(line.mid(prefix.size())).object();
      if (json.value("requestId").toString() != requestId)
        continue;
      if (json.value("state").toString() == QStringLiteral("applied") &&
          json.value("activeTarget").toString() == target) {
        QTextStream(stdout) << QJsonDocument(json).toJson(QJsonDocument::Compact) << '\n';
        return s_exitSuccess;
      }
      QTextStream(stderr) << "DeskMatrix target switch rejected: " << json.value("error").toString() << '\n';
      return s_exitFailed;
    }
  }
  QTextStream(stderr) << "DeskMatrix target switch timed out without confirmation\n";
  return s_exitFailed;
}
