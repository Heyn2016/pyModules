
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

#define FILE_WRITE_NMEMB				1
static PyObject *LEDError;


#define HIGH					1
#define LOW						0

enum LED_NUMBER {
	IXORA_LED_UNDEF = -1,
	IXORA_LED4 = 4,
	IXORA_LED5,
	IXORA_LED_MAX
};

enum COLOR_TYPE {
	COLOR_UNDEF = -1,
	RED = 0,
	GREEN,
	COLOR_MAX
};


static PyObject* ioctl(PyObject* self, PyObject* args)
{
	FILE * leddev = NULL;
	
	int num, color, status = 0;
	char colors[64] = {'\0'};
	char path[128] = {'\0'};
	int ret = -1;

	if(!PyArg_ParseTuple(args, "ii|i", &num, &color, &status))
	{
		PyErr_SetString(LEDError, "Invalid input parameter.");
        return NULL;
    }

	if (num <= IXORA_LED_UNDEF || num >= IXORA_LED_MAX) {
		PyErr_SetString(LEDError, "Invalid Port Number.");
        return NULL;
	}

	if (color <= COLOR_UNDEF || color >= COLOR_MAX) {
		PyErr_SetString(LEDError, "Invalid color.");
        return NULL;
	}

	if (status < LOW || status > HIGH) {
		PyErr_SetString(LEDError, "Invalid status.");
        return NULL;
	}	

	switch(color) {
		case RED:
			memcpy(colors, "RED", strlen("RED"));
			break;
		case GREEN:
			memcpy(colors, "GREEN", strlen("GREEN"));
			break;
		default:
			PyErr_SetString(LEDError, "Invalid color.");
        	return NULL;
			break;
	}
	
	sprintf(path, "/sys/class/leds/LED_%d_%s/brightness", num, colors);
	
	leddev = fopen(path, "w");
	
	if (NULL == leddev) {
		PyErr_Format(LEDError, "Open file faild [%s]" , strerror(errno));
		return NULL;
	}
	
	switch(status) {
		case HIGH:
			ret = fwrite("1", sizeof("1"), FILE_WRITE_NMEMB, leddev);
			break;
		case LOW:
			ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, leddev);
			break;
	}
	
	
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(LEDError, "Write file faild [%s]" , strerror(errno));
		fclose(leddev);
		return NULL;
	}
	fclose(leddev);
	
	Py_RETURN_TRUE;
}

void led_exit()
{
	FILE * leddev = NULL;
	
	int i = 0, j = 0;
	char path[128] = {'\0'};
	char colors[2][6] = {{"RED"}, {"GREEN"}};
	int ret = -1;
	
	for (i=0; i<2; i++) {
		for (j=0; j<2; j++) {
			sprintf(path, "/sys/class/leds/LED_%d_%s/brightness", IXORA_LED4 + i, colors[j]);
			leddev = fopen(path, "w");
			
			if (NULL == leddev) {
				PyErr_Format(LEDError, "Open file faild [%s]" , strerror(errno));
				return ;
			}
			ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, leddev);
			if ((ret < FILE_WRITE_NMEMB)) {
				PyErr_Format(LEDError, "Write file faild [%s]" , strerror(errno));
				fclose(leddev);
				return ;
			}
			fclose(leddev);
			memset(path, '\0', sizeof(path));
		}
	}
}

static PyObject* clear(PyObject* self, PyObject* args)
{
	led_exit();
	Py_RETURN_TRUE;
}


static PyMethodDef led_methods[] =
{
	{"ioctl", ioctl, METH_VARARGS, "Toradex Apalis/Ixora iMX6 Carrier Board Led Control."},
	{"clear", clear, METH_VARARGS, "Toradex Apalis/Ixora iMX6 Carrier Board Led Control Clear."},
    {NULL, NULL}
};


PyDoc_STRVAR(led_module_documentation,
"The functions in this module allow <Toradex Apalis/Ixora iMX6 Carrier Board> using\n"
"\n"
"\n");

static struct PyModuleDef imx6_ixora_led = {
        PyModuleDef_HEAD_INIT,
        "Toradex Apalis/Ixora iMX6",
        led_module_documentation,
        -1,
        led_methods,
        NULL,
        NULL,
        NULL,
        led_exit
};

/*
	Python init function.
*/
PyMODINIT_FUNC PyInit_imx6_ixora_led(void)
{
	PyObject *m;
	
    m = PyModule_Create(&imx6_ixora_led);
    if (m == NULL)
        return NULL;

	LEDError = PyErr_NewException("led.error", NULL, NULL);
    if (LEDError != NULL) {
        Py_INCREF(LEDError);
        PyModule_AddObject(m, "error", LEDError);
    }

	PyModule_AddIntMacro(m, IXORA_LED4);
	PyModule_AddIntMacro(m, IXORA_LED5);
	PyModule_AddIntMacro(m, RED);
	PyModule_AddIntMacro(m, GREEN);
	
	PyModule_AddIntMacro(m, HIGH);
	PyModule_AddIntMacro(m, LOW);
	
    PyModule_AddStringConstant(m, "__version__", "1.0");
	PyModule_AddStringConstant(m, "__author__", "Heyn"); 

    return m;
}
