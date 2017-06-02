
#define PY_SSIZE_T_CLEAN

#include <dlfcn.h>
#include <string.h>
#include <Python.h>

#include <errno.h>

#include <stdio.h>
#include <dirent.h>			//opendir
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "CoraSerial.h"

static PyObject *LTEError;
static module_t *ctx;


enum LED_STATUS {
	LED_UNDEF = -1,
	LED_OFF = 0,
	LED_ON,
	LED_MAX
};


void KMP_GetNext(const char *des, int *next)
{
    int j = 0, k = 0, size = strlen(des);
    next[j] = k;

    for ( j = 1, k = 0; j < size; ++j) {
        while ((k > 0) && (des[j] != des[k])) {
            k = next[k - 1];
        }
        if (des[j] == des[k]) {
            k++;
        }
        next[j] = k;
    }
}

int KMP_Match(const char *src, const char *des, int len)
{
    int next[MAX_RECV_BUF_SIZE] = {'\0'};
    int i = 0, j = 0, size = strlen(des);

    KMP_GetNext(des, next);

    for (i = 0, j = 0; i < len; i++) {
        while ((j > 0) && (src[i] != des[j])) {
            j = next[j-1];
        }
        if (src[i] == des[j]) {
            j++;
        }
        if (j == size) {
            return i - size + 1;
        }
    }

    return -1;
}

static PyObject* new_lte(PyObject* self, PyObject* args)
{
    char *device;
	char parity = 'N';
	int baud = 3000000, data_bit = 8, stop_bit = 1;

	if(!PyArg_ParseTuple(args, "s|icii", &device, &baud, &parity, &data_bit, &stop_bit)) {
		PyErr_SetString(LTEError, "Invalid input parameter.");
        return NULL;
    }

    ctx = module_new(device, baud, parity, data_bit, stop_bit);
    if (ctx == NULL) {
		PyErr_SetString(LTEError, "Illegal function [module_new]");
		return NULL;
	}

	if (-1 == module_connect(ctx)) {
		module_free(ctx);
		PyErr_SetString(LTEError, "Illegal function [module_connect]");
		return NULL;
	}

    Py_RETURN_TRUE;
}


static PyObject* init(PyObject* self, PyObject* args)
{
    const uint8_t cmds[7][16] = {"AT\r\n", "ATE0\r\n", "AT^CURC=0\r\n", "AT^STSF=0\r\n", "ATS0=0\r\n", "AT+CGREG=2\r\n", "AT+CMEE=2\r\n"};
    int ret = 0;

    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ret = module_send(ctx, (uint8_t *)cmds[i], sizeof(cmds[i]));
        if (ret != sizeof(cmds[i])) {
            PyErr_SetString(LTEError, "Communication faild.");
            return NULL;        
        }
        // usleep(300*1000);
        uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};
        int len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);

        if ( len <= 0 || (-1 == KMP_Match((char *)buffer, "OK", len))) {
            printf("ERROR %s", cmds[i]);
            Py_RETURN_FALSE;
        }
    }

    Py_RETURN_TRUE;
}


static PyObject* cmd_lte(PyObject* self, PyObject* args)
{
    char *cmd;

	if(!PyArg_ParseTuple(args, "s", &cmd)) {
		PyErr_SetString(LTEError, "Invalid input parameter.");
        return NULL;
    }

    int ret = module_send(ctx, (uint8_t *)cmd, strlen(cmd));
    if (ret != strlen(cmd)) {
		PyErr_SetString(LTEError, "Communication faild.");
        return NULL;        
    }

    uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};
    int len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);
    if ( len <= 0 || (-1 == KMP_Match((char *)buffer, "OK", len))) {
        return (PyObject*)Py_BuildValue("s", "ERROR");
    }

    return (PyObject*)Py_BuildValue("s", buffer);
}


static PyObject* led(PyObject* self, PyObject* args)
{
    char cmd[32] = {'\0'};
    int  status = 0;

	if(!PyArg_ParseTuple(args, "i", &status)) {
		PyErr_SetString(LTEError, "Invalid input parameter.");
        return NULL;
    }

	if (status <= LED_UNDEF || status >= LED_MAX) {
		PyErr_SetString(LTEError, "Invalid status.");
        return NULL;
	}

    sprintf(cmd, "AT^LEDCTRL=%d\r\n", status);

    if (module_send(ctx, (uint8_t *)cmd, strlen(cmd)) != strlen(cmd)) {
        Py_RETURN_FALSE;
    }
    
    uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};
    int len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);
    if ( len <= 0 || (-1 == KMP_Match((char *)buffer, "OK", len))) {
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}


static PyObject* status(PyObject* self, PyObject* args)
{
    char *cmd = "AT^NDISSTATQRY?\r\n";

    if (module_send(ctx, (uint8_t *)cmd, strlen(cmd)) != strlen(cmd)) {
        Py_RETURN_FALSE;
    }

    uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};

    for (int i = 0; i < 3; i++) {
        memset(buffer, '\0', MAX_RECV_BUF_SIZE);
        int len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);
        int pos = KMP_Match((char *)buffer, "^NDISSTATQRY: ", len);

        if ((len > 0) && (-1 != pos)) {
            switch (buffer[pos + strlen("^NDISSTATQRY: ")]) 
            {
                case '0':
                    return (PyObject*)Py_BuildValue("s", "offline");
                    break;
                case '1':
                    return (PyObject*)Py_BuildValue("s", "online");
                    break;
                default:
                    break;
            }
        }
        usleep(300*1000);
    }

    return (PyObject*)Py_BuildValue("s", "ERROR");    
}

