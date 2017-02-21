
#define PY_SSIZE_T_CLEAN

#include <dlfcn.h>
#include <string.h>
#include <Python.h>

#include <errno.h>
#include "modbus/modbus.h"
#include "modbus/modbus-rtu.h"
#include "modbus/modbus-tcp.h"
#include "modbus/modbus-version.h"



static PyObject *PyModbusError;

static modbus_t *mb;
static int slave_addr = 1;

enum PY_DATA_TYPE {
	PY_DATA_U16 = 0x00,
	PY_DATA_U32,
	PY_DATA_S16,
	PY_DATA_S32,
	PY_DATA_F16,
	PY_DATA_D32,
	PY_DATA_B08
};

//PyCode : pymodbus.new_rtu('/dev/ttymxc1', 9600, b'N', 8, 1)
//PyCode : pymodbus.new_rtu('/dev/ttymxc1')
static PyObject* new_rtu(PyObject* self, PyObject* args)
{
	/*
		Toradex imx6 (linux)
		device -> 	UART2		/dev/ttymxc1
					UART3		/dev/ttymxc3
					UART4		/dev/ttymxc4
	*/

	char *device;
	char parity = 'N';
	int baud = 9600, data_bit = 8, stop_bit = 1;

	if(!PyArg_ParseTuple(args, "s|icii", &device, &baud, &parity, &data_bit, &stop_bit)) {
		PyErr_SetString(PyModbusError, "Invalid input parameter.");
        return NULL;
    }

	mb = modbus_new_rtu(device, baud, parity, data_bit, stop_bit);

	if (mb == NULL) {
		PyErr_SetString(PyModbusError, modbus_strerror(errno));
		return NULL;
	}

	if (-1 == modbus_connect(mb)) {
		PyErr_SetString(PyModbusError, modbus_strerror(errno));
		return NULL;
	}

	modbus_set_slave(mb, slave_addr);

	Py_RETURN_TRUE;
}

// PyCode : pymodbus.new_tcp('192.168.5.100',502)
static PyObject* new_tcp(PyObject* self, PyObject* args)
{
	char *ip;
	int port = 502;
	
	if(!PyArg_ParseTuple(args, "s|i", &ip, &port)) {
		PyErr_SetString(PyModbusError, "Invalid input parameter.");
        return NULL;
    }

	mb = modbus_new_tcp(ip, port);
	if (mb == NULL) {
		PyErr_SetString(PyModbusError, modbus_strerror(errno));
		return NULL;
	}
	
	if (-1 == modbus_connect(mb))
	{
		PyErr_SetString(PyModbusError, modbus_strerror(errno));
		return NULL;
	}

	modbus_set_slave(mb, slave_addr);

	return Py_BuildValue("O", Py_True);
}

//PyCode : pymodbus.set_slave(1)
static PyObject* set_slave(PyObject* self, PyObject* args)
{
	if(!PyArg_ParseTuple(args, "i", &slave_addr))
	{
		PyErr_SetString(PyModbusError, "Invalid input parameter.");
        return NULL;
    }

	if (slave_addr < 0 || slave_addr > 247) {
		PyErr_SetString(PyModbusError, "Invalid slave_addr = [0 .. 247]");
		return NULL;
	}

	modbus_set_slave(mb, slave_addr);

	return Py_BuildValue("O", Py_True);
}

static PyObject* set_timeout(PyObject* self, PyObject* args)
{
	unsigned int sec = 0, usec = 0;
	
	if(!PyArg_ParseTuple(args, "i|i", &sec, &usec))
	{
		PyErr_SetString(PyModbusError, "Invalid input parameter.");
        return NULL;
    }
	
	modbus_set_response_timeout(mb, sec, usec);
	
	Py_RETURN_TRUE;
}


