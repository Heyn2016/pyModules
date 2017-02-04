# -*- coding:utf8 -*-
""" FP7 """
# !/usr/bin/python
# Python:   3.5.2
# Platform: ARMv7
# Author:   Heyn
# Program:  
# History:  2017/01/18 V1.0.0[Heyn]

import time
import pymodbus


def caller(readlist, len = 1):	
	msg = 'Code = ' + str(readlist[0]) + ' Addr = %4d'%(readlist[1]) + '  =>  '

	try:
		msg += str(pymodbus.read_registers(readlist, len))
		print(msg)
	except BaseException as err:
		print(err)
		return msg + str(err) + '\r\n'

	return msg + '\r\n'

if __name__ == '__main__':
	print(pymodbus.new_rtu('/dev/ttymxc1'))
	#print(pymodbus.new_tcp('192.168.5.119'))

	cmds = [[3,1,'D32'], [3,10,'U16'], [3,12,'S16'], [3,20,'F16'], [1,1,'B08'], [1,1,'B08'], [4,0,'U32'], [4,0x07D0,'S32']]

	while(True):
		data = ''
		for cmd in cmds :
			data += caller(cmd, 2)
		time.sleep(2)
