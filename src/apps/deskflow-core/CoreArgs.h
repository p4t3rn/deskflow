/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QCommandLineOption>
/**
 * @brief The CoreArgs class
 * This class contains args for the CoreArgParser
 */
struct CoreArgs
{
  inline static const auto helpOption = QCommandLineOption({"h", "help"}, "Display Help on the command line");
  inline static const auto versionOption = QCommandLineOption({"v", "version"}, "Display version information");
  inline static const auto multiInstanceOption =
      QCommandLineOption("new-instance", "Skip the check for a running instance, always makes a new instance");
  inline static const auto configOption =
      QCommandLineOption({"s", "settings"}, "override configuration file to use", "configFile");
  inline static const auto statusOption =
      QCommandLineOption("status-json", "Query the running DeskMatrix engine over local IPC (read-only)");
  inline static const auto switchTargetOption = QCommandLineOption(
      "switch-target", "Switch the managed single-display engine to an explicit target", "screenName"
  );
  inline static const auto requestIdOption =
      QCommandLineOption("request-id", "Correlation ID for a managed command", "requestId");
  inline static const auto managedSingleDisplayOption =
      QCommandLineOption("managed-single-display", "Lock all cursor edges and accept only explicit target switching");
  inline static const auto ensureCertificateOption = QCommandLineOption(
      "ensure-certificate", "Create or inspect a managed TLS identity and print its public fingerprint", "pemPath"
  );

  inline static const auto options = {
      helpOption,         versionOption,   multiInstanceOption,        configOption,           statusOption,
      switchTargetOption, requestIdOption, managedSingleDisplayOption, ensureCertificateOption
  };
};
