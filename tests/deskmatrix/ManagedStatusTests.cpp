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
    QCOMPARE(json.value("schemaVersion").toInt(), 2);
    QCOMPARE(json.value("engine").toString(), "deskmatrix-deskflow");
    QCOMPARE(json.value("connectionState").toString(), "Starting");
    QCOMPARE(json.value("role").toString(), "server");
    QCOMPARE(json.value("pid").toInteger(), 123);
    QVERIFY(json.value("connectedClients").toArray().isEmpty());
    QCOMPARE(json.value("topologyProfile").toString(), "unknown");
    QVERIFY(json.value("activeTarget").isNull());
    QCOMPARE(json.value("cursorLocked").toBool(), false);
    QVERIFY(json.value("lastCommand").isNull());
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

  void managedSingleDisplayStateIsMachineReadable()
  {
    ManagedStatus status;
    status.observe("topologyProfile", "shared-single-display-v1");
    status.observe("cursorLocked", "true");
    status.observe("activeScreen", "mac-mini");
    status.observe(
        "managedTargetResult",
        R"({"requestId":"scene-42","target":"mac-mini","state":"applied","activeTarget":"mac-mini"})"
    );
    const auto json = status.snapshot("server", "v", 1);
    QCOMPARE(json.value("topologyProfile").toString(), "shared-single-display-v1");
    QCOMPARE(json.value("cursorLocked").toBool(), true);
    QCOMPARE(json.value("activeTarget").toString(), "mac-mini");
    const auto command = json.value("lastCommand").toObject();
    QCOMPARE(command.value("requestId").toString(), "scene-42");
    QCOMPARE(command.value("state").toString(), "applied");
  }
};

QTEST_GUILESS_MAIN(ManagedStatusTests)
#include "ManagedStatusTests.moc"
