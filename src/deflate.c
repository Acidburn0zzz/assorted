/*
 * Deflate (zlib) (un)compression functions
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
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include "assorted_libcerror.h"
#include "assorted_libcnotify.h"
#include "deflate.h"

/* TODO use memory alignment in bit stream */

/* Retrieves a value from the bit stream
 * Returns 1 on success or -1 on error
 */
int deflate_bit_stream_get_value(
     deflate_bit_stream_t *bit_stream,
     uint8_t number_of_bits,
     uint32_t *value_32bit,
     libcerror_error_t **error )
{
	static char *function     = "deflate_bit_stream_get_value";
	uint32_t safe_value_32bit = 0;

	if( bit_stream == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid bit stream.",
		 function );

		return( -1 );
	}
	if( number_of_bits > (uint8_t) 32 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid number of bits value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( value_32bit == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid 32-bit value.",
		 function );

		return( -1 );
	}
	if( number_of_bits == 0 )
	{
		*value_32bit = 0;

		return( 1 );
	}
	while( bit_stream->bit_buffer_size < number_of_bits )
	{
		if( bit_stream->byte_stream_offset >= bit_stream->byte_stream_size )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
			 "%s: invalid byte stream value to small.",
			 function );

			return( -1 );
		}
		safe_value_32bit   = bit_stream->byte_stream[ bit_stream->byte_stream_offset++ ];
		safe_value_32bit <<= bit_stream->bit_buffer_size;

		bit_stream->bit_buffer      |= safe_value_32bit;
		bit_stream->bit_buffer_size += 8;
	}
	safe_value_32bit = bit_stream->bit_buffer;

	if( number_of_bits < 32 )
	{
		/* On VS 2008 32-bit "~( 0xfffffffUL << 32 )" does not behave as expected
		 */
		safe_value_32bit &= ~( 0xffffffffUL << number_of_bits );

		bit_stream->bit_buffer     >>= number_of_bits;
		bit_stream->bit_buffer_size -= number_of_bits;
	}
	else
	{
		bit_stream->bit_buffer      = 0;
		bit_stream->bit_buffer_size = 0;
	}
	*value_32bit = safe_value_32bit;

	return( 1 );
}

/* Constructs the Huffman table
 * Returns 1 on success, 0 if the table is empty or -1 on error
 */
int deflate_huffman_table_construct(
     deflate_huffman_table_t *table,
     const uint16_t *code_sizes_array,
     int number_of_code_sizes,
     libcerror_error_t **error )
{
	int code_offsets_array[ 16 ];

	static char *function = "deflate_huffman_table_construct";
	uint16_t code_size    = 0;
	uint8_t bit_index     = 0;
	int code_offset       = 0;
	int left_value        = 0;
	int symbol            = 0;

	if( table == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid table.",
		 function );

		return( -1 );
	}
	if( code_sizes_array == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid code sizes array.",
		 function );

		return( -1 );
	}
	if( number_of_code_sizes < 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid number of code sizes value out of bounds.",
		 function );

		return( -1 );
	}
/* TODO hardcoded for now */
	table->maximum_number_of_bits = 15;

	if( table->maximum_number_of_bits > 15 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid table - number of bits values out of bounds.",
		 function );

		return( -1 );
	}
	table->number_of_codes = (int) table->maximum_number_of_bits + 1;

	if( memory_set(
	     &( table->codes_array ),
	     0,
	     288 * sizeof( int ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear code counts array.",
		 function );

		return( -1 );
	}
	if( memory_set(
	     &( table->code_counts_array ),
	     0,
	     16 * sizeof( int ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear code counts array.",
		 function );

		return( -1 );
	}
	for( symbol = 0;
	     symbol < number_of_code_sizes;
	     symbol++ )
	{
		code_size = code_sizes_array[ symbol ];

		if( code_size > table->number_of_codes )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid symbol: %d code size: %" PRIu16 " value out of bounds.",
			 function,
			 symbol,
			 code_size );

			return( -1 );
		}
		table->code_counts_array[ code_size ] += 1;
	}
	/* The table has no codes
	 */
	if( table->code_counts_array[ 0 ] == number_of_code_sizes )
	{
		return( 0 );
	}
	/* Check if the set of code sizes is incomplete or over-subscribed
	 */
	left_value = 1;

	for( bit_index = 1;
	     bit_index <= table->maximum_number_of_bits;
	     bit_index++ )
	{
		left_value <<= 1;
		left_value  -= table->code_counts_array[ bit_index ];

		if( left_value < 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: code sizes are over-subscribed.",
			 function );

			return( -1 );
		}
	}
	/* Calculate the offsets for sorting the symbol table
	 */
	code_offsets_array[ 0 ] = 0;
	code_offsets_array[ 1 ] = 0;

	for( bit_index = 1;
	     bit_index < table->maximum_number_of_bits;
	     bit_index++ )
	{
		code_offsets_array[ bit_index + 1 ] = code_offsets_array[ bit_index ]
		                                    + table->code_counts_array[ bit_index ];
	}
	for( symbol = 0;
	     symbol < number_of_code_sizes;
	     symbol++ )
	{
		code_size = code_sizes_array[ symbol ];

		if( code_size == 0 )
		{
			continue;
		}
		code_offset = code_offsets_array[ code_size ];

		if( ( code_offset < 0 )
		 || ( code_offset > number_of_code_sizes ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid symbol: %d code offset: %d value out of bounds.",
			 function,
			 symbol,
			 code_offset );

			return( -1 );
		}
		code_offsets_array[ code_size ]  += 1;
		table->codes_array[ code_offset ] = symbol;
	}
