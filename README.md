USAGE:

Usage is almost similar to that of lv_lib_png
https://github.com/lvgl/lv_lib_png


Sample code:

lv_obj_t * img1;
lv_obj_t * img2;
void lv_demo_jpeg(void)
{
    lv_jpeg_init();
    img = lv_img_create(lv_scr_act(), NULL);
    img2 = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img1,  &wallpaper);            //image 1 from c array
    lv_img_set_src(img2,  ".\\small_image.sjpg")  //image 2 from file system (fopen)
}


