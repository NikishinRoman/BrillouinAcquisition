#include "stdafx.h"
#include "simplemath.h"
#include "ODT.h"

ODT::ODT(QObject *parent, PointGrey **pointGrey, ScanControl **scanControl)
	: QObject(parent), m_pointGrey(pointGrey), m_scanControl(scanControl) {
}

ODT::~ODT() {
	m_acqRunning = false;
	m_algnRunning = false;
}

bool ODT::isAcqRunning() {
	return m_acqRunning;
}

bool ODT::isAlgnRunning() {
	return m_algnRunning;
}

void ODT::setAlgnSettings(ODT_SETTINGS settings) {
	m_algnSettings = settings;
	calculateVoltages(ALGN);
}

void ODT::setAcqSettings(ODT_SETTINGS settings) {
	m_acqSettings = settings;
	calculateVoltages(ACQ);
}

void ODT::setSettings(ODT_MODES mode, ODT_SETTING settingType, double value) {
	ODT_SETTINGS *settings;
	if (mode == ACQ) {
		settings = &m_acqSettings;
	} else if (mode == ALGN) {
		settings = &m_algnSettings;
	} else {
		return;
	}

	switch (settingType) {
		case VOLTAGE:
			settings->radialVoltage = value;
			calculateVoltages(mode);
			break;
		case NRPOINTS:
			settings->numberPoints = value;
			calculateVoltages(mode);
			break;
		case SCANRATE:
			settings->scanRate = value;
			break;
	}
}

void ODT::initialize() {
	calculateVoltages(ALGN);
	calculateVoltages(ACQ);
}

void ODT::calculateVoltages(ODT_MODES mode) {
	if (mode == ALGN) {
		double Ux{ 0 };
		double Uy{ 0 };
		std::vector<double> theta = simplemath::linspace<double>(0, 360, m_algnSettings.numberPoints);
		m_algnSettings.voltages.resize(theta.size());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {
			Ux = m_algnSettings.radialVoltage * cos(theta[i]* M_PI / 180);
			Uy = m_algnSettings.radialVoltage * sin(theta[i]* M_PI / 180);
			m_algnSettings.voltages[i] = { Ux, Uy };
		}
		emit(s_algnSettingsChanged(m_algnSettings));
	}
	if (mode == ACQ) {
		m_acqSettings.voltages.clear();
		if (m_acqSettings.numberPoints < 10) {
			return;
		}
		double Ux{ 0 };
		double Uy{ 0 };

		int n3 = round(m_acqSettings.numberPoints / 3);

		std::vector<double> theta = simplemath::linspace<double>(2*M_PI, 0, n3);
		theta.erase(theta.begin());
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * r*cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * r*sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, n3);
		theta.erase(theta.begin());
		theta.erase(theta.end()-1);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			double r = sqrt(abs(theta[i]));
			Ux = m_acqSettings.radialVoltage * -r*cos(theta[i]) / sqrt(2 * M_PI);
			Uy = m_acqSettings.radialVoltage * -r*sin(theta[i]) / sqrt(2 * M_PI);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		theta = simplemath::linspace<double>(0, 2 * M_PI, m_acqSettings.numberPoints - 2 * n3 + 3);
		theta = simplemath::linspace<double>(0, theta.end()[-2], m_acqSettings.numberPoints - 2 * n3 + 3);
		for (gsl::index i{ 0 }; i < theta.size(); i++) {

			Ux = -1* m_acqSettings.radialVoltage * cos(theta[i]);
			Uy = -1* m_acqSettings.radialVoltage * sin(theta[i]);

			m_acqSettings.voltages.push_back({ Ux, Uy });
		}

		m_acqSettings.numberPoints = m_acqSettings.voltages.size();

		emit(s_acqSettingsChanged(m_acqSettings));
	}
}