/* TODO only used by dynamic Huffman
	if( left_value > 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: code sizes are incomplete.",
		 function );

		return( -1 );
	}
*/
	return( 1 );
}

/* Retrieves a Huffman encoded value from the bit stream
 * Returns 1 on success or -1 on error
 */
int deflate_bit_stream_get_huffman_encoded_value(
     deflate_bit_stream_t *bit_stream,
     deflate_huffman_table_t *table,
     uint32_t *value_32bit,
     libcerror_error_t **error )
{
	static char *function     = "deflate_bit_stream_get_huffman_encoded_value";
	uint32_t bit_buffer       = 0;
	uint32_t safe_value_32bit = 0;
	uint8_t bit_index         = 0;
	uint8_t number_of_bits    = 0;
	int code_size_count       = 0;
	int first_huffman_code    = 0;
	int first_index           = 0;
	int huffman_code          = 0;
	int result                = 0;

	if( bit_stream == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid bit stream.",
		 function );

		return( -1 );
	}
	if( table == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid table.",
		 function );

		return( -1 );
	}
	if( value_32bit == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid 32-bit value.",
		 function );

		return( -1 );
	}
	/* Try to fill the bit buffer with the maximum number of bits
	 */
	while( bit_stream->bit_buffer_size < table->maximum_number_of_bits )
	{
		if( bit_stream->byte_stream_offset >= bit_stream->byte_stream_size )
		{
			break;
		}
		safe_value_32bit   = bit_stream->byte_stream[ bit_stream->byte_stream_offset++ ];
		safe_value_32bit <<= bit_stream->bit_buffer_size;

		bit_stream->bit_buffer      |= safe_value_32bit;
		bit_stream->bit_buffer_size += 8;
	}
	if( table->maximum_number_of_bits < bit_stream->bit_buffer_size )
	{
		number_of_bits = table->maximum_number_of_bits;
	}
	else
	{
		number_of_bits = bit_stream->bit_buffer_size;
	}
	bit_buffer = bit_stream->bit_buffer;

	for( bit_index = 1;
	     bit_index <= number_of_bits;
	     bit_index++ )
	{
		huffman_code <<= 1;
		huffman_code  |= (int) bit_buffer & 0x00000001UL;
		bit_buffer   >>= 1;

		code_size_count = table->code_counts_array[ bit_index ];

		if( ( huffman_code - code_size_count ) < first_huffman_code )
		{
			safe_value_32bit = table->codes_array[ first_index + ( huffman_code - first_huffman_code ) ];

			result = 1;

			break;
		}
		first_huffman_code  += code_size_count;
		first_huffman_code <<= 1;
		first_index         += code_size_count;
	}
	if( result != 0 )
	{
		bit_stream->bit_buffer     >>= bit_index;
		bit_stream->bit_buffer_size -= bit_index;
	}
	if( result != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid huffman encoded value.",
		 function );

		return( -1 );
	}
	*value_32bit = safe_value_32bit;

	return( 1 );
}

/* Initializes the dynamic Huffman tables
 * Returns 1 on success or -1 on error
 */
