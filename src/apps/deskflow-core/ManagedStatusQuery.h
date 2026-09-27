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
#include <QTextStream>

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
      if (object.value("schemaVersion").toInt() != 1 || object.value("engine") != "deskmatrix-deskflow")
        break;
      QTextStream(stdout) << json.toJson(QJsonDocument::Compact) << '\n';
      return s_exitSuccess;
    }
  }
  QTextStream(stderr) << "DeskMatrix engine status unavailable or incompatible\n";
  return s_exitFailed;
}
