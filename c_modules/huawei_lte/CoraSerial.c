


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <assert.h>

#include "CoraSerial.h"

#if HAVE_DECL_TIOCSRS485 || HAVE_DECL_TIOCM_RTS
#include <sys/ioctl.h>
#endif

#if HAVE_DECL_TIOCSRS485
#include <linux/serial.h>
#endif


static int _uarts_connect(module_t *ctx)
{
#if defined(_WIN32)
    DCB dcb;
#else
    struct termios tios;
    speed_t speed;
    int flags;
#endif
    cora_serial_t *uart = ctx->backend_data;

    if (ctx->debug) {
        printf("Opening %s at %d bauds (%c, %d, %d)\n",
               uart->device, uart->baud, uart->parity,
               uart->data_bit, uart->stop_bit);
    }

#if defined(_WIN32)
    /* Some references here:
     * http://msdn.microsoft.com/en-us/library/aa450602.aspx
     */
    win32_ser_init(&uart->w_ser);

    /* uart->device should contain a string like "COMxx:" xx being a decimal
     * number */
    uart->w_ser.fd = CreateFileA(uart->device,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);

    /* Error checking */
    if (uart->w_ser.fd == INVALID_HANDLE_VALUE) {
        if (ctx->debug) {
            fprintf(stderr, "ERROR Can't open the device %s (LastError %d)\n",
                    uart->device, (int)GetLastError());
        }
        return -1;
    }

    /* Save params */
    uart->old_dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(uart->w_ser.fd, &uart->old_dcb)) {
        if (ctx->debug) {
            fprintf(stderr, "ERROR Error getting configuration (LastError %d)\n",
                    (int)GetLastError());
        }
        CloseHandle(uart->w_ser.fd);
        uart->w_ser.fd = INVALID_HANDLE_VALUE;
        return -1;
    }

    /* Build new configuration (starting from current settings) */
    dcb = uart->old_dcb;

    /* Speed setting */
    switch (uart->baud) {
    case 110:
        dcb.BaudRate = CBR_110;
        break;
    case 300:
        dcb.BaudRate = CBR_300;
        break;
    case 600:
        dcb.BaudRate = CBR_600;
        break;
    case 1200:
        dcb.BaudRate = CBR_1200;
        break;
    case 2400:
        dcb.BaudRate = CBR_2400;
        break;
    case 4800:
        dcb.BaudRate = CBR_4800;
        break;
    case 9600:
        dcb.BaudRate = CBR_9600;
        break;
    case 14400:
        dcb.BaudRate = CBR_14400;
        break;
    case 19200:
        dcb.BaudRate = CBR_19200;
        break;
    case 38400:
        dcb.BaudRate = CBR_38400;
        break;
    case 57600:
        dcb.BaudRate = CBR_57600;
        break;
    case 115200:
        dcb.BaudRate = CBR_115200;
        break;
    case 230400:
        /* CBR_230400 - not defined */
        dcb.BaudRate = 230400;
        break;
    case 250000:
        dcb.BaudRate = 250000;
        break;
    case 460800:
        dcb.BaudRate = 460800;
        break;
    case 500000:
        dcb.BaudRate = 500000;
        break;
    case 921600:
        dcb.BaudRate = 921600;
        break;
    case 1000000:
        dcb.BaudRate = 1000000;
        break;
    default:
        dcb.BaudRate = CBR_9600;
        if (ctx->debug) {
            fprintf(stderr, "WARNING Unknown baud rate %d for %s (B9600 used)\n",
                    uart->baud, uart->device);
        }
    }

    /* Data bits */
    switch (uart->data_bit) {
    case 5:
        dcb.ByteSize = 5;
        break;
    case 6:
        dcb.ByteSize = 6;
        break;
    case 7:
        dcb.ByteSize = 7;
        break;
    case 8:
    default:
        dcb.ByteSize = 8;
        break;
    }

    /* Stop bits */
    if (uart->stop_bit == 1)
        dcb.StopBits = ONESTOPBIT;
    else /* 2 */
        dcb.StopBits = TWOSTOPBITS;

    /* Parity */
    if (uart->parity == 'N') {
        dcb.Parity = NOPARITY;
        dcb.fParity = FALSE;
    } else if (uart->parity == 'E') {
        dcb.Parity = EVENPARITY;
        dcb.fParity = TRUE;
    } else {
        /* odd */
        dcb.Parity = ODDPARITY;
        dcb.fParity = TRUE;
    }

    /* Hardware handshaking left as default settings retrieved */

    /* No software handshaking */
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    /* Binary mode (it's the only supported on Windows anyway) */
    dcb.fBinary = TRUE;

    /* Don't want errors to be blocking */
    dcb.fAbortOnError = FALSE;

    /* Setup port */
    if (!SetCommState(uart->w_ser.fd, &dcb)) {
        if (ctx->debug) {
            fprintf(stderr, "ERROR Error setting new configuration (LastError %d)\n",
                    (int)GetLastError());
        }
        CloseHandle(uart->w_ser.fd);
        uart->w_ser.fd = INVALID_HANDLE_VALUE;
        return -1;
    }