int deflate_initialize_dynamic_huffman_tables(
     deflate_bit_stream_t *bit_stream,
     deflate_huffman_table_t *literals_table,
     deflate_huffman_table_t *distances_table,
     libcerror_error_t **error )
{
	uint16_t code_size_array[ 316 ];

	uint8_t code_sizes_sequence[ 19 ] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2,
	        14, 1, 15 };

	deflate_huffman_table_t codes_table;

	static char *function             = "deflate_initialize_dynamic_huffman_tables";
	uint32_t code_size                = 0;
	uint32_t code_size_index          = 0;
	uint32_t code_size_sequence       = 0;
	uint32_t number_of_code_sizes     = 0;
	uint32_t number_of_distance_codes = 0;
	uint32_t number_of_literal_codes  = 0;
	uint32_t symbol                   = 0;
	uint32_t times_to_repeat          = 0;

	if( deflate_bit_stream_get_value(
	     bit_stream,
	     14,
	     &number_of_code_sizes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve value from bit stream.",
		 function );

		return( -1 );
	}
	number_of_literal_codes  = number_of_code_sizes & 0x0000001fUL;
	number_of_code_sizes   >>= 5;
	number_of_distance_codes = number_of_code_sizes & 0x0000001fUL;
	number_of_code_sizes   >>= 5;

	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: number of literal codes\t\t: %" PRIu32 " (0x%02" PRIx32 ")\n",
		 function,
		 number_of_literal_codes + 257,
		 number_of_literal_codes );

		libcnotify_printf(
		 "%s: number of distance codes\t\t: %" PRIu32 " (0x%02" PRIx32 ")\n",
		 function,
		 number_of_distance_codes + 1,
		 number_of_distance_codes );

		libcnotify_printf(
		 "%s: number of code sizes\t\t\t: %" PRIu32 " (0x%02" PRIx32 ")\n",
		 function,
		 number_of_code_sizes + 4,
		 number_of_code_sizes );
	}
	number_of_literal_codes += 257;

	if( number_of_literal_codes > 286 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid number of literal codes value out of bounds.",
		 function );

		return( -1 );
	}
	number_of_distance_codes += 1;

	if( number_of_distance_codes > 30 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid number of distance codes value out of bounds.",
		 function );

		return( -1 );
	}
	number_of_code_sizes += 4;

	for( code_size_index = 0;
	     code_size_index < number_of_code_sizes;
	     code_size_index++ )
	{
		if( deflate_bit_stream_get_value(
		     bit_stream,
		     3,
		     &code_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve value from bit stream.",
			 function );

			return( -1 );
		}
		code_size_sequence = code_sizes_sequence[ code_size_index ];

		code_size_array[ code_size_sequence ] = (uint16_t) code_size;

		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: code size: % 2" PRIu8 "\t\t\t: %" PRIu32 "\n",
			 function,
			 code_size_sequence,
			 code_size );
		}
	}
	while( code_size_index < 19 )
	{
		code_size_sequence = code_sizes_sequence[ code_size_index++ ];

		code_size_array[ code_size_sequence ] = 0;

		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: code size: % 2" PRIu8 "\t\t\t: 0\n",
			 function,
			 code_size_sequence );
		}
	}
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "\n" );
	}
	if( deflate_huffman_table_construct(
	     &codes_table,
	     code_size_array,
	     19,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct codes table.",
		 function );

		return( -1 );
	}
	number_of_code_sizes = number_of_literal_codes + number_of_distance_codes;

	code_size_index = 0;

	while( code_size_index < number_of_code_sizes )
	{
		if( deflate_bit_stream_get_huffman_encoded_value(
		     bit_stream,
		     &codes_table,
		     &symbol,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve literal value from bit stream.",
			 function );

			return( -1 );
		}
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: code value: % 3" PRIu32 "\t\t\t: %" PRIu32 "\n",
			 function,
			 code_size_index,
			 symbol );
		}
		if( symbol < 16 )
		{
			code_size_array[ code_size_index++ ] = (uint16_t) symbol;

			continue;
		}
		code_size = 0;

		if( symbol == 16 )
		{
			if( code_size_index == 0 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
				 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid code size index value out of bounds.",
				 function );

				return( -1 );
			}
			code_size = (uint32_t) code_size_array[ code_size_index - 1 ];

			if( deflate_bit_stream_get_value(
			     bit_stream,
			     2,
			     &times_to_repeat,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve value from bit stream.",
				 function );

				return( -1 );
			}
			times_to_repeat += 3;
		}
		else if( symbol == 17 )
		{
			if( deflate_bit_stream_get_value(
			     bit_stream,
			     3,
			     &times_to_repeat,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve value from bit stream.",
				 function );

				return( -1 );
			}
			times_to_repeat += 3;
		}
		else if( symbol == 18 )
		{
			if( deflate_bit_stream_get_value(
			     bit_stream,
			     7,
			     &times_to_repeat,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve value from bit stream.",
				 function );

				return( -1 );
			}
			times_to_repeat += 11;
		}
		else
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid code value value out of bounds.",
			 function );

			return( -1 );
		}
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: times to repeat\t\t\t: %" PRIu32 "\n",
			 function,
			 times_to_repeat );
		}
		if( ( code_size_index + times_to_repeat ) > number_of_code_sizes )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid times to repeat value out of bounds.",
			 function );

			return( -1 );
		}
		while( times_to_repeat > 0 )
		{
			code_size_array[ code_size_index++ ] = (uint16_t) code_size;

			times_to_repeat--;
		}
	}
	if( code_size_array[ 256 ] == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: end-of-block code value missing in literal codes array.",
		 function );

		return( -1 );
	}
	if( deflate_huffman_table_construct(
	     literals_table,
	     code_size_array,
	     number_of_literal_codes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct literals table.",
		 function );

		return( -1 );
	}
	if( deflate_huffman_table_construct(
	     distances_table,
	     &( code_size_array[ number_of_literal_codes ] ),
	     number_of_distance_codes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct distances table.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Initializes the fixed Huffman tables
 * Returns 1 on success or -1 on error
 */
int deflate_initialize_fixed_huffman_tables(
     deflate_huffman_table_t *literals_table,
     deflate_huffman_table_t *distances_table,
     libcerror_error_t **error )
{
	uint16_t code_size_array[ 318 ];

	static char *function = "deflate_initialize_fixed_huffman_tables";
	uint16_t symbol       = 0;

	for( symbol = 0;
	     symbol < 318;
	     symbol++ )
	{
		if( symbol < 144 )
		{
			code_size_array[ symbol ] = 8;
		}
		else if( symbol < 256 )
		{
			code_size_array[ symbol ] = 9;
		}
		else if( symbol < 280 )
		{
			code_size_array[ symbol ] = 7;
		}
		else if( symbol < 288 )
		{
			code_size_array[ symbol ] = 8;
		}
		else
		{
			code_size_array[ symbol ] = 5;
		}
	}
	if( deflate_huffman_table_construct(
	     literals_table,
	     code_size_array,
	     288,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct literals table.",
		 function );

		return( -1 );
	}
	if( deflate_huffman_table_construct(
	     distances_table,
	     &( code_size_array[ 288 ] ),
	     30,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct distances table.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Decodes a Huffman compressed block
 * Returns 1 on success or -1 on error
 */
int deflate_decode_huffman(
     deflate_bit_stream_t *bit_stream,
     deflate_huffman_table_t *literals_table,
     deflate_huffman_table_t *distances_table,
     uint8_t *uncompressed_data,
     size_t uncompressed_data_size,
     size_t *uncompressed_data_offset,
     libcerror_error_t **error )
{
	uint16_t literal_codes_base[ 29 ] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };

	uint16_t literal_codes_number_of_extra_bits[ 29 ] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

	uint16_t distance_codes_base[ 30 ] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193,
		12289, 16385, 24577};

	uint16_t distance_codes_number_of_extra_bits[ 30 ] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

	static char *function         = "deflate_decode_huffman";
	size_t data_offset            = 0;
	uint32_t code_value           = 0;
	uint32_t extra_bits           = 0;
	uint16_t compression_offset   = 0;
	uint16_t compression_size     = 0;
	uint16_t number_of_extra_bits = 0;

	if( uncompressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data.",
		 function );

		return( -1 );
	}
	if( uncompressed_data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid uncompressed data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( uncompressed_data_offset == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data offset.",
		 function );

		return( -1 );
	}
	data_offset = *uncompressed_data_offset;

	do
	{
		if( deflate_bit_stream_get_huffman_encoded_value(
		     bit_stream,
		     literals_table,
		     &code_value,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve listeral value from bit stream.",
			 function );

			return( -1 );
		}
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: code value\t\t\t\t\t\t: 0x%04" PRIx16 "\n",
			 function,
			 code_value );
		}
		if( code_value < 256 )
		{
			if( data_offset >= uncompressed_data_size )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
				 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
				 "%s: invalid uncompressed data value too small.",
				 function );

				return( -1 );
			}
			uncompressed_data[ data_offset++ ] = (uint8_t) code_value;
		}
		else if( ( code_value > 256 )
		      && ( code_value < 286 ) )
		{
			code_value -= 257;

			number_of_extra_bits = literal_codes_number_of_extra_bits[ code_value ];

			if( deflate_bit_stream_get_value(
			     bit_stream,
			     (uint8_t) number_of_extra_bits,
			     &extra_bits,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve literal extra value from bit stream.",
				 function );

				return( -1 );
			}
			if( libcnotify_verbose != 0 )
			{
				libcnotify_printf(
				 "%s: literal code\t\t\t\t\t\t: %" PRIu16 "\n",
				 function,
				 literal_codes_base[ code_value ] );

				libcnotify_printf(
				 "%s: extra bits\t\t\t\t\t\t: 0x%04" PRIx16 "\n",
				 function,
				 extra_bits );
			}
			compression_size = literal_codes_base[ code_value ] + (uint16_t) extra_bits;

			if( deflate_bit_stream_get_huffman_encoded_value(
			     bit_stream,
			     distances_table,
			     &code_value,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve distance value from bit stream.",
				 function );

				return( -1 );
			}
			if( libcnotify_verbose != 0 )
			{
				libcnotify_printf(
				 "%s: code value\t\t\t\t\t\t: 0x%04" PRIx16 "\n",
				 function,
				 code_value );
			}
			number_of_extra_bits = distance_codes_number_of_extra_bits[ code_value ];

			if( deflate_bit_stream_get_value(
			     bit_stream,
			     (uint8_t) number_of_extra_bits,
			     &extra_bits,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve distance extra value from bit stream.",
				 function );

				return( -1 );
			}
			if( libcnotify_verbose != 0 )
			{
				libcnotify_printf(
				 "%s: distance code\t\t\t\t\t\t: %" PRIu16 "\n",
				 function,
				 distance_codes_base[ code_value ] );

				libcnotify_printf(
				 "%s: extra bits\t\t\t\t\t\t: 0x%04" PRIx16 "\n",
				 function,
				 extra_bits );
			}
			compression_offset = distance_codes_base[ code_value ] + (uint16_t) extra_bits;

			if( libcnotify_verbose != 0 )
			{
				libcnotify_printf(
				 "%s: uncompressed data offset\t\t\t\t: %" PRIzd "\n",
				 function,
				 data_offset );

				libcnotify_printf(
				 "%s: compression offset\t\t\t\t\t: %" PRIu16 "\n",
				 function,
				 compression_offset );

				libcnotify_printf(
				 "%s: compression size\t\t\t\t\t: %" PRIu16 "\n",
				 function,
				 compression_size );
			}
			if( compression_offset > data_offset )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid compression offset value out of bounds.",
				 function );

				return( -1 );
			}
			if( ( data_offset + compression_size ) > uncompressed_data_size )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
				 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
				 "%s: invalid uncompressed data value too small.",
				 function );

				return( -1 );
			}
			while( compression_size > 0 )
			{
				uncompressed_data[ data_offset ] = uncompressed_data[ data_offset - compression_offset ];

				data_offset++;
				compression_size--;
			}
		}
		else if( code_value != 256 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: invalid code value: %" PRIu16 ".",
			 function,
			 code_value );

			return( -1 );
		}
	}
	while( code_value != 256 );

	*uncompressed_data_offset = data_offset;

	return( 1 );
}

