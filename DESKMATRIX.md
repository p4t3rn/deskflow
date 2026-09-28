<!-- SPDX-FileCopyrightText: (C) 2026 DeskMatrix contributors -->
<!-- SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception -->

# DeskMatrix input engine — development branch

Based on upstream `2872df3720f68d4b2abd8876506ff0a75274d1fe`.
This is not yet a packaged or validated replacement for Deskflow.
Keep upstream copyright, GPLv2 and the OpenSSL exception with redistributed binaries
and provide corresponding source for the exact distributed revision.

## Managed status contract, schema 2

`deskflow-core --status-json` queries the running fork over local IPC. It does not
start another engine, change settings, accept certificates or inject input.
Exit 0 means a compatible JSON response was received; nonzero means unavailable,
incompatible, timed out or invalid arguments. Errors go to stderr.

The JSON contains `schemaVersion`, `engine`, `version`, `pid`, `role`,
`connectionState`, `connectedClients`, `topologyProfile`, `activeTarget`,
`cursorLocked` and `lastCommand`. States come from upstream engine events:
Starting / Connecting / Listening / Connected / Disconnected. Server peer names
come from upstream's comma-delimited connectedClients event; screen names containing
commas are not supported by that upstream event format. Listening is not Connected.
Connected does not prove that physical input was delivered, nor that TLS was
enabled. Do not label this status as end-to-end or certificate verification.

The new IPC request is `deskmatrixStatus\n`, returning
`deskmatrixStatus=<compact JSON>\n`. The status snapshot survives other clients
consuming the historical broadcast queue. Stream reads preserve partial lines and
reject oversized requests. Status replies never contain credentials or private keys.

## Managed single-display target switching

Start the server with `--managed-single-display`. This locks all four cursor edges,
rejects directional/toggle switching, and identifies the running profile as
`shared-single-display-v1`. It is intended for several computers sharing one visible
physical monitor, not a continuously visible multi-monitor desktop.

`deskflow-core --switch-target <screen> --request-id <id>` sends a bounded local IPC
request. Exit 0 is returned only after the running server reports the same request ID,
target and active target with state `applied`. Missing/disconnected targets, wrong
roles, non-managed servers, incompatible replies and timeouts fail closed. Target and
request identifiers are restricted to a small non-whitespace character set. Agent
commands should use this interface instead of injecting Deskflow hotkeys.

The core IPC socket now uses the current-user ACL. The legacy system daemon keeps its
upstream cross-account access contract separately. This local IPC is still not a
remote authentication API and must never be exposed over TCP.

## Existing launch format

Use `deskflow-core server --settings <INI>` or `deskflow-core client --settings <INI>`.
`--settings` takes Qt INI settings, **not** the topology text exported by the web console.
Relevant upstream keys: core/computerName, core/port, client/remoteHost,
server/externalConfig, server/externalConfigFile, security/tlsEnabled,
security/checkPeerFingerprints and security/certificate. Never disable TLS or
peer checks to make onboarding appear successful.

## Not implemented / not release-ready

Dedicated engine namespace; authenticated configuration sync; certificate enrollment
and rotation; managed lifecycle; signed Windows distribution; packaged Agent/Web
consumption; Windows/macOS end-to-end input acceptance. The IPC name is still shared
with the upstream GUI and needs a dedicated namespace before release.

Standalone status model tests: `cmake -S tests/deskmatrix -B build/status-tests`,
then build and run CTest. These do not replace a full core build or IPC integration test.
