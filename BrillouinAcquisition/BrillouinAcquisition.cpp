#include "stdafx.h"
#include "BrillouinAcquisition.h"
#include "version.h"
#include "logger.h"
#include "math.h"

BrillouinAcquisition::BrillouinAcquisition(QWidget *parent):
	QMainWindow(parent), ui(new Ui::BrillouinAcquisitionClass) {
	ui->setupUi(this);

	// slot for newly acquired images
	QWidget::connect(
		andor,
		SIGNAL(imageAcquired(unsigned short*, AT_64, AT_64)),
		this,
		SLOT(onNewImage(unsigned short*, AT_64, AT_64))
	);

	// slot to limit the axis of the camera display after user interaction
	QWidget::connect(
		ui->customplot->xAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(xAxisRangeChanged(QCPRange))
	);
	QWidget::connect(
		ui->customplot->yAxis,
		SIGNAL(rangeChanged(QCPRange)),
		this,
		SLOT(yAxisRangeChanged(QCPRange))
	);

	// slots to update the settings inputs
	QWidget::connect(
		this,
		SIGNAL(settingsCameraChanged(SETTINGS_DEVICES)),
		this,
		SLOT(settingsCameraUpdate(SETTINGS_DEVICES))
	);

	QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
	ui->settingsWidget->setTabIcon(0, icon);
	ui->settingsWidget->setTabIcon(1, icon);
	ui->settingsWidget->setTabIcon(2, icon);
	ui->settingsWidget->setIconSize(QSize(16, 16));

	ui->actionEnable_Cooling->setEnabled(FALSE);

	// start camera worker thread
	CameraThread.startWorker(andor);

	// set up the camera image plot
	BrillouinAcquisition::createCameraImage();

	writeExampleH5bmFile();

	// Set up GUI
	std::vector<std::string> groupLabels = {"Reflector", "Objective", "Tubelens", "Baseport", "Sideport", "Mirror"};
	std::vector<int> maxOptions = { 5, 6, 3, 3, 3, 2 };
	std::vector<std::string> presetLabels = { "Brillouin", "Brightfield", "Eyepiece", "Calibration" };
	QVBoxLayout *verticalLayout = new QVBoxLayout;
	verticalLayout->setAlignment(Qt::AlignTop);
	std::string buttonLabel;
	QHBoxLayout *presetLayoutLabel = new QHBoxLayout();
	std::string presetLabelString = "Presets:";
	QLabel *presetLabel = new QLabel(presetLabelString.c_str());
	presetLayoutLabel->addWidget(presetLabel);
	verticalLayout->addLayout(presetLayoutLabel);
	QHBoxLayout *layout = new QHBoxLayout();
	for (int ii = 0; ii < presetLabels.size(); ii++) {
		buttonLabel = std::to_string(ii + 1);
		QPushButton *button = new QPushButton(presetLabels[ii].c_str());
		button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		button->setMinimumWidth(90);
		button->setMaximumWidth(90);
		layout->addWidget(button);
	}
	verticalLayout->addLayout(layout);
	for (int ii = 0; ii < groupLabels.size(); ii++) {
		QHBoxLayout *layout = new QHBoxLayout();

		layout->setAlignment(Qt::AlignLeft);
		QLabel *groupLabel = new QLabel(groupLabels[ii].c_str());
		groupLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		groupLabel->setMinimumWidth(80);
		groupLabel->setMaximumWidth(80);
		layout->addWidget(groupLabel);
		for (int jj = 0; jj < maxOptions[ii]; jj++) {
			buttonLabel = std::to_string(jj + 1);
			QPushButton *button = new QPushButton(buttonLabel.c_str());
			button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			button->setMinimumWidth(40);
			button->setMaximumWidth(40);
			layout->addWidget(button);
		}
		verticalLayout->addLayout(layout);
	}
	ui->beamPathBox->setLayout(verticalLayout);
}

BrillouinAcquisition::~BrillouinAcquisition() {
	CameraThread.exit();
	delete andor;
	qInfo(logInfo()) << "BrillouinAcquisition closed.";
	delete ui;
}