/* Calculates the little-endian Adler-32 of a buffer
 * It uses the initial value to calculate a new Adler-32
 * Returns 1 if successful or -1 on error
 */
int deflate_calculate_adler32(
     uint32_t *checksum_value,
     const uint8_t *buffer,
     size_t size,
     uint32_t initial_value,
     libcerror_error_t **error )
{
	static char *function = "deflate_calculate_adler32";
	size_t buffer_offset  = 0;
	uint32_t lower_word   = 0;
	uint32_t upper_word   = 0;
	uint32_t value_32bit  = 0;
	int block_index       = 0;

	if( checksum_value == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid checksum value.",
		 function );

		return( -1 );
	}
	if( buffer == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid buffer.",
		 function );

		return( -1 );
	}
	if( size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid size value exceeds maximum.",
		 function );

		return( -1 );
	}
	lower_word = initial_value & 0xffff;
	upper_word = ( initial_value >> 16 ) & 0xffff;

	while( size >= 0x15b0 )
	{
		/* The modulo calculation is needed per 5552 (0x15b0) bytes
		 * 5552 / 16 = 347
		 */
		for( block_index = 0;
		     block_index < 347;
		     block_index++ )
		{
			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;
		}
		/* Optimized equivalent of:
		 * lower_word %= 0xfff1
		 */
		value_32bit = lower_word >> 16;
		lower_word &= 0x0000ffffUL;
		lower_word += ( value_32bit << 4 ) - value_32bit;

		if( lower_word > 65521 )
		{
			value_32bit = lower_word >> 16;
			lower_word &= 0x0000ffffUL;
			lower_word += ( value_32bit << 4 ) - value_32bit;
		}
		if( lower_word >= 65521 )
		{
			lower_word -= 65521;
		}
		/* Optimized equivalent of:
		 * upper_word %= 0xfff1
		 */
		value_32bit = upper_word >> 16;
		upper_word &= 0x0000ffffUL;
		upper_word += ( value_32bit << 4 ) - value_32bit;

		if( upper_word > 65521 )
		{
			value_32bit = upper_word >> 16;
			upper_word &= 0x0000ffffUL;
			upper_word += ( value_32bit << 4 ) - value_32bit;
		}
		if( upper_word >= 65521 )
		{
			upper_word -= 65521;
		}
		size -= 0x15b0;
	}
	if( size > 0 )
	{
		while( size > 16 )
		{
			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			size -= 16;
		}
		while( size > 0 )
		{
			lower_word += buffer[ buffer_offset++ ];
			upper_word += lower_word;

			size--;
		}
		/* Optimized equivalent of:
		 * lower_word %= 0xfff1
		 */
		value_32bit = lower_word >> 16;
		lower_word &= 0x0000ffffUL;
		lower_word += ( value_32bit << 4 ) - value_32bit;

		if( lower_word > 65521 )
		{
			value_32bit = lower_word >> 16;
			lower_word &= 0x0000ffffUL;
			lower_word += ( value_32bit << 4 ) - value_32bit;
		}
		if( lower_word >= 65521 )
		{
			lower_word -= 65521;
		}
		/* Optimized equivalent of:
		 * upper_word %= 0xfff1
		 */
		value_32bit = upper_word >> 16;
		upper_word &= 0x0000ffffUL;
		upper_word += ( value_32bit << 4 ) - value_32bit;

		if( upper_word > 65521 )
		{
			value_32bit = upper_word >> 16;
			upper_word &= 0x0000ffffUL;
			upper_word += ( value_32bit << 4 ) - value_32bit;
		}
		if( upper_word >= 65521 )
		{
			upper_word -= 65521;
		}
	}
	*checksum_value = ( upper_word << 16 ) | lower_word;

	return( 1 );
}

