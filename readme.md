# libttf
the objective of libttf is to be a simple and portable library to render `.ttf` font  
orignaly written for [the stanix operating system](https://github.com/tayoky/stanix) libttf is hopping to one day become a viable concurrent to libfreetype

# build and install
start by launching the configure script
```sh
./configure
```
> [!NOTE]
> you can use the options `--prefix` and `--host`

then run make
```sh
make
```

and finally install
```sh
make install
```
> [!NOTE]
> this might require root permission if installing globally

# api
you can get started with
```c
#include <stdio.h>
#include <ttf.h>

int main(){
    //open a ttf file
    ttf_file *font = ttf_open("DejaVuSans.ttf");
    if(!font){
        printf("libttf : %s\n",ttf_error());
        return 1;
    }

    ttf_close(font);
    return 0;
}
```

and then compile with
```sh
gcc test.c -lttf
```

## functions
- `ttf_file *ttf_open(const char *path)`
- `void ttf_close(ttf_file *font)`
- `const char *ttf_error()`
- `uint32_t ttf_char2glyf(ttf_file *font,wchar_t c)`
- `ttf_glyph *ttf_getglyph(ttf_file *font,wchar_t c)`

# support
currently libttf support only font of type truetype outline with a unicode endoding of format 12