void BrillouinAcquisition::writeExampleH5bmFile() {
	h5bm = new H5BM(0, "Brillouin-0.h5", H5F_ACC_RDWR);

	std::string now = QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc())
		.toString(Qt::ISODate).toStdString();

	h5bm->setDate(now);
	std::string date = h5bm->getDate();

	std::string version = h5bm->getVersion();

	std::string commentIn = "Brillouin data";
	h5bm->setComment(commentIn);
	std::string commentOut = h5bm->getComment();

	int resolutionX, resolutionY, resolutionZ;

	resolutionX = 11;
	resolutionY = 13;
	resolutionZ = 2;

	h5bm->setResolution("x", resolutionX);
	h5bm->setResolution("y", resolutionY);
	h5bm->setResolution("z", resolutionZ);

	int resolutionXout = h5bm->getResolution("x");

	// Create position vector
	double minX, maxX, minY, maxY, minZ, maxZ;
	minX = -10;
	maxX = 10;
	minY = -20;
	maxY = 20;
	minZ = 0;
	maxZ = 10;
	int nrPositions = resolutionX * resolutionY * resolutionZ;
	std::vector<double> positionsX(nrPositions);
	std::vector<double> positionsY(nrPositions);
	std::vector<double> positionsZ(nrPositions);
	std::vector<double> posX = math::linspace(minX, maxX, resolutionX);
	std::vector<double> posY = math::linspace(minY, maxY, resolutionY);
	std::vector<double> posZ = math::linspace(minZ, maxZ, resolutionZ);
	int ll = 0;
	for (int ii = 0; ii < resolutionZ; ii++) {
		for (int jj = 0; jj < resolutionX; jj++) {
			for (int kk = 0; kk < resolutionY; kk++) {
				positionsX[ll] = posX[jj];
				positionsY[ll] = posY[kk];
				positionsZ[ll] = posZ[ii];
				ll++;
			}
		}
	}

	int rank = 3;
	// For compatibility with MATLAB respect Fortran-style ordering: z, x, y
	hsize_t *dims = new hsize_t[rank];
	dims[0] = resolutionZ;
	dims[1] = resolutionX;
	dims[2] = resolutionY;

	h5bm->setPositions("x", positionsX, rank, dims);
	h5bm->setPositions("y", positionsY, rank, dims);
	h5bm->setPositions("z", positionsZ, rank, dims);
	delete[] dims;

	positionsX = h5bm->getPositions("x");
	positionsY = h5bm->getPositions("y");
	positionsZ = h5bm->getPositions("z");

	// set payload data
	std::vector<double> data(1600);
	int rank_data = 3;
	hsize_t nrImages = 2;
	hsize_t imageWidth = 100;
	hsize_t imageHeight = 80;
	hsize_t dims_data[3] = {nrImages, imageHeight, imageWidth};
	for (int ii = 0; ii < resolutionZ; ii++) {
		for (int jj = 0; jj < resolutionX; jj++) {
			for (int kk = 0; kk < resolutionY; kk++) {
				h5bm->setPayloadData(jj, kk, ii, data, rank, dims_data);
			}
		}
	}

	data = h5bm->getPayloadData(1, 1, 1);
	date = h5bm->getPayloadDate(1, 1, 1);

	h5bm->setBackgroundData(data, rank, dims_data, "2016-05-06T11:11:00+02:00");
	data = h5bm->getBackgroundData();
	date = h5bm->getBackgroundDate();

	h5bm->setCalibrationData(1, data, rank, dims_data, "methanol", 3.799);
	h5bm->setCalibrationData(2, data, rank, dims_data, "water", 5.088);

	delete h5bm;
}

