#ifndef ACQUISITIONMODE_H
#define ACQUISITIONMODE_H

#include <QtCore>
#include <gsl/gsl>

#include "../Acquisition.h"

class AcquisitionMode : public QObject {
	Q_OBJECT

public:
	AcquisitionMode(QObject *parent, Acquisition *acquisition);
	~AcquisitionMode();
	bool m_abort = false;

public slots:
	void init() {};
	virtual void startRepetitions() = 0;

protected:
	Acquisition *m_acquisition = nullptr;
	virtual void abortMode() = 0;

private slots:
	virtual void acquire(std::unique_ptr <StorageWrapper> & storage) = 0;

signals:
	void s_repetitionProgress(ACQUISITION_STATE, double, int);	// progress in percent and the remaining time in seconds
	void s_totalProgress(int, int);								// repetitions
	void s_acqRunning(bool);									// is the acquisition running
};

#endif //ACQUISITIONMODE_H