static PyObject* online(PyObject* self, PyObject* args)
{
    
    char *cmd = "AT^NDISDUP=1,1\r\n";

    int ret = module_send(ctx, (uint8_t *)cmd, strlen(cmd));
    if (ret != strlen(cmd)) {
        Py_RETURN_FALSE;
    }
    
    uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};
    int len = 0;

    for (int i = 0; i < 3; i++) {
        memset(buffer, '\0', MAX_RECV_BUF_SIZE);
        len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);
        if ((len > 0) && (-1 != KMP_Match((char *)buffer, "^NDISSTAT: 1", len))) {
            system("udhcpc -n -i usb0");
            Py_RETURN_TRUE;
        }
        usleep(300*1000);
    }

    Py_RETURN_FALSE;
}

static PyObject* offline(PyObject* self, PyObject* args)
{
    char *cmd = "AT^NDISDUP=1,0\r\n";
    int ret = module_send(ctx, (uint8_t *)cmd, strlen(cmd));
    if (ret != strlen(cmd)) {
        Py_RETURN_FALSE;
    }

    uint8_t buffer[MAX_RECV_BUF_SIZE] = {'\0'};
    int len = 0;

    for (int i = 0; i < 3; i++) {
        memset(buffer, '\0', MAX_RECV_BUF_SIZE);
        len = module_recv(ctx, buffer, MAX_RECV_BUF_SIZE);
        if ((len > 0) && (-1 != KMP_Match((char *)buffer, "^NDISSTAT: 0", len))) {
            Py_RETURN_TRUE;
        }
        usleep(300*1000);
    }

    Py_RETURN_FALSE;
}

static PyObject* restart(PyObject* self, PyObject* args)
{
    char *cmd = "AT^RESET\r\n";

    if (module_send(ctx, (uint8_t *)cmd, strlen(cmd)) != strlen(cmd)) {
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}


static PyObject* call(PyObject* self, PyObject* args)
{
    char *num;
    char cmd[32] = {'\0'};

	if(!PyArg_ParseTuple(args, "s", &num)) {
		PyErr_SetString(LTEError, "Invalid input parameter.");
        return NULL;
    }

    sprintf(cmd, "ATD%s;\r\n", num);
    if (module_send(ctx, (uint8_t *)cmd, strlen(cmd)) != strlen(cmd)) {
        Py_RETURN_FALSE;
    }
    
    Py_RETURN_TRUE;
}

static PyObject* hangup(PyObject* self, PyObject* args)
{
    char *cmd = "AT+CHUP\r\n";

    if (module_send(ctx, (uint8_t *)cmd, strlen(cmd)) != strlen(cmd)) {
        Py_RETURN_FALSE;
    }
    
    Py_RETURN_TRUE;
}

static PyObject* del_lte(PyObject* self, PyObject* args)
{
    module_close(ctx);
  	module_free(ctx);
    Py_RETURN_NONE;
}

void free_lte()
{
    module_close(ctx);  
  	module_free(ctx);
}

static PyMethodDef lte_methods[] =
{
    {"led",     led,     METH_VARARGS, "LTE module LED control."},
    {"init",    init,    METH_NOARGS,  "Init LTE module."},
    {"call",    call,    METH_VARARGS, "Call number."},
    {"new_lte", new_lte, METH_VARARGS, "!"},
    {"cmd_lte", cmd_lte, METH_VARARGS, "!"},
    {"del_lte", del_lte, METH_NOARGS,  "!"},
    {"status",  status,  METH_NOARGS,  "LTE connection status"},
    {"online",  online,  METH_NOARGS,  "LTE connection"},
    {"offline", offline, METH_NOARGS,  "LTE disconnection"},
    {"restart", restart, METH_NOARGS,  "LTE module reset"},
    {"hangup",  hangup,  METH_NOARGS,  "HangUp call."},
    {NULL, NULL}
};


PyDoc_STRVAR(lte_module_documentation,
"This interface specifications that is supported by Huawei Mobile Broadband product ME909s module.\n"
"AT commands of international standards, such as 3GPP and ITU-T.\n"
"\n"
"\n");

static struct PyModuleDef huawei_me909s_lte = {
        PyModuleDef_HEAD_INIT,
        "Toradex Apalis/Ixora iMX6",
        lte_module_documentation,
        -1,
        lte_methods,
        NULL,
        NULL,
        NULL,
        free_lte
};

/*
	Python init function.
*/
PyMODINIT_FUNC PyInit_me909s(void)
{
	PyObject *m;
	
    m = PyModule_Create(&huawei_me909s_lte);
    if (m == NULL) {
        return NULL;
	}

	LTEError = PyErr_NewException("lte.error", NULL, NULL);
    if (LTEError != NULL) {
        Py_INCREF(LTEError);
        PyModule_AddObject(m, "error", LTEError);
    }

	PyModule_AddIntMacro(m, LED_ON);
	PyModule_AddIntMacro(m, LED_OFF);

    PyModule_AddStringConstant(m, "__version__", "1.0");
	PyModule_AddStringConstant(m, "__author__", "Heyn"); 

    return m;
}
