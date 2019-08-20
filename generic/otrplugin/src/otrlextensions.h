/*
 * otrlextensions.h - Off-the-Record Messaging library extensions
 *
 * Strongly based on parts of the Off-the-Record Messaging library,
 * Copyright (C) 2004-2008  Ian Goldberg, Chris Alexander, Nikita Borisov
 *                          <otr@cypherpunks.ca>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef OTRLEXTENSIONS_H
#define OTRLEXTENSIONS_H

#include <stdio.h>
#include <libotr/userstate.h>

/* Store all keys of an OtrlUserState. */
gcry_error_t otrl_privkey_write(OtrlUserState us, const char* filename);

/* Store all keys of an OtrlUserState.
 * The FILE* must be open for reading and writing. */
gcry_error_t otrl_privkey_write_FILEp(OtrlUserState us, FILE* privf);

#endif OTRLEXTENSIONS_H
