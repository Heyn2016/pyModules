## Python3.5.2 Modules (Cross-compiling)
#### openssl-1.0.2j (URL:https://www.openssl.org/source/)
#### readline-6.2 (URL:http://ftp.gnu.org/gnu/readline/)
#### termcap-1.3.1 (URL:http://ftp.gnu.org/gnu/termcap/)

## Support Toradex iMX6Q
#### [1] Linux 3.14.52  
#### [2] GCC Linaro GCC 5.2-2015.11-2  

## Installing and Using

#### [1] OpenSSL

$ tar zxvf openssl-1.0.2j.tar.gz  
$ ./Configure linux-armv4 -march=armv7-a -mthumb -mfpu=neon  -mfloat-abi=hard shared --cross-compile-prefix=/home/gcc-linaro/bin/arm-linux-gnueabihf-  
$ make  
$ make install  
Install default path: /usr/local/ssl  

#### [2] readline

$ CC=arm-linux-gnueabihf-gcc ./configure --host=arm-linux-gnueabihf  --prefix=/usr/local/readline  
$ vim Makefile  

CC = `arm-linux-gnueabihf-`gcc  
AR = `arm-linux-gnueabihf-`ar  
RANLIB = `arm-linux-gnueabihf-`ranlib  

$ make  
$ make install  

#### [3] termcap  
$ ./comfigure  
$ vim Makefile

CC = `arm-linux-gnueabihf-`gcc  
AR = `arm-linux-gnueabihf-`ar  
RANLIB = `arm-linux-gnueabihf-`ranlib  
CFLAGS = -g `-fPIC`  
prefix = `/usr/local/readline`  

$ make  
$ make install  


### [4] Python3.5.2/Modules/Setup

SSL=/usr/local/ssl  
_ssl _ssl.c \  
    -DUSE_SSL -I$(SSL)/include -I$(SSL)/include/openssl \  
    -L$(SSL)/lib -lssl -lcrypto  

readline readline.c -I`/usr/local/readline/include` -L`/usr/local/readline/lib` -lreadline -ltermcap  


