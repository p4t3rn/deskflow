/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016, 2025 - 2026 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreArgParser.h"
#include "ManagedCertificate.h"
#include "ManagedStatusQuery.h"

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "deskflow/ClientApp.h"
#include "deskflow/ServerApp.h"
#include "deskflow/ipc/CoreIpcServer.h"

#if defined(Q_OS_WIN)
#include "arch/win32/ArchMiscWindows.h"
#endif

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSharedMemory>
#include <QTextStream>
#include <QThread>

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
  const auto utf8 = message.toUtf8();
  switch (type) {
  case QtDebugMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_DEBUG "%s", utf8.constData());
    break;
  case QtInfoMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_INFO "%s", utf8.constData());
    break;
  case QtWarningMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_WARN "%s", utf8.constData());
    break;
  case QtCriticalMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_ERR "%s", utf8.constData());
    break;
  case QtFatalMsg:
    CLOG->print(context.file, context.line, CLOG_TAG_CRIT "%s", utf8.constData());
    break;
  }
  if (type == QtFatalMsg) {
    abort();
  }
}

void showHelp(const CoreArgParser &parser)
{
  QTextStream(stdout) << parser.helpText();
}

App *createApp(const CoreArgParser &parser, EventQueue &events, const QString &processName)
{
  if (parser.serverMode()) {
    return new ServerApp(&events, processName, parser.managedSingleDisplay());
  } else if (parser.clientMode()) {
    return new ClientApp(&events, processName);
  }
  return nullptr;
}

int main(int argc, char **argv)
{
  // A status query must work without a graphical session and must not initialize
  // the input backend, shared-memory lock, or persistent settings.
  for (int i = 1; i < argc; ++i) {
    const auto arg = QString::fromLocal8Bit(argv[i]);
    if (arg == QStringLiteral("--status-json") || arg == QStringLiteral("--switch-target") ||
        arg == QStringLiteral("--ensure-certificate") || arg.startsWith(QStringLiteral("--ensure-certificate=")) ||
        arg.startsWith(QStringLiteral("--switch-target="))) {
      QCoreApplication queryApp(argc, argv);
      const CoreArgParser parser(QCoreApplication::arguments());
      if (!parser.errorText().isEmpty()) {
        QTextStream(stderr) << parser.errorText() << '\n';
        return s_exitArgs;
      }
      if (parser.ensureCertificateRequested()) {
        if (parser.statusJson() || parser.switchTargetRequested()) {
          QTextStream(stderr) << "certificate initialization cannot be combined with IPC commands\n";
          return s_exitArgs;
        }
        return ensureManagedCertificate(parser.certificatePath());
      }
      if (parser.switchTargetRequested())
        return requestManagedTarget(parser.switchTarget(), parser.requestId());
      return queryManagedStatus();
    }
  }
#if defined(Q_OS_WIN)
  ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

  QApplication::setApplicationName(QStringLiteral("%1 Core").arg(kAppName));
  QApplication app(argc, argv);

  Arch arch;
  arch.init();

  Log log;
  qInstallMessageHandler(qtMessageHandler);

  CoreArgParser parser(QCoreApplication::arguments());

  // Print any parser errors
  if (!parser.errorText().isEmpty()) {
    QTextStream(stderr) << parser.errorText() << "\n";
    return s_exitArgs;
  }

  if (parser.help()) {
    showHelp(parser);
    return s_exitSuccess;
  }

  if (parser.version()) {
    QTextStream(stdout) << parser.versionText();
    return s_exitSuccess;
  }

  if (parser.statusJson())
    return queryManagedStatus();
  if (parser.switchTargetRequested())
    return requestManagedTarget(parser.switchTarget(), parser.requestId());

  // Before we check any more args we need to check for a duplicate process.
  // Create a shared memory segment with a unique key
  // This is to prevent a new instance from running if one is already running
  QSharedMemory sharedMemory(kCoreBinName);

  // Attempt to attach first and detach in order to clean up stale shm chunks
  // This can happen if the previous instance was killed or crashed
  if (sharedMemory.attach())
    sharedMemory.detach();

  if (!sharedMemory.create(1) && parser.singleInstanceOnly()) {
    LOG_WARN("an instance of deskflow core is already running");
    return s_exitDuplicate;
  }

  parser.parse();

  EventQueue events;
  const auto processName = QFileInfo(argv[0]).fileName();

  App *coreApp = createApp(parser, events, processName);

  const auto role = parser.serverMode() ? QStringLiteral("server") : QStringLiteral("client");
  const auto ipcServer = new deskflow::core::ipc::CoreIpcServer(&app, role); // NOSONAR - Qt managed
  QObject::connect(
      ipcServer, &deskflow::core::ipc::IpcServer::stopProcessRequested, coreApp, &App::quit, Qt::DirectConnection
  );
  QObject::connect(
      ipcServer, &deskflow::core::ipc::CoreIpcServer::managedSwitchTargetRequested, &app,
      [&events](const QString &requestId, const QString &target) {
        events.addEvent(Event(
            EventTypes::ServerAppManagedSwitchTarget, events.getSystemTarget(),
            new ServerApp::ManagedSwitchTargetInfo(requestId.toStdString(), target.toStdString())
        ));
      }
  );
  ipcServer->listen();

  QThread coreThread;
  QObject::connect(&coreThread, &QThread::finished, &app, &QApplication::quit);
  coreApp->run(coreThread);

  int exitCode = QApplication::exec();
  coreThread.wait();

  if (exitCode == s_exitSuccess) {
    exitCode = coreApp->getExitCode();
  }
  delete coreApp;

  LOG_DEBUG("core exited, code: %d", exitCode);
  return exitCode;
}