/* Compresses data using zlib compression
 * Returns 1 on success or -1 on error
 */
int deflate_compress(
     const uint8_t *uncompressed_data,
     size_t uncompressed_data_size,
     int compression_level,
     uint8_t *compressed_data,
     size_t *compressed_data_size,
     libcerror_error_t **error )
{
	static char *function = "deflate_compress";

	if( uncompressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data.",
		 function );

		return( -1 );
	}
	if( uncompressed_data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid uncompressed data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( compressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid compressed data.",
		 function );

		return( -1 );
	}
	if( compressed_data_size == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid compressed data size.",
		 function );

		return( -1 );
	}
	if( *compressed_data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid compressed data size value exceeds maximum.",
		 function );

		return( -1 );
	}
/* TODO implement */
	libcerror_error_set(
	 error,
	 LIBCERROR_ERROR_DOMAIN_RUNTIME,
	 LIBCERROR_RUNTIME_ERROR_GENERIC,
	 "%s: NOT IMPLEMENTED YET",
	 function );

	return( -1 );
}

/* TODO split read zlib header and decompress */
/* TODO after split add uint16_t compression_window_size */

/* Decompresses data using zlib compression
 * Returns 1 on success or -1 on error
 */
int deflate_decompress(
     const uint8_t *compressed_data,
     size_t compressed_data_size,
     uint8_t *uncompressed_data,
     size_t *uncompressed_data_size,
     libcerror_error_t **error )
{
	deflate_bit_stream_t bit_stream;
	deflate_huffman_table_t dynamic_huffman_distances_table;
	deflate_huffman_table_t dynamic_huffman_literals_table;
	deflate_huffman_table_t fixed_huffman_distances_table;
	deflate_huffman_table_t fixed_huffman_literals_table;

	static char *function                 = "deflate_decompress";
	size_t compressed_data_offset         = 0;
	size_t uncompressed_data_offset       = 0;
	uint32_t block_size                   = 0;
	uint32_t block_size_copy              = 0;
	uint32_t compression_window_size      = 0;
	uint32_t calculated_checksum          = 0;
	uint32_t preset_dictionary_identifier = 0;
	uint32_t stored_checksum              = 0;
	uint32_t value_32bit                  = 0;
	uint8_t block_type                    = 0;
	uint8_t compression_information       = 0;
	uint8_t compression_method            = 0;
	uint8_t compression_window_bits       = 0;
	uint8_t last_block_flag               = 0;
	uint8_t skip_bits                     = 0;

	if( compressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid compressed data.",
		 function );

		return( -1 );
	}
	if( compressed_data_size < 2 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
		 "%s: invalid compressed data value too small.",
		 function );

		return( -1 );
	}
	if( compressed_data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid compressed data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( uncompressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data.",
		 function );

		return( -1 );
	}
	if( uncompressed_data_size == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data size.",
		 function );

		return( -1 );
	}
	if( *uncompressed_data_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid uncompressed data size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: header data:\n",
		 function );
		libcnotify_print_data(
		 compressed_data,
		 2,
		 0 );
	}
	if( compressed_data_offset >= compressed_data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
		 "%s: invalid compressed data value too small.",
		 function );

		return( -1 );
	}
	compression_method      = compressed_data[ 0 ] & 0x0f;
	compression_information = compressed_data[ 0 ] >> 4;

	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: header compression method\t\t\t\t\t: %" PRIu8 "\n",
		 function,
		 compression_method );

		libcnotify_printf(
		 "%s: header compression information\t\t\t\t: %" PRIu8 "\n",
		 function,
		 compression_information );

		libcnotify_printf(
		 "%s: header check bits\t\t\t\t\t\t: 0x%02" PRIx8 "\n",
		 function,
		 compressed_data[ 1 ] & 0x1f );

		libcnotify_printf(
		 "%s: header preset dictionary flag\t\t\t\t: %" PRIu8 "\n",
		 function,
		 ( compressed_data[ 1 ] >> 5 ) & 0x01 );

		libcnotify_printf(
		 "%s: header compression level\t\t\t\t\t: %" PRIu8 " (",
		 function,
		 compressed_data[ 1 ] >> 6 );

		switch( compressed_data[ 1 ] >> 6 )
		{
			case 0:
				libcnotify_printf(
				 "Fastest" );
				break;

			case 1:
				libcnotify_printf(
				 "Fast" );
				break;

			case 2:
				libcnotify_printf(
				 "Default" );
				break;

			case 3:
			default:
				libcnotify_printf(
				 "Slow/Maximum" );
				break;
		}
		libcnotify_printf(
		 ")\n" );
	}
