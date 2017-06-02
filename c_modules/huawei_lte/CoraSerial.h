

#ifndef CORA_SERIAL_H
#define CORA_SERIAL_H

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif


#define MAX_RECV_BUF_SIZE    256

/* Timeouts in microsecond (0.3 s) */
#define _RESPONSE_TIMEOUT    300000
#define _BYTE_TIMEOUT        300000


// #define HAVE_DECL_TIOCM_RTS 1
#define UARTS_RTS_NONE   0
#define UARTS_RTS_UP     1
#define UARTS_RTS_DOWN   2

typedef struct _cora_serial {
    /* Device: "/dev/ttyS0", "/dev/ttyUSB0" or "/dev/tty.USA19*" on Mac OS X. */
    char *device;
    /* Bauds: 9600, 19200, 57600, 115200, etc */
    int baud;
    /* Data bit */
    uint8_t data_bit;
    /* Stop bit */
    uint8_t stop_bit;
    /* Parity: 'N', 'O', 'E' */
    char parity;
#if defined(_WIN32)
    struct win32_ser w_ser;
    DCB old_dcb;
#else
    /* Save old termios settings */
    struct termios old_tios;
#endif

#if HAVE_DECL_TIOCM_RTS
    int rts;
    int rts_delay;
    int onebyte_time;
    void (*set_rts) (module_t *ctx, int on);
#endif
    /* To handle many slaves on the same link */
    int confirmation_to_ignore;
} cora_serial_t;


typedef struct _modules module_t;


typedef struct _module_backend {
    ssize_t (*send) (module_t *ctx, const uint8_t *req, int req_length);
    ssize_t (*recv) (module_t *ctx, uint8_t *rsp, int rsp_length);
    int  (*connect) (module_t *ctx);
    void (*close)   (module_t *ctx);
    int  (*flush)   (module_t *ctx);
    int  (*select)  (module_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
    void (*free)    (module_t *ctx);
} module_backend_t;


struct _modules {
    int s;          /* file descriptor */
    int debug;
    int marks;
    struct timeval response_timeout;
    struct timeval byte_timeout;
    const module_backend_t *backend;
    void *backend_data;
};



module_t* module_new(const char *device, int baud, char parity, int data_bit, int stop_bit);
int module_connect(module_t *ctx);
int module_recv(module_t *ctx, uint8_t *rsp, int rsp_length);
int module_send(module_t *ctx, uint8_t *req, int req_length);
void module_free(module_t *ctx);
void module_close(module_t *ctx);
#endif /* CORA_SERIAL_H */