
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

static PyObject *PWMError;

static char pwmdevpath[64] = {'\0'};
static char periodpath[64] = {'\0'};
static char cyclespath[64] = {'\0'};

static PyObject* new_pwm(PyObject* self, PyObject* args)
{
	FILE * export_file = NULL;
	FILE * unexport_file = NULL;
	
	FILE * periods = NULL;
	FILE * duty_cycles = NULL;
	
	int devnum = 0;
	char pathname[64] = {'\0'};
	int ret = -1;
	
	if(!PyArg_ParseTuple(args, "|i", &devnum)) {
		PyErr_SetString(PWMError, "Invalid input parameter.");
        return NULL;
    }
	
	if (devnum < 1 || devnum > 2) {
		PyErr_SetString(PWMError, "Invalid PWM parameter.");
		return NULL;
	}
	
	sprintf(pathname, "/sys/class/pwm/pwmchip%d/export", devnum - 1);
	export_file = fopen(pathname, "w");
	if (NULL == export_file) {
		PyErr_Format(PWMError, "Open file faild [%s]" , strerror(errno));
		return NULL;
	}
	
	memset(pathname, '\0', sizeof(pathname));
	sprintf(pathname, "/sys/class/pwm/pwmchip%d/unexport", devnum - 1);
	unexport_file = fopen(pathname, "w");
	if (NULL == unexport_file) {
		PyErr_Format(PWMError, "Open file faild [%s]" , strerror(errno));
		return NULL;
	}
	
	
	memset(pathname, '\0', sizeof(pathname));
	sprintf(pathname, "/sys/class/pwm/pwmchip%d/pwm0", devnum - 1);
	
	if (NULL == opendir(pathname)) {
		ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, export_file);
		if (ret < FILE_WRITE_NMEMB) {
			PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
			fclose(export_file);
			fclose(unexport_file);
			return NULL;
		}
	}
	
	fclose(export_file);
	fclose(unexport_file);
	
	sprintf(periodpath, "/sys/class/pwm/pwmchip%d/pwm0/period", devnum - 1);
	periods = fopen(periodpath, "w");
	if (NULL == periods) {
		PyErr_Format(PWMError, "Open file faild [%s]" , strerror(errno));
		return NULL;
	}
	
	ret = fwrite("1000000000", sizeof("1000000000"), FILE_WRITE_NMEMB, periods);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(periods);
		return NULL;
	}
	
	fclose(periods);
	
	
	sprintf(cyclespath, "/sys/class/pwm/pwmchip%d/pwm0/duty_cycle", devnum - 1);
	duty_cycles = fopen(cyclespath, "w");
	if (NULL == duty_cycles) {
		PyErr_Format(PWMError, "Open file faild [%s]" , strerror(errno));
		return NULL;
	}
	
	ret = fwrite("500000000", sizeof("500000000"), FILE_WRITE_NMEMB, duty_cycles);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(duty_cycles);
		return NULL;
	}

	fclose(duty_cycles);
	
	sprintf(pwmdevpath, "/sys/class/pwm/pwmchip%d/pwm0/enable", devnum - 1);
	
	Py_RETURN_TRUE;
}

static PyObject* start(PyObject* self, PyObject* args)
{
	FILE * pwmdev = NULL;
	int ret = -1;
	
	pwmdev = fopen(pwmdevpath, "w");

	ret = fwrite("1", sizeof("1"), FILE_WRITE_NMEMB, pwmdev);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(pwmdev);
		return NULL;
	}
	fclose(pwmdev);
	
	Py_RETURN_TRUE;
}

static PyObject* attrs(PyObject* self, PyObject* args)
{
	FILE * period_fd = NULL;
	FILE * cycles_fd = NULL;
	int ret = -1;
	
	long period, duty_cycle;
	char values[64] = {'\0'};
	
	if(!PyArg_ParseTuple(args, "kk", &period, &duty_cycle))
	{
		PyErr_SetString(PWMError, "Invalid input parameter.");
        return NULL;
    }
	
	
	cycles_fd = fopen(cyclespath, "w");
	ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, cycles_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(cycles_fd);
		return NULL;
	}
	fclose(cycles_fd);
	
	period_fd = fopen(periodpath, "w");
	sprintf(values, "%ld", period * 1000 * 1000);
	ret = fwrite(values, strlen(values), FILE_WRITE_NMEMB, period_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(period_fd);
		return NULL;
	}
	fclose(period_fd);

	
	cycles_fd = fopen(cyclespath, "w");
	memset(values, '\0', sizeof(values));
	sprintf(values, "%ld", duty_cycle * 1000 * 1000);
	ret = fwrite(values, strlen(values), FILE_WRITE_NMEMB, cycles_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(cycles_fd);
		return NULL;
	}
	fclose(cycles_fd);
	
	Py_RETURN_TRUE;
	
}


