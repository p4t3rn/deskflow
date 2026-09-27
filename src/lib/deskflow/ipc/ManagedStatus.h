/*
 * SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace deskflow::core::ipc {

// Updated on the IPC thread, from the same events that drive the upstream GUI.
// A running process or listening socket is deliberately not called "connected".
class ManagedStatus
{
public:
  void observe(const QString &command, const QString &args)
  {
    if (command == QStringLiteral("connectionState")) {
      m_state = args;
      if (args != QStringLiteral("Connected"))
        m_clients = {};
    } else if (command == QStringLiteral("connectedClients")) {
      m_clients = QJsonArray::fromStringList(args.split(',', Qt::SkipEmptyParts));
    }
  }

  QJsonObject snapshot(const QString &role, const QString &version, qint64 pid) const
  {
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("engine"), QStringLiteral("deskmatrix-deskflow")},
        {QStringLiteral("version"), version},
        {QStringLiteral("pid"), pid},
        {QStringLiteral("role"), role},
        {QStringLiteral("connectionState"), m_state},
        {QStringLiteral("connectedClients"), m_clients}
    };
  }

private:
  QString m_state = QStringLiteral("Starting");
  QJsonArray m_clients;
};

} // namespace deskflow::core::ipc
