/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

// libotr 4.1.1's message.h is not self-contained: it relies on protocol
// declarations such as OtrlPolicy and OtrlTLV being visible first. Keep that
// dependency in one test-only compatibility header rather than relying on
// include order in individual test translation units.
extern "C" {
#include <libotr/proto.h>
}

extern "C" {
#include <libotr/auth.h>
#include <libotr/b64.h>
#include <libotr/instag.h>
#include <libotr/message.h>
#include <libotr/privkey.h>
#include <libotr/userstate.h>
}
