#include "stdafx.h"
#include <iostream>
#include "andor.h"
#include "logger.h"
#include <windows.h>

Andor::Andor(QObject *parent)
	: QObject(parent) {
	m_abort = false;
	int i_retCode;
	i_retCode = AT_InitialiseLibrary();
	if (i_retCode != AT_SUCCESS) {
		//error condition, check atdebug.log file
	} else {
		initialised = TRUE;
	}
}

Andor::~Andor() {
	if (connected) {
		AT_Close(Hndl);
	}
	AT_FinaliseLibrary();
}

void Andor::connect() {
	if (!connected) {
		int i_retCode = AT_Open(0, &Hndl);
		if (i_retCode == AT_SUCCESS) {
			connected = TRUE;
		}
	}
}

void Andor::disconnect() {
	if (connected) {
		int i_retCode = AT_Close(Hndl);
		if (i_retCode == AT_SUCCESS) {
			connected = FALSE;
		}
	}
}

bool Andor::getConnectionStatus() {
	return connected;
}

void Andor::setSensorCooling(bool cooling) {
	int i_retCode = AT_SetBool(Hndl, L"SensorCooling", cooling);
}

bool Andor::getSensorCooling() {
	AT_BOOL szValue;
	int i_retCode = AT_GetBool(Hndl, L"SensorCooling", &szValue);
	return szValue;
}

const wchar_t Andor::getTemperatureStatus() {
	int i_retCode;
	i_retCode = AT_GetEnumIndex(Hndl, L"TemperatureStatus", &temperatureStatusIndex);
	AT_GetEnumStringByIndex(Hndl, L"TemperatureStatus", temperatureStatusIndex, temperatureStatus, 256);
	return *temperatureStatus;
}

double Andor::getSensorTemperature() {
	double szValue;
	int i_retCode = AT_GetFloat(Hndl, L"SensorTemperature", &szValue);
	return szValue;
}

void Andor::checkCamera() {
	Sleep(5000);
	qInfo(logInfo()) << "Checking camera.";
	int i_retCode;
	AT_64 iNumberDevices = 0;
	i_retCode = AT_GetInt(AT_HANDLE_SYSTEM, L"DeviceCount", &iNumberDevices);
	if (iNumberDevices <= 0) {
		// No cameras found, check all redistributable binaries
		// have been copied to the executable directory or are in the system path
		// and check atdebug.log file
	} else {
		if (!connected) {
			i_retCode = AT_Open(0, &Hndl);
			if (i_retCode != AT_SUCCESS) {
				//error condition - check atdebug.log
			} else {
				connected = TRUE;
			}
		}
		AT_WC szValue[64];
		i_retCode = AT_GetString(Hndl, L"SerialNumber", szValue, 64);
		if (i_retCode == AT_SUCCESS) {
			//The serial number of the camera is szValue
			qInfo(logInfo()) << "The serial number is" << szValue;
			//wcout << L"The serial number is " << szValue << endl;
		} else {
			//Serial Number feature was not found, check the error code for information
		}
	}
}

void Andor::getImages() {
	qInfo(logInfo()) << "Acquisition started.";
	Sleep(5000);
	qInfo(logInfo()) << "Acquisition finished.";
}