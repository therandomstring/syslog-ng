/*
 * Copyright (c) 2023 One Identity LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#ifndef SYSLOG_NG_FUZZING_HELPER_H
#define SYSLOG_NG_FUZZING_HELPER_H

#include "cfg.h"
#include "msg-format.h"
#include "logthrdest/logthrdestdrv.h"

typedef struct AppInfo AppInfo;
struct AppInfo
{
  GlobalConfig *config;
  MsgFormatOptions parse_options;
  void (*load_module) (AppInfo *self, const char* module);
};

/**
 * Initialises the app
 */
AppInfo *
app_new(void);

/**
 * Use this to free up your AppInfo at the end of the testcase
 * @param app
 */
void
app_free(AppInfo *app);

/**
 * This is needed, as destinations only accept LogMessages.
 * From here on, *EVERYTHING* is officially fuzzing `syslogformat`
 */
LogMessage *
syslog_message_new(AppInfo *app, const uint8_t *data, size_t size);

/**
 * If you used a LogMessage allocated with syslog_message_new, use this function to free it.
 * @param message
 */
void
syslog_message_free(LogMessage *message);

/**
 * Processes given message with the destination worker.
 * Performs a single connect-insert-disconnect cycle.
 * @param destination_worker
 * @param message
 */
void
destination_worker_connect_and_insert(LogThreadedDestWorker *destination_worker, LogMessage *message);

#endif //SYSLOG_NG_FUZZING_HELPER_H