#else
    /* The O_NOCTTY flag tells UNIX that this program doesn't want
       to be the "controlling terminal" for that port. If you
       don't specify this then any input (such as keyboard abort
       signals and so forth) will affect your process

       Timeouts are ignored in canonical input mode or when the
       NDELAY option is set on the file via open or fcntl */
    flags = O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif

    ctx->s = open(uart->device, flags);
    if (ctx->s == -1) {
        if (ctx->debug) {
            fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
                    uart->device, strerror(errno));
        }
        return -1;
    }

    /* Save */
    tcgetattr(ctx->s, &uart->old_tios);

    memset(&tios, 0, sizeof(struct termios));

    /* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    switch (uart->baud) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
#ifdef B57600
    case 57600:
        speed = B57600;
        break;
#endif
#ifdef B115200
    case 115200:
        speed = B115200;
        break;
#endif
#ifdef B230400
    case 230400:
        speed = B230400;
        break;
#endif
#ifdef B460800
    case 460800:
        speed = B460800;
        break;
#endif
#ifdef B500000
    case 500000:
        speed = B500000;
        break;
#endif
#ifdef B576000
    case 576000:
        speed = B576000;
        break;
#endif
#ifdef B921600
    case 921600:
        speed = B921600;
        break;
#endif
#ifdef B1000000
    case 1000000:
        speed = B1000000;
        break;
#endif
#ifdef B1152000
   case 1152000:
        speed = B1152000;
        break;
#endif
#ifdef B1500000
    case 1500000:
        speed = B1500000;
        break;
#endif
#ifdef B2500000
    case 2500000:
        speed = B2500000;
        break;
#endif
#ifdef B3000000
    case 3000000:
        speed = B3000000;
        break;
#endif
#ifdef B3500000
    case 3500000:
        speed = B3500000;
        break;
#endif
#ifdef B4000000
    case 4000000:
        speed = B4000000;
        break;
#endif
    default:
        speed = B9600;
        if (ctx->debug) {
            fprintf(stderr,
                    "WARNING Unknown baud rate %d for %s (B9600 used)\n", uart->baud, uart->device);
        }
    }

    /* Set the baud rate */
    if ((cfsetispeed(&tios, speed) < 0) ||
        (cfsetospeed(&tios, speed) < 0)) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }

    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (uart->data_bit) {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (uart->stop_bit == 1)
        tios.c_cflag &=~ CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (uart->parity == 'N') {
        /* None */
        tios.c_cflag &=~ PARENB;
    } else if (uart->parity == 'E') {
        /* Even */
        tios.c_cflag |= PARENB;
        tios.c_cflag &=~ PARODD;
    } else {
        /* Odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (uart->parity == 'N') {
        /* None */
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &=~ OPOST;

    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->s, TCSANOW, &tios) < 0) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }
#endif

    return 0;
}

static void _uarts_free(module_t *ctx) 
{
    free(((cora_serial_t*)ctx->backend_data)->device);
    free(ctx->backend_data);
    free(ctx);
}

static void _uarts_close(module_t *ctx)
{
    /* Restore line settings and close file descriptor in RTU mode */
    cora_serial_t *ctx_serial = ctx->backend_data;

#if defined(_WIN32)
    /* Revert settings */
    if (!SetCommState(ctx_serial->w_ser.fd, &ctx_serial->old_dcb) && ctx->debug) {
        fprintf(stderr, "ERROR Couldn't revert to configuration (LastError %d)\n",
                (int)GetLastError());
    }

    if (!CloseHandle(ctx_serial->w_ser.fd) && ctx->debug) {
        fprintf(stderr, "ERROR Error while closing handle (LastError %d)\n",
                (int)GetLastError());
    }
#else
    if (ctx->s != -1) {
        tcsetattr(ctx->s, TCSANOW, &ctx_serial->old_tios);
        close(ctx->s);
        ctx->s = -1;
    }
#endif
}


