#include <lvgl/lvgl.h>
#include "../lv_sjpg.h"

LV_IMG_DECLARE( small_image_sjpg );
LV_IMG_DECLARE( wallpaper_jpg );

void demo_jpg_sjpg( void )
{
    lv_fs_if_pc_init();
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
