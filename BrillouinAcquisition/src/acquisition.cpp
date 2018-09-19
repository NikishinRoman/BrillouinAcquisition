#include "stdafx.h"
#include "acquisition.h"
#include "simplemath.h"
#include "logger.h"
#include "filesystem"

using namespace std::experimental::filesystem::v1;


Acquisition::Acquisition(QObject *parent)
	: QObject(parent) {
}

Acquisition::~Acquisition() {
	m_modeRunning = ACQUISITION_MODE::NONE;
}

ACQUISITION_MODE Acquisition::isAcqRunning() {
	return m_modeRunning;
}

void Acquisition::newAcquisition(StoragePath path) {
	openAcquisition(path, H5F_ACC_TRUNC);
}

void Acquisition::openAcquisition(StoragePath path, int flag) {
	// if an acquisition is running, do nothing
	if (m_modeRunning != ACQUISITION_MODE::NONE) {
		emit(s_acqModeRunning(m_modeRunning));
		return;
	}
	m_path = path;
	
	emit(s_filenameChanged(m_path.filename));
	m_storage = std::make_unique <StorageWrapper>(nullptr, m_path, flag);
}

void Acquisition::openAcquisition() {
	StoragePath defaultPath = StoragePath{};
	defaultPath = checkFilename(defaultPath);

	openAcquisition(defaultPath);
}

void Acquisition::newRepetition(ACQUISITION_MODE mode) {
	if (m_storage == nullptr) {
		openAcquisition();
	}
	m_storage->newRepetition(mode);
}

void Acquisition::closeAcquisition() {
	m_storage.reset();
}

void Acquisition::setAcquisitionState(ACQUISITION_MODE mode, ACQUISITION_STATE state) {
	switch (mode) {
		case ACQUISITION_MODE::BRILLOUIN:
			m_states.Brillouin = state;
			break;
		case ACQUISITION_MODE::ODT:
			m_states.ODT = state;
			break;
		case ACQUISITION_MODE::FLUORESCENCE:
			m_states.Fluorescence = state;
			break;
		default:
			break;
	}
}

bool Acquisition::isModeRunning(ACQUISITION_MODE mode) {
	return (bool)(m_modeRunning & mode);
}

/*
 * Function checks if starting an acquisition of given mode is allowed.
 * If yes, it adds the requested mode to the currently running acquisition to modes.
 */
bool Acquisition::startMode(ACQUISITION_MODE mode) {
	// If no acquisition file is open, open one.
	if (m_storage == nullptr) {
		openAcquisition();
	}

	// Check that the requested mode is not already running.
	if ((bool)(mode & m_modeRunning)) {
		return false;
	}
	// Check, that Brillouin and ODT don't run simultaneously.
	if (((mode | m_modeRunning) & (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT))
		== (ACQUISITION_MODE::BRILLOUIN | ACQUISITION_MODE::ODT)) {
		return false;
	}
	// Add the requested mode to the running modes.
	m_modeRunning |= mode;
	emit(s_acqModeRunning(m_modeRunning));
	return true;
}

/* 
 * Stops the selected mode.
 */
void Acquisition::stopMode(ACQUISITION_MODE mode) {
	m_modeRunning &= ~mode;
	emit(s_acqModeRunning(m_modeRunning));
}

void Acquisition::checkFilename() {
	m_path = checkFilename(m_path);
}

StoragePath Acquisition::checkFilename(StoragePath desiredPath) {
	std::string oldFilename = desiredPath.filename;
	// get filename without extension
	std::string rawFilename = oldFilename.substr(0, oldFilename.find_last_of("."));
	// remove possibly attached number separated by a hyphen
	rawFilename = rawFilename.substr(0, rawFilename.find_last_of("-"));
	desiredPath.fullPath = desiredPath.folder + "/" + desiredPath.filename;
	int count = 0;
	while (exists(desiredPath.fullPath)) {
		desiredPath.filename = rawFilename + '-' + std::to_string(count) + oldFilename.substr(oldFilename.find_last_of("."), std::string::npos);
		desiredPath.fullPath = desiredPath.folder + "/" + desiredPath.filename;
		count++;
	}
	if (desiredPath.filename != oldFilename) {
		emit(s_filenameChanged(desiredPath.filename));
	}
	return desiredPath;
}