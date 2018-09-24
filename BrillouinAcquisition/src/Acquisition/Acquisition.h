#ifndef ACQUISITION_H
#define ACQUISITION_H

#include "../storageWrapper.h"
#include "../thread.h"

enum class ACQUISITION_STATE {
	STARTED,
	RUNNING,
	FINISHED,
	ABORTED
};

struct ACQUISITION_STATES {
	ACQUISITION_STATE Brillouin = ACQUISITION_STATE::FINISHED;
	ACQUISITION_STATE ODT = ACQUISITION_STATE::FINISHED;
	ACQUISITION_STATE Fluorescence = ACQUISITION_STATE::FINISHED;
};

struct REPETITIONS {
	int count = 1;			// [1]		number of repetitions
	double interval = 10;	// [min]	interval between repetitions
};

class Acquisition : public QObject {
	Q_OBJECT

public:
	Acquisition(QObject *parent);
	~Acquisition();
	ACQUISITION_MODE getCurrentModes();
	std::unique_ptr <StorageWrapper> m_storage = nullptr;

public slots:
	void init() {};
	// overrides previous acquisition (potential dataloss)
	void newFile(StoragePath path);
	/*
	 * Opens acquisition and adds new repetitions without overriding by default.
	 * Different behaviour can be specified by supplying a different flag.
	 */
	void openFile(StoragePath path, int flag = H5F_ACC_RDWR);
	void openFile();
	void newRepetition(ACQUISITION_MODE mode);
	void closeFile();
	void setAcquisitionState(ACQUISITION_MODE mode, ACQUISITION_STATE state);
	
	bool isModeEnabled(ACQUISITION_MODE mode);

	bool enableMode(ACQUISITION_MODE);
	void disableMode(ACQUISITION_MODE);

private:
	StoragePath m_path;
	ACQUISITION_MODE m_currentModes = ACQUISITION_MODE::NONE;	// which mode is currently acquiring
	ACQUISITION_STATES m_states;								// state of the acquisition modes

private slots:
	void checkFilename();
	StoragePath checkFilename(StoragePath desiredPath);

signals:
	void s_currentModes(ACQUISITION_MODE);	// which acquisition mode is running
	void s_filenameChanged(std::string);
	void s_openFileFailed();
};

#endif //ACQUISITION_H