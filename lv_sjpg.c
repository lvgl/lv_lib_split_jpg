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




#include "../../lv_examples.h"
#include "tjpgd.h"
#include "lv_sjpg.h"
#include <stdio.h>
#include <stdlib.h>


uint8_t* workb;
const size_t sz_work = TJPGD_WORKBUFF_SIZE;	/* Size of working buffer for TJpgDec module */


static lv_res_t decoder_info(lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header);
static lv_res_t decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc);
static lv_res_t decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc, lv_coord_t x,lv_coord_t y, lv_coord_t len, uint8_t * buf);
static void decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc);


void lv_split_jpeg_init(void)
{
    workb = malloc(sz_work);
    /*Create a new decoder and register functions */
    lv_img_decoder_t* dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
    lv_img_decoder_set_read_line_cb(dec, decoder_read_line);
}



/**
 * Get info about a SJPG image
 * @param decoder pointer to the decoder where this function belongs
 * @param src can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */

static lv_res_t decoder_info(lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header)
{
  /*Check whether the type `src` is known by the decoder*/
  // if(is_png(src) == false) return LV_RES_INV;
  /* Read the SJPG header and find `width` and `height` */

  lv_img_src_t src_type = lv_img_src_get_type(src);          /*Get the source type*/

  if(src_type == LV_IMG_SRC_VARIABLE) {
      uint8_t *raw_sjpeg_data = (uint8_t *)((lv_img_dsc_t * )src)->data;

      if(!strncmp((char *)raw_sjpeg_data, "_SJPG__", strlen("_SJPG__") )) {

        raw_sjpeg_data += 14; //seek to res info ... refer sjpeg format
        header->always_zero = 0;
        header->cf = LV_IMG_CF_RAW;

        header->w = *raw_sjpeg_data++;
        header->w |= *raw_sjpeg_data++ << 8;

        header->h = *raw_sjpeg_data++;
        header->h |= *raw_sjpeg_data++ << 8;

        return LV_RES_OK;
    }
  }

  else if(src_type == LV_IMG_SRC_FILE) {
        const char * fn = src;
        if(!strcmp(&fn[strlen(fn) - 4], "sjpg")) {

            uint8_t buff[22];
            memset(buff, 0, sizeof(buff));

            #if 1//LV_USE_FS_IF

            lv_fs_file_t file;
            lv_fs_res_t res = lv_fs_open(&file , fn, LV_FS_MODE_RD);
            if(res != LV_FS_RES_OK) return 78;

            uint32_t rn;
            res = lv_fs_read(&file, buff, 8, &rn);
            if(res != LV_FS_RES_OK || rn != 8) {
                lv_fs_close(&file);
                return LV_RES_INV;
            }

            if(strcmp((char *)buff, "_SJPG__") == 0 ) {
                lv_fs_seek(&file, 14);
                //fseek(file, 14, SEEK_SET); //seek to res info ... refer sjpeg format
//                int rn = fread(buff, 1, 4, file);

                res = lv_fs_read(&file, buff, 4, &rn);
                if(res != LV_FS_RES_OK || rn != 4 ) {
                    lv_fs_close(&file);
                    return LV_RES_INV;
                }
                header->always_zero = 0;
                uint8_t *raw_sjpeg_data = buff;
                header->w = *raw_sjpeg_data++;
                header->w |= *raw_sjpeg_data++ << 8;
                header->h = *raw_sjpeg_data++;
                header->h |= *raw_sjpeg_data++ << 8;
                lv_fs_close(&file);
                return LV_RES_OK;
            #endif
        }
    }
  }
  return LV_RES_INV;
}



static int img_data_cb(JDEC* jd, void* data, JRECT* rect)
{
    io_source_t *io = jd->device;
    uint8_t *cache = io->img_cache_buff;
    int xres = io->img_cache_x_res;
    uint8_t *buf = data;
    int x1 = rect->left, x2 = rect->right, y1 = rect->top, y2 = rect->bottom;

    for( int y = y1; y <= y2; y++ ) {
        memcpy( ( cache + y * xres * 2 + x1 * 2), buf, ( x2 -x1 + 1 ) * 2 );
        buf += ( x2 - x1 + 1 ) * 2;
    }


    return 1;
}


