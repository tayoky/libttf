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
}```

and then compile with
```sh
gcc test.c -lttf
```
