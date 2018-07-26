#include "stdafx.h"
#include "NIDAQ.h"

double NIDAQ::positionToVoltage(double position) {
	return (position / m_mumPerVolt);
}

double NIDAQ::voltageToPosition(double voltage) {
	return (voltage * m_mumPerVolt);
}

NIDAQ::NIDAQ() noexcept {
	m_availablePresets = {};
	m_availableElements = {};
}

NIDAQ::~NIDAQ() {
	disconnectDevice();
}

bool NIDAQ::connectDevice() {
	if (!m_isConnected) {
		// Connect to DAQ
		DAQmxCreateTask("", &taskHandle);
		// Configure analog output channels
		DAQmxCreateAOVoltageChan(taskHandle, "Dev1/ao0:1", "", -1.0, 1.0, DAQmx_Val_Volts, "");
		// Start DAQ task
		DAQmxStartTask(taskHandle);
		m_isConnected = true;
		m_isCompatible = true;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

bool NIDAQ::disconnectDevice() {
	if (m_isConnected) {
		// Stop and clear DAQ task
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		m_isConnected = false;
		m_isCompatible = false;
	}
	emit(connectedDevice(m_isConnected && m_isCompatible));
	return m_isConnected && m_isCompatible;
}

void NIDAQ::init() {
}

void NIDAQ::setElement(ScanControl::DEVICE_ELEMENT element, int position) {
}

void NIDAQ::setElements(ScanControl::SCAN_PRESET preset) {
}

void NIDAQ::getElements() {
}

void NIDAQ::setPosition(POINT3 position) {
	voltages.chA = positionToVoltage(position.x);
	voltages.chB = positionToVoltage(position.y);
	float64 data[2] = { voltages.chA, voltages.chB };
	DAQmxWriteAnalogF64(taskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
}

void NIDAQ::movePosition(POINT3 distance) {
}

void NIDAQ::setPositionRelativeX(double positionX) {
	voltages.chA = positionToVoltage(positionX);
	float64 data[2] = { voltages.chA, voltages.chB };
	DAQmxWriteAnalogF64(taskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
}

void NIDAQ::setPositionRelativeY(double positionY) {
	voltages.chB = positionToVoltage(positionY);
	float64 data[2] = { voltages.chA, voltages.chB };
	DAQmxWriteAnalogF64(taskHandle, 1, true, 10.0, DAQmx_Val_GroupByChannel, data, NULL, NULL);
}

void NIDAQ::setPositionRelativeZ(double positionZ) {
}

POINT3 NIDAQ::getPosition() {
	double x = voltageToPosition(voltages.chA);
	double y = voltageToPosition(voltages.chB);
	return POINT3{ x, y, 0 };
}