static unsigned int input_func (JDEC* jd, uint8_t* buff, unsigned int ndata)
{
    io_source_t *io = jd->device;

    if(!io) return 0;

    if(io->type == SJPEG_IO_SOURCE_C_ARRAY) {
        if(buff) {
            memcpy(buff, io->raw_sjpg_data, ndata);
        }
        io->raw_sjpg_data += ndata;
        return ndata;
    }

    else if(io->type == SJPEG_IO_SOURCE_DISK) {

        //FILE *file = io->file;
        lv_fs_file_t* lv_file_p = &(io->lv_file);

        if( buff ) {
           // int rn =  fread( buff, 1, ndata, file );
           //return rn;
            uint32_t rn = 0;
            lv_fs_read(lv_file_p, buff, ndata, &rn);
            return rn;
        } else {
            uint32_t pos;
            lv_fs_tell(lv_file_p, &pos);
            lv_fs_seek(lv_file_p, ndata + pos);
            return ndata;
        }
    }

    return 0;
}


/**
 * Open SJPG image and return the decided image
 * @param decoder pointer to the decoder where this function belongs
 * @param dsc pointer to a descriptor which describes this decoding session
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
 //SJPEG sjp;
static lv_res_t decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    if(dsc->src_type == LV_IMG_SRC_VARIABLE) {
        uint8_t *data;
        SJPEG* sjpeg = ( SJPEG* ) dsc->user_data;
        if(sjpeg == NULL) {
           sjpeg =  malloc(sizeof(SJPEG));
            dsc->user_data = sjpeg;
             sjpeg->sjpeg_data = (uint8_t *)( (lv_img_dsc_t* )(dsc->src) )->data;
        }
        data = sjpeg->sjpeg_data;
        data += 14;

        sjpeg->sjpeg_x_res = *data++;
        sjpeg->sjpeg_x_res |= *data++ << 8;

        sjpeg->sjpeg_y_res = *data++;
        sjpeg->sjpeg_y_res |= *data++ << 8;

        sjpeg->sjpeg_total_frames = *data++;
        sjpeg->sjpeg_total_frames |= *data++ << 8;

        sjpeg->sjpeg_single_frame_height = *data++;
        sjpeg->sjpeg_single_frame_height |= *data++ << 8;

        sjpeg->frame_base_array = malloc( sizeof(uint8_t *) * sjpeg->sjpeg_total_frames );
        sjpeg->frame_base_offset = NULL;

        uint8_t *img_frame_base = data +  sjpeg->sjpeg_total_frames *2;
        sjpeg->frame_base_array[0] = img_frame_base;

        for( int i = 1; i <  sjpeg->sjpeg_total_frames; i++ ) {
             int offset = *data++;
             offset |= *data++ <<8;
             sjpeg->frame_base_array[i] = sjpeg->frame_base_array[i-1] + offset;
        }
        sjpeg->sjpeg_cache_frame_index = -1;
        sjpeg->frame_cache = (void *)malloc( sjpeg->sjpeg_x_res * sjpeg->sjpeg_single_frame_height * 2 );
        sjpeg->io.img_cache_buff = sjpeg->frame_cache;
        sjpeg->io.img_cache_x_res = sjpeg->sjpeg_x_res;
        sjpeg->tjpeg_jd =   malloc( sizeof( JDEC ) );
        sjpeg->io.type = SJPEG_IO_SOURCE_C_ARRAY;
        sjpeg->io.lv_file.file_d = NULL;
        dsc->img_data = NULL;
        return LV_RES_OK;
    }

    else if(dsc->src_type == LV_IMG_SRC_FILE) {
        /* If all fine, then the file will be kept open */
        const char * fn = dsc->src;
        uint8_t *data;

        if(!strcmp(&fn[strlen(fn) - 4], "sjpg")) {

            uint8_t buff[22];
            memset(buff, 0, sizeof(buff));


            lv_fs_file_t lv_file;
            lv_fs_res_t res = lv_fs_open(&lv_file , fn, LV_FS_MODE_RD);
            if(res != LV_FS_RES_OK) {
                return 78;
            }


            uint32_t rn;
            res = lv_fs_read(&lv_file, buff, 22, &rn);
            if(res != LV_FS_RES_OK || rn != 22 ) {
                lv_fs_close(&lv_file);
                return LV_RES_INV;
            }

            if(strcmp((char *)buff, "_SJPG__") == 0 ) {

                SJPEG* sjpeg = ( SJPEG* ) dsc->user_data;
                if(sjpeg == NULL) {
                    sjpeg =   malloc(sizeof(SJPEG));
                    dsc->user_data = sjpeg;
                    sjpeg->sjpeg_data = (uint8_t *)( (lv_img_dsc_t* )(dsc->src) )->data;
                }
                data = buff;
                data += 14;

                sjpeg->sjpeg_x_res = *data++;
                sjpeg->sjpeg_x_res |= *data++ << 8;

                sjpeg->sjpeg_y_res = *data++;
                sjpeg->sjpeg_y_res |= *data++ << 8;

                sjpeg->sjpeg_total_frames = *data++;
                sjpeg->sjpeg_total_frames |= *data++ << 8;

                sjpeg->sjpeg_single_frame_height = *data++;
                sjpeg->sjpeg_single_frame_height |= *data++ << 8;

                sjpeg->frame_base_array = NULL;//malloc( sizeof(uint8_t *) * sjpeg->sjpeg_total_frames );
                sjpeg->frame_base_offset = malloc( sizeof(int) * sjpeg->sjpeg_total_frames );

                int img_frame_start_offset = (SJPEG_FRAME_INFO_ARRAY_OFFSET + sjpeg->sjpeg_total_frames *2);
                sjpeg->frame_base_offset[0] = img_frame_start_offset; //pointer used to save integer for now...

                for( int i = 1; i <  sjpeg->sjpeg_total_frames; i++ ) {
                    uint32_t rn;
                    res = lv_fs_read(&lv_file, buff, 2, &rn);
                        if(res != LV_FS_RES_OK || rn != 2 ) {
                        lv_fs_close(&lv_file);
                        return LV_RES_INV;
                    }

                    data = buff;
                    int offset = *data++;
                    offset |= *data++ <<8;
                    sjpeg->frame_base_offset[i] = sjpeg->frame_base_offset[i-1] + offset;
                }

                sjpeg->sjpeg_cache_frame_index = -1; //INVALID AT BEGINNING for a forced compare mismatch at first time.
                sjpeg->frame_cache = (void *)malloc( sjpeg->sjpeg_x_res * sjpeg->sjpeg_single_frame_height * 2 );
                sjpeg->io.img_cache_buff = sjpeg->frame_cache;
                sjpeg->io.img_cache_x_res = sjpeg->sjpeg_x_res;
                sjpeg->tjpeg_jd =    malloc( sizeof( JDEC ) );
                sjpeg->io.type = SJPEG_IO_SOURCE_DISK;
                memcpy(&(sjpeg->io.lv_file), &lv_file, sizeof(lv_file));
                dsc->img_data = NULL;
                return LV_RES_OK;
            }
        }
    }
    return LV_RES_INV;
}




