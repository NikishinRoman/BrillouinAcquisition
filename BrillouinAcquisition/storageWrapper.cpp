#include "stdafx.h"
#include "storageWrapper.h"
#include "logger.h"


StorageWrapper::~StorageWrapper() {
	// clear image queue in case acquisition was aborted
	// and the queue is still filled
	if (m_abort) {
		while (!m_payloadQueue.isEmpty()) {
			IMAGE *img = m_payloadQueue.dequeue();
			delete img;
		}
		while (!m_calibrationQueue.isEmpty()) {
			CALIBRATION *cal = m_calibrationQueue.dequeue();
			delete cal;
		}
	}
}

void StorageWrapper::s_enqueuePayload(IMAGE *img) {
	m_payloadQueue.enqueue(img);
}

void StorageWrapper::s_enqueueCalibration(CALIBRATION *cal) {
	m_calibrationQueue.enqueue(cal);
}

void StorageWrapper::s_finishedQueueing() {
	m_finishedQueueing = true;
}

void StorageWrapper::startWritingQueues() {
	m_observeQueues = true;
	m_finishedQueueing = false;
	s_writeQueues();
}

void StorageWrapper::stopWritingQueues() {
	m_observeQueues = false;
}

void StorageWrapper::s_writeQueues() {
	while (!m_payloadQueue.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		IMAGE *img = m_payloadQueue.dequeue();
		setPayloadData(img->indX, img->indY, img->indZ, img->data, img->rank, img->dims, img->date);
		//std::string info = "Image written " + std::to_string(m_writtenImagesNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenImagesNr++;
		delete img;
		img = nullptr;
	}

	while (!m_calibrationQueue.isEmpty()) {
		if (m_abort) {
			m_finished = true;
			return;
		}
		CALIBRATION *cal = m_calibrationQueue.dequeue();
		setCalibrationData(cal->index, cal->data, cal->rank, cal->dims, cal->sample, cal->shift, cal->date);
		//std::string info = "Calibration written " + std::to_string(m_writtenCalibrationsNr);
		//qInfo(logInfo()) << info.c_str();
		m_writtenCalibrationsNr++;
		delete cal;
		cal = nullptr;
	}

	// continue to write queues if
	// m_observeQueues is true
	// m_abort was not set from the outside
	// m_finishedQeueuing shows that more queue entries will come
	if (m_observeQueues && !m_abort && !m_finishedQueueing) {
		QMetaObject::invokeMethod(this, "s_writeQueues", Qt::QueuedConnection);
	} else {
		m_finished = true;
		emit finished();
	}
}