/* TODO validate check bits */
	if( ( compressed_data[ 1 ] & 0x20 ) != 0 )
	{
		if( compressed_data_size < 6 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
			 "%s: invalid compressed data value too small.",
			 function );

			return( -1 );
		}
		byte_stream_copy_to_uint32_big_endian(
		 &( compressed_data[ 2 ] ),
		 preset_dictionary_identifier );

		compressed_data_offset += 4;
		compressed_data_size   -= 4;

		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: header preset dictionary identifier\t\t\t: 0x%08" PRIx32 "\n",
			 function,
			 preset_dictionary_identifier );
		}
	}
	compressed_data_offset += 2;
	compressed_data_size   -= 2;

	if( compression_method != 8 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported compression method: %" PRIu8 ".",
		 function,
		 compression_method );

		return( -1 );
	}
	compression_window_bits = (uint8_t) compression_information + 8;
	compression_window_size = 1UL << compression_window_bits;

	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: header compression window size\t\t\t\t: %" PRIu32 " (%" PRIu8 ")\n",
		 function,
		 compression_window_size,
		 compression_window_bits );
	}
	if( compression_window_size > 32768 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported compression window size: %" PRIu32 ".",
		 function,
		 compression_window_size );

		return( -1 );
	}
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "\n" );
	}
	if( compressed_data_offset >= compressed_data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
		 "%s: invalid compressed data value too small.",
		 function );

		return( -1 );
	}
	bit_stream.byte_stream        = compressed_data;
	bit_stream.byte_stream_size   = compressed_data_size;
	bit_stream.byte_stream_offset = compressed_data_offset;
	bit_stream.bit_buffer         = 0;
	bit_stream.bit_buffer_size    = 0;

	if( deflate_initialize_fixed_huffman_tables(
	     &fixed_huffman_literals_table,
	     &fixed_huffman_distances_table,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to construct fixed Huffman tables.",
		 function );

		return( -1 );
	}