/**
 * Decode `len` pixels starting from the given `x`, `y` coordinates and store them in `buf`.
 * Required only if the "open" function can't open the whole decoded pixel array. (dsc->img_data == NULL)
 * @param decoder pointer to the decoder the function associated with
 * @param dsc pointer to decoder descriptor
 * @param x start x coordinate
 * @param y start y coordinate
 * @param len number of pixels to decode
 * @param buf a buffer to store the decoded pixels
 * @return LV_RES_OK: ok; LV_RES_INV: failed
 */

static lv_res_t decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc, lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t * buf)
{

    if(dsc->src_type == LV_IMG_SRC_VARIABLE) {
        SJPEG* sjpeg = ( SJPEG* ) dsc->user_data;


        JRESULT rc;

        int sjpeg_req_frame_index = y / sjpeg->sjpeg_single_frame_height;

        /*If line not from cache, refresh cache */
        if(sjpeg_req_frame_index != sjpeg->sjpeg_cache_frame_index) {
            sjpeg->io.raw_sjpg_data = sjpeg->frame_base_array[ sjpeg_req_frame_index ];
            rc = jd_prepare( sjpeg->tjpeg_jd, input_func, workb, (unsigned int)sz_work, &(sjpeg->io));
            if(rc != JDR_OK ) return LV_RES_INV;
            rc = jd_decomp ( sjpeg->tjpeg_jd, img_data_cb, 0);
            if(rc != JDR_OK ) return LV_RES_INV;
            sjpeg->sjpeg_cache_frame_index = sjpeg_req_frame_index;
        }

        memcpy( buf, (uint8_t *)sjpeg->frame_cache + (x * 2) + ( y % sjpeg->sjpeg_single_frame_height ) * sjpeg->sjpeg_x_res*2 , len*2 );
        return LV_RES_OK;


        return LV_RES_OK;
    }


    else if(dsc->src_type == LV_IMG_SRC_FILE) {
        SJPEG* sjpeg = ( SJPEG* ) dsc->user_data;
        JRESULT rc;
        int sjpeg_req_frame_index = y / sjpeg->sjpeg_single_frame_height;

//      FILE *file = (FILE *)sjpeg->io.file;
//      if(!file) goto end;

        lv_fs_file_t* lv_file_p = &(sjpeg->io.lv_file);
        if(!lv_file_p) goto end;

   /*     int rn;
        res = lv_fs_read(lv_file_p, buff, 22, &rn);
        if(res != LV_FS_RES_OK || rn != 8 ) {
            lv_fs_close(lv_file_p);
            return LV_RES_INV;
        }
*/


        /*If line not from cache, refresh cache */
        if(sjpeg_req_frame_index != sjpeg->sjpeg_cache_frame_index) {
            sjpeg->io.file_seek_offset = (int)(sjpeg->frame_base_offset [ sjpeg_req_frame_index ]);
            //fseek(sjpeg->io.file,  sjpeg->io.file_seek_offset , SEEK_SET);
            lv_fs_seek( &(sjpeg->io.lv_file), sjpeg->io.file_seek_offset );
            rc = jd_prepare( sjpeg->tjpeg_jd, input_func, workb, (unsigned int)sz_work, &(sjpeg->io));
            if(rc != JDR_OK ) return LV_RES_INV;
            rc = jd_decomp ( sjpeg->tjpeg_jd, img_data_cb, 0);
            if(rc != JDR_OK ) return LV_RES_INV;
            sjpeg->sjpeg_cache_frame_index = sjpeg_req_frame_index;
        }

        memcpy( buf, (uint8_t *)sjpeg->frame_cache + (x * 2) + ( y % sjpeg->sjpeg_single_frame_height ) * sjpeg->sjpeg_x_res*2 , len*2 );
        return LV_RES_OK;
    }


end:

return LV_RES_INV;



}


/**
 * Free the allocated resources
 * @param decoder pointer to the decoder where this function belongs
 * @param dsc pointer to a descriptor which describes this decoding session
 */
static void decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
  /*Free all allocated data*/
    SJPEG* sjpeg = ( SJPEG* ) dsc->user_data;
    if(!sjpeg) return;

    switch(dsc->src_type) {

        case LV_IMG_SRC_FILE:
            if(sjpeg->io.lv_file.file_d) {
                lv_fs_close(&(sjpeg->io.lv_file));
            }
			//no break

        case LV_IMG_SRC_VARIABLE:// OR LV_IMG_SRC_FILE
            if(sjpeg->frame_cache) free(sjpeg->frame_cache);
            if(sjpeg->frame_base_array) free(sjpeg->frame_base_array);
            if(sjpeg->frame_base_offset) free(sjpeg->frame_base_offset);
            if(sjpeg->tjpeg_jd) free(sjpeg->tjpeg_jd);
            break;

        default:
            ;
    }
}