static PyObject* rate(PyObject* self, PyObject* args)
{
	FILE * period_fd = NULL;
	FILE * cycles_fd = NULL;
	int ret = -1;
	
	long period, duty_cycle, rate = 1;
	char values[64] = {'\0'};
	
	if(!PyArg_ParseTuple(args, "k", &rate))
	{
		PyErr_SetString(PWMError, "Invalid input parameter.");
        return NULL;
    }
	
	if (rate <= 0) {
		PyErr_SetString(PWMError, "Invalid input parameter.");
        return NULL;
	}
	
	period = 1000 * 1000 * 1000 / rate;
	duty_cycle = period / 2;
	
	
	cycles_fd = fopen(cyclespath, "w");
	ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, cycles_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(cycles_fd);
		return NULL;
	}
	fclose(cycles_fd);
	
	period_fd = fopen(periodpath, "w");
	sprintf(values, "%ld", period);
	ret = fwrite(values, strlen(values), FILE_WRITE_NMEMB, period_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(period_fd);
		return NULL;
	}
	fclose(period_fd);

	
	cycles_fd = fopen(cyclespath, "w");
	memset(values, '\0', sizeof(values));
	sprintf(values, "%ld", duty_cycle);
	ret = fwrite(values, strlen(values), FILE_WRITE_NMEMB, cycles_fd);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(cycles_fd);
		return NULL;
	}
	fclose(cycles_fd);
	
	Py_RETURN_TRUE;
	
}

static PyObject* stop(PyObject* self, PyObject* args)
{
	FILE * pwmdev = NULL;
	int ret = -1;
	
	pwmdev = fopen(pwmdevpath, "w");

	ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, pwmdev);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(pwmdev);
		return NULL;
	}
	fclose(pwmdev);
	
	Py_RETURN_TRUE;
}

void del_pwm()
{
	FILE * pwmdev = NULL;
	int ret = -1;
	
	pwmdev = fopen(pwmdevpath, "w");

	ret = fwrite("0", sizeof("0"), FILE_WRITE_NMEMB, pwmdev);
	if ((ret < FILE_WRITE_NMEMB)) {
		PyErr_Format(PWMError, "Write file faild [%s]" , strerror(errno));
		fclose(pwmdev);
		return ;
	}
	fclose(pwmdev);
}

static PyMethodDef pwm_methods[] =
{
	{"new_pwm",  new_pwm,  METH_VARARGS, "Toradex Apalis/Colibri iMX6 pwm open" },
	{"start", start, METH_VARARGS, "Toradex Apalis/Colibri iMX6 pwm start"},
	{"attrs", attrs, METH_VARARGS, "Toradex Apalis/Colibri iMX6 pwm attrs"},
	{"rate",  rate,  METH_VARARGS, "Toradex Apalis/Colibri iMX6 pwm frequency"},
	{"stop",  stop,  METH_VARARGS, "Toradex Apalis/Colibri iMX6 pwm stop" },
    {NULL, NULL}
};


PyDoc_STRVAR(pwm_module_documentation,
"The functions in this module allow Toradex Apalis/Colibri iMX6 using\n"
"\n"
"\n");

static struct PyModuleDef pwm = {
        PyModuleDef_HEAD_INIT,
        "Toradex Apalis/Colibri iMX6",
        pwm_module_documentation,
        -1,
        pwm_methods,
        NULL,
        NULL,
        NULL,
        del_pwm
};

/*
	Python init function.
*/
PyMODINIT_FUNC
PyInit_pwm(void)
{
	PyObject *m;
	
    m = PyModule_Create(&pwm);
    if (m == NULL)
        return NULL;

	PWMError = PyErr_NewException("pwm.error", NULL, NULL);
    if (PWMError != NULL) {
        Py_INCREF(PWMError);
        PyModule_AddObject(m, "error", PWMError);
    }

    PyModule_AddStringConstant(m, "__version__", "1.0");
	PyModule_AddStringConstant(m, "__author__", "Heyn"); 

    return m;
}
