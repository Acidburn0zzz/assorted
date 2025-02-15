/*
 * Decompresses 7-bit ASCII compressed data
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

#include <common.h>
#include <file_stream.h>
#include <memory.h>
#include <narrow_string.h>
#include <system_string.h>
#include <types.h>

#if defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif

#include "ascii7.h"
#include "assorted_getopt.h"
#include "assorted_libcerror.h"
#include "assorted_libcfile.h"
#include "assorted_libcnotify.h"
#include "assorted_output.h"

/* Prints the executable usage information
 */
void usage_fprint(
      FILE *stream )
{
	if( stream == NULL )
	{
		return;
	}
	fprintf( stream, "Use ascii7decompress to decompress 7-bit ASCII compressed data.\n\n" );

	fprintf( stream, "Usage: ascii7decompress [ -o offset ] [ -s size ] [ -hvV ] source\n\n" );

	fprintf( stream, "\tsource: the source file\n\n" );

	fprintf( stream, "\t-h:     shows this help\n" );
	fprintf( stream, "\t-o:     data offset (default is 0)\n" );
	fprintf( stream, "\t-s:     size of data (default is the file size)\n" );
	fprintf( stream, "\t-v:     verbose output to stderr\n" );
	fprintf( stream, "\t-V:     print version\n" );
	fprintf( stream, "\n" );
}

/* The main program
 */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
