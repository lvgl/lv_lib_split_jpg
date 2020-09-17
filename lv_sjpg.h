#ifndef _LV_SJPEG_
#define _LV_SJPEG_

/*----------------------------------------------------------------------------------------------------------------------------------
/    SJPEG is a custom created modified JPEG file format for small embedded platforms.
/    It will contain multiple JPEG fragments all embedded into a single file with a custom header.
/    This makes JPEG decoding easier using any JPEG library. Overall file size will be almost
/    similar to the parent jpeg file. We can generate sjpeg from any jpeg using a python script
/    provided along with this project.			
/                                                                                     (by vinodstanur | 2020 )
/    SJPEG FILE STRUCTURE
/    --------------------------------------------------------------------------------------------------------------------------------
/    Bytes                       |   Value                                                                                           |
/    --------------------------------------------------------------------------------------------------------------------------------
/                                                                                                                                    
/    0 - 7                       |   "_SJPG__" followed by '\0'
/
/    8 - 13                      |   "V1.00" followed by '\0'       [VERSION OF SJPG FILE for future compatibiliby]
/
/    14 - 15                     |   X_RESOLUTION (width)            [little endian]
/
/    16 - 17                     |   Y_RESOLUTION (height)           [little endian]
/
/    18 - 19                     |   TOTAL_FRAMES inside sjpeg       [little endian]
/
/    20 - 21                     |   JPEG BLOCK WIDTH (16 normally)  [little endian]
/
/    22 - [(TOTAL_FRAMES*2 )]    |   SIZE OF EACH JPEG SPLIT FRAGMENTS   (FRAME_INFO_ARRAY)
/
/   SJPEG data                   |   Each JPEG frame can be extracted from SJPEG data by parsing the FRAME_INFO_ARRAY one time.
/
/----------------------------------------------------------------------------------------------------------------------------------
/                   JPEG DECODER
/                   ------------
/	We are using TJpgDec - Tiny JPEG Decompressor library from ELM-CHAN for decoding each split-jpeg fragments.
/	The tjpgd.c and tjpgd.h is not modified and those are used as it is. So if any update comes for the tiny-jpeg,
/	just replace those files with updated files.
/---------------------------------------------------------------------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdio.h>

#define SJPEG_VERSION_OFFSET            8
#define SJPEG_X_RES_OFFSET              14
#define SJPEG_y_RES_OFFSET              16
#define SJPEG_TOTAL_FRAMES_OFFSET       18
#define SJPEG_BLOCK_WIDTH_OFFSET        20
#define SJPEG_FRAME_INFO_ARRAY_OFFSET   22

typedef enum {
    SJPEG_IO_SOURCE_C_ARRAY,
    SJPEG_IO_SOURCE_DISK,
}  io_source_type;

typedef struct {
    io_source_type type;
    FILE *file;
    uint8_t* img_cache_buff;
    int img_cache_x_res;
    int img_cache_y_res;
    union {
        uint8_t *raw_sjpg_data;
        uint32_t file_seek_offset;
    };
} io_source_t;


typedef struct {
    uint8_t *sjpeg_data;
    int sjpeg_x_res;
    int sjpeg_y_res;
    int sjpeg_total_frames;
    int sjpeg_single_frame_height;
    int sjpeg_cache_frame_index;
    uint8_t **frame_base_array; //to save base address of each split frames upto sjpeg_total_frames.
    int *frame_base_offset; //to save base offset for fseek
    uint8_t *frame_cache;
    JDEC *tjpeg_jd;
    io_source_t io;
} SJPEG;









#ifdef __cplusplus
}
#endif

#endif /* _LV_JPEG_WRAPPER */