void BrillouinAcquisition::createCameraImage() {
	// QCustomPlotsTest
	// configure axis rect:

	QCustomPlot *customPlot = ui->customplot;

	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
	customPlot->axisRect()->setupFullAxesBox(true);
	customPlot->xAxis->setLabel("x");
	customPlot->yAxis->setLabel("y");

	// this are the selection modes
	customPlot->setSelectionRectMode(QCP::srmZoom);	// allows to select region by rectangle
	customPlot->setSelectionRectMode(QCP::srmNone);	// allows to drag the position
	// the different modes need to be set on keypress (e.g. on shift key)

	// set background color to default light gray
	customPlot->setBackground(QColor(240, 240, 240, 255));
	customPlot->axisRect()->setBackground(Qt::white);

	// set up the QCPColorMap:
	colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
	int nx = 200;
	int ny = 200;
	colorMap->data()->setSize(nx, ny); // we want the color map to have nx * ny data points
	colorMap->data()->setRange(QCPRange(-4, 4), QCPRange(-4, 4)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
																  // now we assign some data, by accessing the QCPColorMapData instance of the color map:
	double x, y;
	unsigned short z;
	//double z;
	for (int xIndex = 0; xIndex<nx; ++xIndex) {
		for (int yIndex = 0; yIndex<ny; ++yIndex) {
			colorMap->data()->cellToCoord(xIndex, yIndex, &x, &y);
			double r = 3 * qSqrt(x*x + y*y) + 1e-2;
			double tmp = x*(qCos(r + 2) / r - qSin(r + 2) / r); // the B field strength of dipole radiation (modulo physical constants)
			tmp += 0.5;
			tmp *= 65536;
			z = tmp;
			colorMap->data()->setCell(xIndex, yIndex, z);
		}
	}

	// turn off interpolation
	colorMap->setInterpolate(false);

	// add a color scale:
	QCPColorScale *colorScale = new QCPColorScale(customPlot);
	customPlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Intensity");

	// set the color gradient of the color map to one of the presets:
	//colorMap->setGradient(QCPColorGradient::gpPolar);

	QCPColorGradient gradient = QCPColorGradient();
	setColormap(&gradient, gpParula);
	colorMap->setGradient(gradient);

	// we could have also created a QCPColorGradient instance and added own colors to
	// the gradient, see the documentation of QCPColorGradient for what's possible.

	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	colorMap->rescaleDataRange();

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
	QCPMarginGroup *marginGroup = new QCPMarginGroup(customPlot);
	customPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale the key (x) and value (y) axes so the whole color map is visible:
	customPlot->rescaleAxes();

	// equal axis spacing
	//customPlot->xAxis->setRange(-1.5, 1.5);
	//customPlot->yAxis->setRange(-2.5, 2.5);
	//customPlot->yAxis->setScaleRatio(customPlot->xAxis, 1.0);
	//customPlot->replot();
	//customPlot->savePng("./export.png");
}

void BrillouinAcquisition::xAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->xAxis->setRange(newRange.bounded(0, 2048));
	if (newRange.lower >= 0) {
		settings.camera.roi.left = newRange.lower;
	}
	int width = newRange.upper - newRange.lower;
	if (width < 2048) {
		settings.camera.roi.width = width;
	}
	emit(settingsCameraChanged(settings));
}

void BrillouinAcquisition::yAxisRangeChanged(const QCPRange &newRange) {
	// checks for certain range
	ui->customplot->yAxis->setRange(newRange.bounded(0, 2048));
	if (newRange.lower >= 0) {
		settings.camera.roi.bottom = newRange.lower;
	}
	int height = newRange.upper - newRange.lower;
	if (height < 2048) {
		settings.camera.roi.height = height;
	}
	emit(settingsCameraChanged(settings));
}

void BrillouinAcquisition::settingsCameraUpdate(SETTINGS_DEVICES settings) {
	ui->ROILeft->setValue(settings.camera.roi.left);
	ui->ROIWidth->setValue(settings.camera.roi.width);
	ui->ROIBottom->setValue(settings.camera.roi.bottom);
	ui->ROIHeight->setValue(settings.camera.roi.height);
}

