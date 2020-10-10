# LV JPG / SJPG DECODER 

# Overview:
  - lv_lib_split_jpg supports both normal jpg and the custom sjpg formats.
  - Decoding normal jpg consumes RAM with the site fo the whole uncompressed image (recommended only for devices with more RAM)
  - sjpg is a custom format based on "normal" JPG and specially made for lvgl.
  - sjpg is 'split-jpeg' which is a bundle of small jpeg fragments with an sjpg header.
  - sjpg size will be almost comparable to the jpg file or might be a slightly larger.
  - lv_sjpg can open and handle multiple files at same time.
  - File read from disk (fread) and c-array is implemented.
  - SJPEG frame fragment cache enables fast fetching of lines if availble in cache.
  - By default the sjpg image cache will be xres * 2 * 16 bytes (can be modified)
  - Currently only 16 bit image format is supported (to do)
# Sample code:
```c 
    
   /*-------
   
    USE lvgl/lv_fs_if driver for using lv_fs in simulator
   
   Modify lv_conf.h as below for testing in PC simulator
    #define LV_USE_FS_IF	1
    #if LV_USE_FS_IF
    #  define LV_FS_IF_FATFS    '\0'
    #  define LV_FS_IF_PC     'S'
    #endif  
  
    For fast simulatneous multi jpg/sjpg rendering in expense of additional ram, 
    you can modify LV_IMG_CACHE_DEF_SIZE to 2 or above if testing on simulator or
    using it in devices like raspberry pi etc.
  
  -------*/

#include <lvgl/lvgl.h>
#include "../lv_sjpg.h"

LV_IMG_DECLARE( small_image_sjpg );
LV_IMG_DECLARE( wallpaper_jpg );

void demo_jpg_sjpg( void )
{
  lv_fs_if_init();
  lv_obj_t * img1;
  lv_obj_t * img2;

  lv_split_jpeg_init();
  img1 = lv_img_create( lv_scr_act(), NULL );
  img2 = lv_img_create( lv_scr_act(), NULL );

  //jpg from c array
  lv_img_set_src( img1,  &wallpaper_jpg );      

  //sjpg from file (lv_fs)
  lv_img_set_src( img2,  "S.\\lv_lib_split_jpg\\example\\images\\small_image.sjpg" ); 
}


```
# Converting .jpg to jpg c array
  - Use lvgl online tool https://lvgl.io/tools/imageconverter 
  - Color format = RAW, output format = C Array
  
# Converting .jpg to .sjpg  (python3 and PIL library required)
- Drag and drop a jpeg image on top of the jpg_to_sjpg.py 
- or
- Run python script on shell with jpeg filename as argument. It should generate filename.c and filename.sjpg files.
```sh
python jpg_to_sjpg.py wallpaper.jpg
```
Expected result:
```sh

Conversion started...

Input:
        walpaper.jpg
        RES = 640 x 480

Output:
        Time taken = 1.66 sec
        bin size = 77.1 KB
        walpaper.sjpg           (bin file)
        walpaper.c              (c array)

All good!

```

- User can use the c array or the .sjpg file.