//PyCode : pymodbus.read_registers([3, 0000, 'U16'], 2)
static PyObject* read_registers(PyObject* self, PyObject* args)
{
	PyObject * listObj = NULL;
	
	unsigned short u16_dest[MODBUS_MAX_READ_REGISTERS] = {0x00};
	unsigned char  u08_dest[MODBUS_MAX_READ_REGISTERS] = {0x00};

	int i, j, addr, nb = 1 , pynb = 1;
	int regs = 0, index = 0, code = 0;

	const char type[7][4] = {"U16", "U32", "S16", "S32", "F16", "D32", "B08"};

	char *type_name;
	
	if (! PyArg_ParseTuple( args, "O!|i", &PyList_Type, &listObj, &pynb)) {
		PyErr_SetString(PyModbusError, "Invalid input parameter.");
		return NULL;
	}

	if (!PyList_Check(listObj)) {
		PyErr_SetString(PyModbusError, "Invalid object. It's must be a list object.");
		return NULL;
	}

	if (PyList_Size(listObj) != 3) {
		PyErr_SetString(PyModbusError, "Invalid input data size.");
		return NULL;
	}

	code = (int)PyLong_AsLong(PyList_GetItem(listObj, 0));
	addr = (int)PyLong_AsLong(PyList_GetItem(listObj, 1));
	type_name = PyUnicode_AsUTF8(PyList_GetItem(listObj, 2));

	for (index = 0; index < (sizeof(type) / sizeof(type[0])); index++) {
		if (!memcmp(type[index], type_name, strlen(type[index]))) {
			switch (index) {
				case PY_DATA_U16: case PY_DATA_S16: case PY_DATA_B08:
					nb = 1 * pynb;
					break;
				case PY_DATA_U32: case PY_DATA_S32: case PY_DATA_F16:
					nb = 2 * pynb;
					break;
				case PY_DATA_D32:
					nb = 4 * pynb;
					break;
				default:
					break;
			}
			break;
		}
	}

	if (index >= (sizeof(type) / sizeof(type[0]))) {
		PyErr_Format(PyModbusError, "Invalid data type [%s]. Vaild type (U16, U32, S16, S32, F16, D32, B08)" , type_name);
		return NULL;
	}

	switch (code) {
		case 0x01:
			regs = modbus_read_input_bits(mb, addr, nb, u08_dest);
			break;
		case 0x02:
			regs = modbus_read_bits(mb, addr, nb, u08_dest);
			break;		
		case 0x03:
			regs = modbus_read_registers(mb, addr, nb, u16_dest);
			break;
		case 0x04:
			regs = modbus_read_input_registers(mb, addr, nb, u16_dest);
			break;
		default:
			PyErr_SetString(PyModbusError, "Invalid function code");
			return NULL;		
			break;												
	}

	if (-1 == regs) {
		PyErr_SetString(PyModbusError, modbus_strerror(errno));
		return NULL;
	}

	PyObject* pList = PyList_New(pynb);

	switch (index) {
		case PY_DATA_U16:	// U16
			for (i = 0; i < pynb; i++) {
				PyList_SetItem(pList, i, Py_BuildValue("h", u16_dest[i]));
			}
			break;

		case PY_DATA_U32:	// U32
			for (i = 0; i < pynb; i++) {
				unsigned int ieee = 0;
				for (j = 0 ; j < 2; j++) {
					ieee += (unsigned int)u16_dest[j + i*2] << j*16;
				}
				PyList_SetItem(pList, i, Py_BuildValue("i", (unsigned int)ieee));
			}		
			break;

		case PY_DATA_S16:	// S16
			for (i = 0; i < pynb; i++) {
				PyList_SetItem(pList, i, Py_BuildValue("h", (signed short)u16_dest[i]));
			}
			break;

		case PY_DATA_S32:	// S32
			for (i = 0; i < pynb; i++) {
				int ieee = 0;
				for (j = 0 ; j < 2; j++) {
					ieee += (int)u16_dest[j + i*2] << j*16;
				}
				PyList_SetItem(pList, i, Py_BuildValue("i", (int)ieee));
			}
			break;

		case PY_DATA_F16:	// F16
			for (i = 0; i < pynb; i++) {
				float value = 0.0;
				int ieee = 0;
				for (j = 0 ; j < 2; j++) {
					ieee += (int)u16_dest[j + i*2] << j*16;
				}
				memcpy(&value, &ieee, sizeof(float));
				PyList_SetItem(pList, i, Py_BuildValue("f", value));
			}
			break;

		case PY_DATA_D32:	// D32
			for (i = 0; i < pynb; i++) {
				double double_value = 0.0;
				long long int64_ieee = 0;
				for (j = 0; j < 4; j++) {
					int64_ieee += (long long)u16_dest[j + i*4] << j*16;
				}
				memcpy(&double_value, &int64_ieee, sizeof(double));
				PyList_SetItem(pList, i, Py_BuildValue("d", double_value));
			}
			break;

		case PY_DATA_B08:	// B08
			for (i = 0; i < pynb; i++) {
				PyList_SetItem(pList, i, Py_BuildValue("B", (unsigned char)u08_dest[i]));
			}
			break;

		default:
			break;																
	}
	return pList;
}

static PyObject* free_rtu(PyObject* self, PyObject *noarg)
{
	modbus_close(mb);  
  	modbus_free(mb);

	Py_RETURN_NONE;
}


static PyObject* free_tcp(PyObject* self, PyObject *noarg)
{
	modbus_close(mb);  
  	modbus_free(mb);

	Py_RETURN_NONE;
}


static PyMethodDef pymodbus_methods[] =
{
	{"new_rtu", new_rtu, METH_VARARGS, "libmodbus-3.1.4 RTU!"},
	{"new_tcp", new_tcp, METH_VARARGS, "libmodbus-3.1.4 TCP!"},

	{"set_timeout", set_timeout, METH_VARARGS, "modbus_set_response_timeout!"},
	{"set_slave", set_slave, METH_VARARGS, "modbus_set_slave!"},
	{"read_registers", read_registers, METH_VARARGS, "libmodbus-3.1.4 Read Registers!"},

	{"free_rtu", free_rtu, METH_NOARGS, "free_rtu!"},
	{"free_tcp", free_tcp, METH_NOARGS, "free_tcp!"},

	
    {NULL, NULL}
};


PyDoc_STRVAR(pymodbus_module_documentation,
"Python wrapper for libmodbus\n"
"Python Interface for libmodbus written with CFFI. This libmodbus wrapper is compatible with Python 3.5+\n"
"Required packages : libmodbus \n"
"\n");

static struct PyModuleDef pymodbus = {
        PyModuleDef_HEAD_INIT,
        "libmodbus-3.1.4",
        pymodbus_module_documentation,
        -1,
        pymodbus_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

/*
	Python init function.
*/
PyMODINIT_FUNC
PyInit_pymodbus(void)
{
	PyObject *m, *ver;
	
    m = PyModule_Create(&pymodbus);
    if (m == NULL)
        return NULL;

	PyModbusError = PyErr_NewException("pymodbus.error", NULL, NULL);
    if (PyModbusError != NULL) {
        Py_INCREF(PyModbusError);
        PyModule_AddObject(m, "error", PyModbusError);
    }

    ver = PyUnicode_FromString(LIBMODBUS_VERSION_STRING);
    if (ver != NULL) {
        PyModule_AddObject(m, "LIBMODBUS_VERSION_STRING", ver);
	}

    PyModule_AddStringConstant(m, "__version__", "0.0.2");
	PyModule_AddStringConstant(m, "__author__", "Heyn"); 

    return m;
}
