/*
 * SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ipc/ManagedStatus.h"

#include <QJsonDocument>
#include <QTest>

using deskflow::core::ipc::ManagedStatus;

class ManagedStatusTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initialStateIsNotConnected()
  {
    ManagedStatus status;
    const auto json = status.snapshot("server", "test-version", 123);
    QCOMPARE(json.value("schemaVersion").toInt(), 1);
    QCOMPARE(json.value("engine").toString(), "deskmatrix-deskflow");
    QCOMPARE(json.value("connectionState").toString(), "Starting");
    QCOMPARE(json.value("role").toString(), "server");
    QCOMPARE(json.value("pid").toInteger(), 123);
    QVERIFY(json.value("connectedClients").toArray().isEmpty());
  }

  void listeningIsNotConnected()
  {
    ManagedStatus status;
    status.observe("connectionState", "Listening");
    QCOMPARE(status.snapshot("server", "v", 1).value("connectionState").toString(), "Listening");
  }

  void disconnectClearsPeerList()
  {
    ManagedStatus status;
    status.observe("connectionState", "Connected");
    status.observe("connectedClients", "desktop,laptop");
    QCOMPARE(status.snapshot("server", "v", 1).value("connectedClients").toArray().size(), 2);
    status.observe("connectionState", "Listening");
    QVERIFY(status.snapshot("server", "v", 1).value("connectedClients").toArray().isEmpty());
    status.observe("connectionState", "Disconnected");
    QCOMPARE(status.snapshot("client", "v", 1).value("connectionState").toString(), "Disconnected");
  }

  void reconnectAndUnrelatedMessages()
  {
    ManagedStatus status;
    status.observe("connectionState", "Connecting");
    status.observe("retryIn", "5");
    QCOMPARE(status.snapshot("client", "v", 1).value("connectionState").toString(), "Connecting");
    status.observe("connectionState", "Connected");
    status.observe("connectedClients", "电脑-\"A\",laptop");
    const auto json = status.snapshot("server", "v", 1);
    QCOMPARE(QJsonDocument::fromJson(QJsonDocument(json).toJson()).object(), json);
    status.observe("connectedClients", "");
    QVERIFY(status.snapshot("server", "v", 1).value("connectedClients").toArray().isEmpty());
  }
};

QTEST_GUILESS_MAIN(ManagedStatusTests)
#include "ManagedStatusTests.moc"
