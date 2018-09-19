#include "stdafx.h"
#include "AcquisitionMode.h"

AcquisitionMode::AcquisitionMode(QObject *parent, Acquisition *acquisition)
	: QObject(parent), m_acquisition(acquisition) {
}

AcquisitionMode::~AcquisitionMode() {
}

void AcquisitionMode::abort() {
	m_abort = true;
}