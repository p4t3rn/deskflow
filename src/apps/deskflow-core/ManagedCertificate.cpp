/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ManagedCertificate.h"

#include "net/SecureUtils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <exception>
#include <memory>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace {

bool validateCertificate(const QString &path, QByteArray &fingerprint)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return false;
  const auto pem = file.readAll();
  if (pem.isEmpty())
    return false;

  const auto bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new_mem_buf(pem.constData(), pem.size()), BIO_free);
  if (!bio)
    return false;
  const auto key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(
      PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr), EVP_PKEY_free
  );
  const auto certificate =
      std::unique_ptr<X509, decltype(&X509_free)>(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr), X509_free);
  if (!key || !certificate || EVP_PKEY_bits(key.get()) < 2048 ||
      X509_check_private_key(certificate.get(), key.get()) != 1)
    return false;

  const auto result = deskflow::sslCertFingerprint(certificate.get(), QCryptographicHash::Sha256);
  fingerprint = result.data;
  return result.isValid();
}

} // namespace

int ensureManagedCertificate(const QString &path)
{
  const QFileInfo info(path);
  if (path.isEmpty() || !info.isAbsolute() || info.isSymLink()) {
    QTextStream(stderr) << "certificate path must be absolute and must not be a symbolic link\n";
    return 2;
  }

  bool created = false;
  if (!info.exists()) {
    QDir directory(info.absolutePath());
    if (!directory.mkpath(QStringLiteral("."))) {
      QTextStream(stderr) << "failed to create certificate directory\n";
      return 1;
    }
    try {
      deskflow::generatePemSelfSignedCert(path, 2048);
      created = true;
    } catch (const std::exception &error) {
      QTextStream(stderr) << "failed to create certificate: " << error.what() << '\n';
      return 1;
    }
  } else if (!info.isFile()) {
    QTextStream(stderr) << "certificate path is not a regular file\n";
    return 1;
  }

  QFile certificateFile(path);
  if (!certificateFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
    QTextStream(stderr) << "failed to restrict certificate permissions\n";
    return 1;
  }

  QByteArray fingerprint;
  if (!validateCertificate(path, fingerprint)) {
    QTextStream(stderr) << "existing certificate or private key is invalid\n";
    return 1;
  }

  const QJsonObject result{
      {QStringLiteral("schemaVersion"), 1},
      {QStringLiteral("created"), created},
      {QStringLiteral("fingerprintSha256"), QString::fromLatin1(fingerprint.toHex())},
  };
  QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << '\n';
  return 0;
}