int wmain( int argc, wchar_t * const argv[] )
#else
int main( int argc, char * const argv[] )
#endif
{
	char destination[ 128 ];

	libcerror_error_t *error      = NULL;
	libcfile_file_t *source_file  = NULL;
	system_character_t *source    = NULL;
	uint8_t *buffer               = NULL;
	uint8_t *uncompressed_data    = NULL;
	char *program                 = "ascii7decompress";
	system_integer_t option       = 0;
	size64_t source_size          = 0;
	size_t buffer_size            = 0;
	size_t uncompressed_data_size = 0;
	ssize_t read_count            = 0;
	off_t source_offset           = 0;
	int print_count               = 0;
	int verbose                   = 0;

	assorted_output_version_fprint(
	 stdout,
	 program );

	while( ( option = assorted_getopt(
	                   argc,
	                   argv,
	                   _SYSTEM_STRING( "ho:s:vV" ) ) ) != (system_integer_t) -1 )
	{
		switch( option )
		{
			case '?':
			default:
				fprintf(
				 stderr,
				 "Invalid argument: %" PRIs_SYSTEM "\n",
				 argv[ optind ] );

				usage_fprint(
				 stdout );

				return( EXIT_FAILURE );

			case 'h':
				usage_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			case 'o':
				source_offset = atol( optarg );

				break;

			case 's':
				source_size = atol( optarg );

				break;

			case 'v':
				verbose = 1;

				break;

			case 'V':
				assorted_output_copyright_fprint(
				 stdout );

				return( EXIT_SUCCESS );
		}
	}
	if( optind == argc )
	{
		fprintf(
		 stderr,
		 "Missing source file.\n" );

		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	source = argv[ optind ];

	libcnotify_stream_set(
	 stderr,
	 NULL );
	libcnotify_verbose_set(
	 verbose );

	/* Open the source file
	 */
	if( libcfile_file_initialize(
	     &source_file,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to create source file.\n" );

		goto on_error;
	}
	if( libcfile_file_open(
	     source_file,
	     source,
	     LIBCFILE_OPEN_READ,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to open source file.\n" );

		goto on_error;
	}
	if( source_size == 0 )
	{
		if( libcfile_file_get_size(
		     source_file,
		     &source_size,
		     &error ) == -1 )
		{
			fprintf(
			 stderr,
			 "Unable to determine size of source file.\n" );

			goto on_error;
		}
	}
	if( source_size == 0 )
	{
		fprintf(
		 stderr,
		 "Invalid source size value is zero.\n" );

		goto on_error;
	}
	if( source_size > (size_t) SSIZE_MAX )
	{
		fprintf(
		 stderr,
		 "Invalid source size value exceeds maximum.\n" );

		goto on_error;
	}
	/* Create the input buffer
	 */
	buffer_size = source_size;

	buffer = (uint8_t *) memory_allocate(
	                      sizeof( uint8_t ) * buffer_size );

	if( buffer == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to create buffer.\n" );

		return( EXIT_FAILURE );
	}
	uncompressed_data_size = 1 + ( ( ( source_size - 1 ) * 8 ) / 7 );

	uncompressed_data = (uint8_t *) memory_allocate(
	                                 sizeof( uint8_t ) * uncompressed_data_size );

	if( uncompressed_data == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to create uncompressed data buffer.\n" );

		return( EXIT_FAILURE );
	}
	/* Position the source file at the right offset
	 */
	if( libcfile_file_seek_offset(
	     source_file,
	     source_offset,
	     SEEK_SET,
	     &error ) == -1 )
	{
		fprintf(
		 stderr,
		 "Unable to seek offset in source file.\n" );

		goto on_error;
	}
	print_count = narrow_string_snprintf(
	               destination,
	               128,
	               "%s.ascii7decompressed",
	               source );

	if( ( print_count < 0 )
	 || ( print_count > 128 ) )
	{
		fprintf(
		 stderr,
		 "Unable to set destination filename.\n" );

		memory_free(
		 uncompressed_data );
		memory_free(
		 buffer );

		return( EXIT_FAILURE );
	}
	fprintf(
	 stdout,
	 "Starting 7-bit ASCII decompression of: %" PRIs_SYSTEM " at offset: %" PRIjd " (0x%08" PRIjx ").\n",
	 source,
	 source_offset,
	 source_offset );

	read_count = libcfile_file_read_buffer(
		      source_file,
		      buffer,
		      source_size,
	              &error );

	if( read_count != (ssize_t) source_size )
	{
		fprintf(
		 stderr,
		 "Unable to read from source file.\n" );

		goto on_error;
	}
	/* Decompress the data
	 */
	fprintf(
	 stderr,
	 "Compressed data:\n" );

	libcnotify_print_data(
	 buffer,
	 source_size,
	 0 );

	if( ascii7_decompress(
	     uncompressed_data,
	     uncompressed_data_size,
	     buffer,
	     source_size,
	     NULL ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to decompress data.\n" );

		goto on_error;
	}
	fprintf(
	 stderr,
	 "Uncompressed data:\n" );

	libcnotify_print_data(
	 uncompressed_data,
	 uncompressed_data_size,
	 0 );

	source_offset += source_size;
	source_size   -= source_size;

#ifdef NOWRITE
	libcfile_file_t *destination_file = NULL;
	ssize_t write_count               = 0;

	if( destination_file == NULL )
	{
		/* Open the destination file
		 */
		if( libcfile_file_initialize(
		     &destination_file,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to create destination file.\n" );

			goto on_error;
		}
		if( libcfile_file_open(
		     destination_file,
		     destination,
		     LIBCFILE_OPEN_WRITE,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to open destination file.\n" );

			goto on_error;
		}
	}
	write_count = libcfile_file_write_buffer(
		       destination_file,
		       uncompressed_data,
		       uncompressed_data_size,
		       &error );

	if( write_count != (ssize_t) uncompressed_data_size )
	{
		fprintf(
		 stderr,
		 "Unable to write to destination file.\n" );

		goto on_error;
	}
	/* Clean up
	 */
	if( destination_file != NULL )
	{
		if( libcfile_file_close(
		     destination_file,
		     &error ) != 0 )
		{
			fprintf(
			 stderr,
			 "Unable to close destination file.\n" );

			goto on_error;
		}
		if( libcfile_file_free(
		     &destination_file,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to free destination file.\n" );

			goto on_error;
		}
	}
#endif
	/* Clean up
	 */
	if( libcfile_file_close(
	     source_file,
	     &error ) != 0 )
	{
		fprintf(
		 stderr,
		 "Unable to close source file.\n" );

		goto on_error;
	}
	if( libcfile_file_free(
	     &source_file,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to free source file.\n" );

		goto on_error;
	}
	fprintf(
	 stdout,
	 "7-bit ASCII decompression:\tSUCCESS\n" );

	memory_free(
	 uncompressed_data );

	memory_free(
	 buffer );

	return( EXIT_SUCCESS );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
#ifdef NOWRITE
	if( destination_file != NULL )
	{
		libcfile_file_free(
		 &destination_file,
		 NULL );
	}
#endif
	if( uncompressed_data != NULL )
	{
		memory_free(
		 uncompressed_data );
	}
	if( buffer != NULL )
	{
		memory_free(
		 buffer );
	}
	if( source_file != NULL )
	{
		libcfile_file_free(
		 &source_file,
		 NULL );
	}
	return( EXIT_FAILURE );
}