/* TODO find optimized solution to read bit stream from bytes */
	while( bit_stream.byte_stream_offset < bit_stream.byte_stream_size )
	{
		if( deflate_bit_stream_get_value(
		     &bit_stream,
		     3,
		     &value_32bit,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve value from bit stream.",
			 function );

			return( -1 );
		}
		last_block_flag = (uint8_t) ( value_32bit & 0x00000001UL );
		value_32bit   >>= 1;
		block_type      = (uint8_t) value_32bit;

		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: block header last block flag\t\t\t\t: %" PRIu8 "\n",
			 function,
			 last_block_flag );

			libcnotify_printf(
			 "%s: block header block type\t\t\t\t\t: %" PRIu8 " (",
			 function,
			 block_type );

			switch( block_type )
			{
				case DEFLATE_BLOCK_TYPE_UNCOMPRESSED:
					libcnotify_printf(
					 "Uncompressed" );
					break;

				case DEFLATE_BLOCK_TYPE_HUFFMAN_FIXED:
					libcnotify_printf(
					 "Fixed Huffman" );
					break;

				case DEFLATE_BLOCK_TYPE_HUFFMAN_DYNAMIC:
					libcnotify_printf(
					 "Dynamic Huffman" );
					break;

				case DEFLATE_BLOCK_TYPE_RESERVED:
				default:
					libcnotify_printf(
					 "Reserved" );
					break;
			}
			libcnotify_printf(
			 ")\n" );

			libcnotify_printf(
			 "\n" );
		}
		switch( block_type )
		{
			case DEFLATE_BLOCK_TYPE_UNCOMPRESSED:
				/* Ignore the bits in the buffer upto the next byte
				 */
				skip_bits = bit_stream.bit_buffer_size & 0x07;

				if( libcnotify_verbose != 0 )
				{
					libcnotify_printf(
					 "%s: skip bits\t\t\t\t\t\t\t: %" PRIu8 "\n",
					 function,
					 skip_bits );
				}
				if( skip_bits > 0 )
				{
					if( deflate_bit_stream_get_value(
					     &bit_stream,
					     skip_bits,
					     &value_32bit,
					     error ) != 1 )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
						 "%s: unable to retrieve value from bit stream.",
						 function );

						return( -1 );
					}
				}
				if( deflate_bit_stream_get_value(
				     &bit_stream,
				     32,
				     &block_size,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve value from bit stream.",
					 function );

					return( -1 );
				}
				block_size_copy = block_size >> 16;
				block_size     &= 0x0000ffffUL;

				if( libcnotify_verbose != 0 )
				{
					libcnotify_printf(
					 "%s: block header unknown1\t\t\t\t\t: %" PRIu32 "\n",
					 function,
					 value_32bit );

					libcnotify_printf(
					 "%s: block header block size\t\t\t\t\t: %" PRIu32 "\n",
					 function,
					 block_size );

					libcnotify_printf(
					 "%s: block header block size copy\t\t\t\t: %" PRIu16 " (%" PRIu32 ")\n",
					 function,
					 block_size_copy ^ 0x0000ffffUL,
					 block_size_copy );
				}
				block_size_copy = ( block_size >> 16 ) ^ 0x0000ffffUL;

				if( block_size != block_size_copy )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_INPUT,
					 LIBCERROR_INPUT_ERROR_VALUE_MISMATCH,
					 "%s: mismatch in block size ( %" PRIu32 " != %" PRIu32 " ).",
					 function,
					 block_size,
					 block_size_copy );

					return( -1 );
				}
				if( block_size == 0 )
				{
					break;
				}
				if( (size_t) block_size > ( bit_stream.byte_stream_size - bit_stream.byte_stream_offset ) )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
					 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
					 "%s: invalid compressed data value too small.",
					 function );

					return( -1 );
				}
				if( (size_t) block_size > ( *uncompressed_data_size - uncompressed_data_offset ) )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
					 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
					 "%s: invalid uncompressed data value too small.",
					 function );

					return( -1 );
				}
				if( memory_copy(
				     &( uncompressed_data[ uncompressed_data_offset ] ),
				     &( compressed_data[ bit_stream.byte_stream_offset ] ),
				     (size_t) block_size ) == NULL )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_MEMORY,
					 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
					 "%s: unable to initialize lz buffer.",
					 function );

					return( -1 );
				}
				bit_stream.byte_stream_offset += block_size;
				uncompressed_data_offset      += block_size;

				/* Flush the bit-stream buffer
				 */
				bit_stream.bit_buffer      = 0;
				bit_stream.bit_buffer_size = 0;

				break;

			case DEFLATE_BLOCK_TYPE_HUFFMAN_FIXED:
				if( deflate_decode_huffman(
				     &bit_stream,
				     &fixed_huffman_literals_table,
				     &fixed_huffman_distances_table,
				     uncompressed_data,
				     *uncompressed_data_size,
				     &uncompressed_data_offset,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to decode fixed Huffman encoded bit stream.",
					 function );

					return( -1 );
				}
				break;

			case DEFLATE_BLOCK_TYPE_HUFFMAN_DYNAMIC:
				if( deflate_initialize_dynamic_huffman_tables(
				     &bit_stream,
				     &dynamic_huffman_literals_table,
				     &dynamic_huffman_distances_table,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
					 "%s: unable to construct dynamic Huffman tables.",
					 function );

					return( -1 );
				}
				if( deflate_decode_huffman(
				     &bit_stream,
				     &dynamic_huffman_literals_table,
				     &dynamic_huffman_distances_table,
				     uncompressed_data,
				     *uncompressed_data_size,
				     &uncompressed_data_offset,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to decode dynamic Huffman encoded bit stream.",
					 function );

					return( -1 );
				}
				break;

			case DEFLATE_BLOCK_TYPE_RESERVED:
			default:
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
				 "%s: unsupported block type.",
				 function );

				return( -1 );
		}
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "\n" );
		}
		if( last_block_flag != 0 )
		{
			break;
		}
	}
	if( ( bit_stream.byte_stream_size - bit_stream.byte_stream_offset ) >= 4 )
	{
		while( bit_stream.bit_buffer_size >= 8 )
		{
			bit_stream.byte_stream_offset -= 1;
			bit_stream.bit_buffer_size    -= 8;
		}
		byte_stream_copy_to_uint32_big_endian(
		 &( bit_stream.byte_stream[ bit_stream.byte_stream_offset ] ),
		 stored_checksum );

		if( deflate_calculate_adler32(
		     &calculated_checksum,
		     uncompressed_data,
		     uncompressed_data_offset,
		     1,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to calculate checksum.",
			 function );

			return( -1 );
		}
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: stored checksum\t\t\t\t\t\t: 0x%08" PRIx32 "\n",
			 function,
			 stored_checksum );

			libcnotify_printf(
			 "%s: calculated checksum\t\t\t\t\t\t: 0x%08" PRIx32 "\n",
			 function,
			 calculated_checksum );
		}
		if( stored_checksum != calculated_checksum )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_INPUT,
			 LIBCERROR_INPUT_ERROR_CHECKSUM_MISMATCH,
			 "%s: checksum does not match (stored: 0x%08" PRIx32 ", calculated: 0x%08" PRIx32 ").",
			 function,
			 stored_checksum,
			 calculated_checksum );

			return( -1 );
		}
	}
	*uncompressed_data_size = uncompressed_data_offset;

	return( 1 );
}

#ifdef TODO
	/* Align the compressed data
	 */
	alignment_size = (uint8_t) ( (intptr_t) compressed_data % sizeof( zdecompress_aligned_t ) );

	if( alignment_size != 0 )
	{
		byte_size = alignment_size;

		do
		{
			byte_size--;

			bit_stream <<= 8;
			bit_stream  |= compressed_data[ byte_size ];
		}
		while( byte_size != 0 );

		aligned_compressed_data = (zdecompress_aligned_t *) compressed_data;
	}
	else
	{
		aligned_compressed_data = (zdecompress_aligned_t *) compressed_data;

		bit_stream = *
	}
#endif