void BrillouinAcquisition::on_ROILeft_valueChanged(int left) {
	settings.camera.roi.left = left;
	ui->customplot->xAxis->setRange(QCPRange(left, left + settings.camera.roi.width));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROIWidth_valueChanged(int width) {
	settings.camera.roi.width = width;
	ui->customplot->xAxis->setRange(QCPRange(settings.camera.roi.left, settings.camera.roi.left + width));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROIBottom_valueChanged(int bottom) {
	settings.camera.roi.bottom = bottom;
	ui->customplot->yAxis->setRange(QCPRange(bottom, bottom + settings.camera.roi.height));
	ui->customplot->replot();
}

void BrillouinAcquisition::on_ROIHeight_valueChanged(int height) {
	settings.camera.roi.height = height;
	ui->customplot->yAxis->setRange(QCPRange(settings.camera.roi.bottom, settings.camera.roi.bottom + height));
	ui->customplot->replot();
}

void BrillouinAcquisition::onNewImage(unsigned short* unpackedBuffer, AT_64 width, AT_64 height) {

	colorMap->data()->setSize(width, height); // we want the color map to have nx * ny data points
	colorMap->data()->setRange(QCPRange(0, width), QCPRange(0, height)); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions

	double x, y;
	int tIndex;
	for (int xIndex = 0; xIndex<width; ++xIndex) {
		for (int yIndex = 0; yIndex<height; ++yIndex) {
			colorMap->data()->cellToCoord(xIndex, yIndex, &x, &y);
			tIndex = xIndex * height + yIndex;
			colorMap->data()->setCell(xIndex, yIndex, unpackedBuffer[tIndex]);
		}
	}
	colorMap->rescaleDataRange();
	ui->customplot->rescaleAxes();
	ui->customplot->replot();
}

void BrillouinAcquisition::on_actionConnect_Camera_triggered() {
	if (andor->getConnectionStatus()) {
		andor->disconnect();
		ui->actionConnect_Camera->setText("Connect Camera");
		ui->actionEnable_Cooling->setText("Enable Cooling");
		QIcon icon(":/BrillouinAcquisition/assets/00disconnected.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(FALSE);
		ui->camera_playPause->setEnabled(FALSE);
		ui->camera_singleShot->setEnabled(FALSE);
	} else {
		andor->connect();
		ui->actionConnect_Camera->setText("Disconnect Camera");
		QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
		ui->settingsWidget->setTabIcon(0, icon);
		ui->actionEnable_Cooling->setEnabled(TRUE);
		ui->camera_playPause->setEnabled(TRUE);
		ui->camera_singleShot->setEnabled(TRUE);
	}
}

void BrillouinAcquisition::on_actionEnable_Cooling_triggered() {
	if (andor->getConnectionStatus()) {
		if (andor->getSensorCooling()) {
			andor->setSensorCooling(FALSE);
			ui->actionEnable_Cooling->setText("Enable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/01standby.png");
			ui->settingsWidget->setTabIcon(0, icon);
		}
		else {
			andor->setSensorCooling(TRUE);
			ui->actionEnable_Cooling->setText("Disable Cooling");
			QIcon icon(":/BrillouinAcquisition/assets/02cooling.png");
			ui->settingsWidget->setTabIcon(0, icon);
		}
	}
}

void BrillouinAcquisition::on_actionAbout_triggered() {
	QString clean = "Yes";
	if (Version::VerDirty) {
		clean = "No";
	}
	std::string preRelease = "";
	if (Version::PRERELEASE.length() > 0) {
		preRelease = "-" + Version::PRERELEASE;
	}
	QString str = QString("BrillouinAcquisition Version %1.%2.%3%4 <br> Build from commit: <a href='%5'>%6</a><br>Clean build: %7<br>Author: <a href='mailto:%8?subject=BrillouinAcquisition'>%9</a><br>Date: %10")
		.arg(Version::MAJOR)
		.arg(Version::MINOR)
		.arg(Version::PATCH)
		.arg(preRelease.c_str())
		.arg(Version::Url.c_str())
		.arg(Version::Commit.c_str())
		.arg(clean)
		.arg(Version::AuthorEmail.c_str())
		.arg(Version::Author.c_str())
		.arg(Version::Date.c_str());

	QMessageBox::about(this, tr("About BrillouinAcquisition"), str);
}

void BrillouinAcquisition::on_camera_playPause_clicked() {
	// example for synchronous/blocking execution
	andor->acquireStartStop();
}

void BrillouinAcquisition::on_camera_singleShot_clicked() {
	// example for asynchronous/non-blocking execution
	QMetaObject::invokeMethod(andor, "acquireSingle", Qt::QueuedConnection);
}

void BrillouinAcquisition::setColormap(QCPColorGradient *gradient, CustomGradientPreset preset) {
	gradient->clearColorStops();
	switch (preset) {
		case gpParula:
			gradient->setColorInterpolation(QCPColorGradient::ciRGB);
			gradient->setColorStopAt(0.00, QColor( 53,  42, 135));
			gradient->setColorStopAt(0.05, QColor( 53,  62, 175));
			gradient->setColorStopAt(0.10, QColor( 27,  85, 215));
			gradient->setColorStopAt(0.15, QColor(  2, 106, 225));
			gradient->setColorStopAt(0.20, QColor( 15, 119, 219));
			gradient->setColorStopAt(0.25, QColor( 20, 132, 212));
			gradient->setColorStopAt(0.30, QColor( 13, 147, 210));
			gradient->setColorStopAt(0.35, QColor(  6, 160, 205));
			gradient->setColorStopAt(0.40, QColor(  7, 170, 193));
			gradient->setColorStopAt(0.45, QColor( 24, 177, 178));
			gradient->setColorStopAt(0.50, QColor( 51, 184, 161));
			gradient->setColorStopAt(0.55, QColor( 85, 189, 142));
			gradient->setColorStopAt(0.60, QColor(122, 191, 124));
			gradient->setColorStopAt(0.65, QColor(155, 191, 111));
			gradient->setColorStopAt(0.70, QColor(184, 189,  99));
			gradient->setColorStopAt(0.75, QColor(211, 187,  88));
			gradient->setColorStopAt(0.80, QColor(236, 185,  76));
			gradient->setColorStopAt(0.85, QColor(255, 193,  58));
			gradient->setColorStopAt(0.90, QColor(250, 209,  43));
			gradient->setColorStopAt(0.95, QColor(245, 227,  30));
			gradient->setColorStopAt(1.00, QColor(249, 251,  14));
			break;
	}
}