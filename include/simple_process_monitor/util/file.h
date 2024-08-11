/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */

#ifndef UTIL_FILE_H
#define UTIL_FILE_H

#include <stdbool.h>

/**
 * Reads an proc filesystem object
 * @param buf buffer to write to
 * @param buf_size size of buf
 * @param name name of proc object
 * @param pid number of the process or < 0 if main directory
 * @param tid number of the thread or < 0 if main directory or process
 * @param bytes_read number of bytes read to buffer
 * @return true if succeeded otherwise false.
 */
bool file_readProc(char *buf, int buf_size, const char *name, int pid, int tid, int *bytes_read);

#endif
