/*
 * SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
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
    } else if (command == QStringLiteral("activeScreen")) {
      m_activeTarget = args;
    } else if (command == QStringLiteral("cursorLocked")) {
      m_cursorLocked = args == QStringLiteral("true");
    } else if (command == QStringLiteral("topologyProfile")) {
      m_topologyProfile = args;
    } else if (command == QStringLiteral("managedTargetResult")) {
      const auto object = QJsonDocument::fromJson(args.toUtf8()).object();
      if (!object.isEmpty())
        m_lastCommand = object;
    }
  }

  QJsonObject snapshot(const QString &role, const QString &version, qint64 pid) const
  {
    return {
        {QStringLiteral("schemaVersion"), 2},
        {QStringLiteral("engine"), QStringLiteral("deskmatrix-deskflow")},
        {QStringLiteral("version"), version},
        {QStringLiteral("pid"), pid},
        {QStringLiteral("role"), role},
        {QStringLiteral("connectionState"), m_state},
        {QStringLiteral("connectedClients"), m_clients},
        {QStringLiteral("topologyProfile"), m_topologyProfile},
        {QStringLiteral("activeTarget"),
         m_activeTarget.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(m_activeTarget)},
        {QStringLiteral("cursorLocked"), m_cursorLocked},
        {QStringLiteral("lastCommand"),
         m_lastCommand.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(m_lastCommand)}
    };
  }

private:
  QString m_state = QStringLiteral("Starting");
  QJsonArray m_clients;
  QString m_topologyProfile = QStringLiteral("unknown");
  QString m_activeTarget;
  bool m_cursorLocked = false;
  QJsonObject m_lastCommand;
};

} // namespace deskflow::core::ipc
