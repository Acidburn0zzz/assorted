/*
 * The internal libfcrypto header
 *
 * Copyright (C) 2008-2019, Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined( _ASSORTED_LIBFCRYPTO_H )
#define _ASSORTED_LIBFCRYPTO_H

#include <common.h>

/* Define HAVE_LOCAL_LIBFCRYPTO for local use of libfcrypto
 */
#if defined( HAVE_LOCAL_LIBFCRYPTO )

#include <libfcrypto_definitions.h>
#include <libfcrypto_rc4_context.h>
#include <libfcrypto_serpent_context.h>
#include <libfcrypto_support.h>
#include <libfcrypto_types.h>

#else

/* If libtool DLL support is enabled set LIBFCRYPTO_DLL_IMPORT
 * before including libfcrypto.h
 */
#if defined( _WIN32 ) && defined( DLL_IMPORT )
#define LIBFCRYPTO_DLL_IMPORT
#endif

#include <libfcrypto.h>

#endif /* defined( HAVE_LOCAL_LIBFCRYPTO ) */

#endif /* !defined( _ASSORTED_LIBFCRYPTO_H ) */