#if HAVE_DECL_TIOCM_RTS
static void _uarts_ioctl_rts(module_t *ctx, int on)
{
    int fd = ctx->s;
    int flags;

    ioctl(fd, TIOCMGET, &flags);
    if (on) {
        flags |= TIOCM_RTS;
    } else {
        flags &= ~TIOCM_RTS;
    }
    ioctl(fd, TIOCMSET, &flags);
}
#endif


static ssize_t _uarts_send(module_t *ctx, const uint8_t *req, int req_length)
{
#if defined(_WIN32)
    cora_serial_t *ctx_serial = ctx->backend_data;
    DWORD n_bytes = 0;
    return (WriteFile(ctx_serial->w_ser.fd, req, req_length, &n_bytes, NULL)) ? (ssize_t)n_bytes : -1;
#else
#if HAVE_DECL_TIOCM_RTS
    cora_serial_t *ctx_serial = ctx->backend_data;
    if (ctx_serial->rts != UARTS_RTS_NONE) {
        ssize_t size;

        if (ctx->debug) {
            fprintf(stderr, "Sending request using RTS signal\n");
        }

        ctx_serial->set_rts(ctx, ctx_serial->rts == UARTS_RTS_UP);
        usleep(ctx_serial->rts_delay);

        size = write(ctx->s, req, req_length);

        usleep(ctx_serial->onebyte_time * req_length + ctx_serial->rts_delay);
        ctx_serial->set_rts(ctx, ctx_serial->rts != UARTS_RTS_UP);

        return size;
    } else {
#endif
        return write(ctx->s, req, req_length);
#if HAVE_DECL_TIOCM_RTS
    }
#endif
#endif
}


static ssize_t _uarts_recv(module_t *ctx, uint8_t *rsp, int rsp_length)
{
#if defined(_WIN32)
    return win32_ser_read(&((cora_serial_t *)ctx->backend_data)->w_ser, rsp, rsp_length);
#else
    return read(ctx->s, rsp, rsp_length);
#endif
}


static int _uarts_select(module_t *ctx, fd_set *rset, struct timeval *tv, int length_to_read)
{
    int s_rc;
#if defined(_WIN32)
    s_rc = win32_ser_select(&((cora_serial_t *)ctx->backend_data)->w_ser, length_to_read, tv);
    if (s_rc == 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    if (s_rc < 0) {
        return -1;
    }
#else
    while ((s_rc = select(ctx->s+1, rset, NULL, NULL, tv)) == -1) {
        if (errno == EINTR) {
            if (ctx->debug) {
                fprintf(stderr, "A non blocked signal was caught\n");
            }
            /* Necessary after an error */
            FD_ZERO(rset);
            FD_SET(ctx->s, rset);
        } else {
            return -1;
        }
    }

    if (s_rc == 0) {
        /* Timeout */
        errno = ETIMEDOUT;
        return -1;
    }
#endif

    return s_rc;
}

static int _uarts_flush(module_t *ctx)
{
#if defined(_WIN32)
    cora_serial_t *ctx_rtu = ctx->backend_data;
    ctx_rtu->w_ser.n_bytes = 0;
    return (PurgeComm(ctx_rtu->w_ser.fd, PURGE_RXCLEAR) == FALSE);
#else
    return tcflush(ctx->s, TCIOFLUSH);
#endif
}

/**************************************************************************************


**************************************************************************************/
const module_backend_t _module_backend = {
    _uarts_send,
    _uarts_recv,
    _uarts_connect,
    _uarts_close,
    _uarts_flush,
    _uarts_select,
    _uarts_free
};


void _module_init_common(module_t *ctx)
{
    ctx->s = -1;
    ctx->debug = FALSE;
    ctx->marks = 315692481;
    ctx->response_timeout.tv_sec = 0;
    ctx->response_timeout.tv_usec = _RESPONSE_TIMEOUT;
    ctx->byte_timeout.tv_sec = 0;
    ctx->byte_timeout.tv_usec = _BYTE_TIMEOUT;
}

void module_free(module_t *ctx)
{
    if (ctx == NULL)
        return;

    ctx->backend->free(ctx);
}

void module_close(module_t *ctx)
{
    if (ctx == NULL)
        return;

    ctx->backend->close(ctx);
}

int module_set_byte_timeout(module_t *ctx, uint32_t to_sec, uint32_t to_usec)
{
    /* Byte timeout can be disabled when both values are zero */
    if (ctx == NULL || to_usec > 999999) {
        errno = EINVAL;
        return -1;
    }

    ctx->byte_timeout.tv_sec = to_sec;
    ctx->byte_timeout.tv_usec = to_usec;
    return 0;
}

/* Get the timeout interval between two consecutive bytes of a message */
int module_get_byte_timeout(module_t *ctx, uint32_t *to_sec, uint32_t *to_usec)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    *to_sec = ctx->byte_timeout.tv_sec;
    *to_usec = ctx->byte_timeout.tv_usec;
    return 0;
}

