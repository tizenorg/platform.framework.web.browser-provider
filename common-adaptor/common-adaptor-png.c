/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <png.h>

#include <browser-provider-log.h>

typedef struct {
	const png_byte *data;
	png_size_t size;
} bp_image_fmt;

static void __read_png_data_callback(png_structp png_ptr,
	png_byte *raw_data, png_size_t read_length) {
	bp_image_fmt *handle = png_get_io_ptr(png_ptr);
	const png_byte *ptr = handle->data + handle->size;
	memcpy(raw_data, ptr, read_length);
	handle->size += read_length;
}

static void __user_write_data_callback(png_structp png_ptr,
	png_bytep raw_data, png_uint_32 length)
{
	bp_image_fmt *handle = png_get_io_ptr(png_ptr);
	png_byte *ptr = (png_byte *)handle->data;
	memcpy(ptr + handle->size, raw_data, length);
	handle->size += length;
}

int bp_common_raw_to_png(const unsigned char *raw_data, int width,
	int height, unsigned char **png_data, int *length)
{
	if (width > 0 && height > 0 && raw_data != NULL) {

		/* initialize stuff */
		png_structp png_ptr =
			png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
				NULL);
		if (png_ptr == NULL) {
			TRACE_ERROR("png_create_write_struct failed");
			return -1;
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL) {
			TRACE_ERROR("png_create_info_struct failed");
			png_destroy_write_struct(&png_ptr, (png_infopp)0);
			return -1;
		}
		if (setjmp(png_jmpbuf(png_ptr))) {
			TRACE_ERROR("Error during init_io");
			png_destroy_write_struct(&png_ptr, (png_infopp)0);
			return -1;
		}
		png_voidp png_buffer = calloc(width * height, sizeof(png_bytep));
		if (png_buffer == NULL) {
			TRACE_ERROR("malloc failed");
			png_destroy_write_struct(&png_ptr, (png_infopp)0);
			return -1;
		}
		bp_image_fmt write_io_ptr;
		write_io_ptr.data = (png_voidp)png_buffer;
		write_io_ptr.size = 0;
		png_set_write_fn(png_ptr, (png_voidp)&write_io_ptr,
			(png_rw_ptr)__user_write_data_callback, 0);
		png_set_IHDR(png_ptr, info_ptr, width, height, 8,
			PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_set_bgr(png_ptr);
		png_bytep *row_pointers = malloc((size_t)(height * sizeof(png_bytep)));
		int j = 0;
		for (j = 0;j < height; j++)
			row_pointers[j] = (png_bytep)(raw_data + (j * width * 4));
		png_set_rows(png_ptr, info_ptr, (png_bytepp)row_pointers);
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		free(row_pointers);
		*length = write_io_ptr.size;
		*png_data = png_buffer;
		return 0;
	}
	return -1;
}

int bp_common_png_to_raw(const unsigned char *png_data,
	unsigned char **raw_data, int *width, int *height, int *raw_length)
{
	//first 8bytes is for header
	if (png_data == NULL ||
			png_sig_cmp((unsigned char *)png_data, 0, 8) != 0) {
		TRACE_ERROR("not recognized as a PNG");
		return -1;
	}

	/* initialize stuff */
	png_structp png_ptr =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		TRACE_ERROR("png_create_read_struct failed");
		return -1;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		TRACE_ERROR("png_create_info_struct failed");
		png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
		return -1;
	}

	// replace FD STREAM
	bp_image_fmt png_data_handle = (bp_image_fmt) {png_data, 0};
	png_set_read_fn(png_ptr, &png_data_handle, __read_png_data_callback);

	if (setjmp(png_jmpbuf(png_ptr))) {
		TRACE_ERROR("Error during init_io");
		png_destroy_read_struct(&png_ptr, (png_infopp)0, (png_infopp)0);
		return -1;
	}

	int ret = -1;
	int bit_depth, color_type;
	png_uint_32 png_width;
	png_uint_32 png_height;
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height,
		&bit_depth, &color_type, NULL, NULL, NULL);

	/* expand transparency entry -> alpha channel if present */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	/* expand gray (w/reduced bits) -> 8-bit RGB if necessary */
	if ((color_type == PNG_COLOR_TYPE_GRAY) ||
			(color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
		png_set_gray_to_rgb(png_ptr);
		if (bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png_ptr);
	}
	/* expand palette -> RGB if necessary */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	// Add alpha channel, if there is none.
    // Rationale: GL_RGBA is faster than GL_RGB on many GPUs)
    if (color_type == PNG_COLOR_TYPE_PALETTE ||
			color_type == PNG_COLOR_TYPE_RGB) {
		png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
		png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
	/* reduce 16bit color -> 8bit color if necessary */
	if (bit_depth < 8)
		png_set_packing(png_ptr);
	else if (bit_depth > 8)
		png_set_strip_16(png_ptr);
	png_set_bgr(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	const png_size_t row_size = png_get_rowbytes(png_ptr, info_ptr);
	const int raws_buffer_length = row_size * png_height;

	if (raws_buffer_length > 0) {
		png_byte* raws_buffer = malloc(raws_buffer_length);
		if (raws_buffer != NULL) {
			png_uint_32 i;
			png_byte* row_ptrs[png_height];
			for (i = 0; i < png_height; i++)
				row_ptrs[i] = raws_buffer + i * row_size;
			png_read_image(png_ptr, &row_ptrs[0]);
			*raw_data = raws_buffer;
			*raw_length = raws_buffer_length;
			*width = (int)png_width;
			*height = (int)png_height;
			ret = 0;
		}
	}
	png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)0);

	return ret;
}
