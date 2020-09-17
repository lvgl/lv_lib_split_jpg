# LV SJPG DECODER 
- (first commit)

# Overview:
  - SJPG is 'split-jpeg' which is a bundle of small jpeg fragments with an sjpg header.
  - SJPG size will be almost comparable to the jpg file or might be a slightly larger.
  - lv_sjpg can open and handle multiple files at same time.
  - File read from disk (fread) and c-array is implemented.
  - SJPEG frame fragment cache enables fast fetching of lines if availble in cache.
  - By default the image cache will be xres * 2 * 16 bytes (can be modified)
  - Currently only 16 bit image format is supported (to do)
# Sample code:
```c
lv_obj_t * img1;
lv_obj_t * img2;

void lv_demo_sjpeg(void)
{
    lv_jpeg_init();
    img1 = lv_img_create(lv_scr_act(), NULL);
    img2 = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img1,  &wallpaper);            //image 1 from c array
    lv_img_set_src(img2,  ".\\small_image.sjpg")  //image 2 from file system (fopen)
}
```

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