module_t* module_new(const char *device, int baud, char parity, int data_bit, int stop_bit)
{
    FILE *fp = NULL;
    module_t *ctx = NULL;
    cora_serial_t *ctx_serial = NULL;

    /* Check device argument */
    if (device == NULL || *device == 0) {
        fprintf(stderr, "The device string is empty\n");
        errno = EINVAL;
        return NULL;
    }

    /* Check baud argument */
    if (baud == 0) {
        fprintf(stderr, "The baud rate value must not be zero\n");
        errno = EINVAL;
        return NULL;
    }

    fp = popen("lsusb | grep 'Huawei' | awk '{print $6}' | awk -F[':'] '{print $1 $2}'", "r");
    char buffer[10] = {'\0'};
    fgets(buffer, sizeof(buffer), fp);
    pclose(fp);

    ctx = (module_t *)malloc(sizeof(module_t));
    _module_init_common(ctx);
    if (strtoul(buffer, NULL, 16) != ctx->marks) {
        return NULL;
    }

    ctx->backend = &_module_backend;
    ctx->backend_data = (cora_serial_t *)malloc(sizeof(cora_serial_t));
    ctx_serial = (cora_serial_t *)ctx->backend_data;
    ctx_serial->device = NULL;
    /* Device name and \0 */
    ctx_serial->device = (char *)malloc((strlen(device) + 1) * sizeof(char));
    strcpy(ctx_serial->device, device);
    ctx_serial->baud = baud;

    if (parity == 'N' || parity == 'E' || parity == 'O') {
        ctx_serial->parity = parity;
    } else {
        module_free(ctx);
        errno = EINVAL;
        return NULL;
    }
    ctx_serial->data_bit = data_bit;
    ctx_serial->stop_bit = stop_bit;

#if HAVE_DECL_TIOCM_RTS
    /* The RTS use has been set by default */
    ctx_serial->rts = MODBUS_RTU_RTS_NONE;

    /* Calculate estimated time in micro second to send one byte */
    ctx_serial->onebyte_time = 1000000 * (1 + data_bit + (parity == 'N' ? 0 : 1) + stop_bit) / baud;

    /* The internal function is used by default to set RTS */
    ctx_serial->set_rts = _modbus_rtu_ioctl_rts;

    /* The delay before and after transmission when toggling the RTS pin */
    ctx_serial->rts_delay = ctx_serial->onebyte_time;
#endif

    ctx_serial->confirmation_to_ignore = FALSE;

    return ctx;
}


int module_connect(module_t *ctx)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->backend->connect(ctx);
}

int module_recv(module_t *ctx, uint8_t *rsp, int rsp_length)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    fd_set rset;
    struct timeval tv;
    struct timeval *p_tv;

    /* Add a file descriptor to the set */
    FD_ZERO(&rset);
    FD_SET(ctx->s, &rset);

    tv.tv_sec = ctx->response_timeout.tv_sec;
    tv.tv_usec = ctx->response_timeout.tv_usec;
    p_tv = &tv;

    int rc = ctx->backend->select(ctx, &rset, p_tv, rsp_length);
    if (rc == -1) {
        printf("Select Error");
        return -1;
    }

    return ctx->backend->recv(ctx, rsp, rsp_length);
}

int module_send(module_t *ctx, uint8_t *req, int req_length)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->backend->send(ctx, req, req_length);
}