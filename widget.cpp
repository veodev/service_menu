#include "widget.h"
#include "ui_widget.h"

#include "externalkeyboard.h"
#include "lm75.h"
#include "genericTempSensor.h"
#include "screen.h"
#include "wifimanager.h"

#include <QtEndian>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QCryptographicHash>
#include <QtSerialPort/QSerialPort>
#include <QThread>
#include <QAudioProbe>
#include <QAudioRecorder>
#include <QUrl>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QtBluetooth/QtBluetooth>
#include <unistd.h>

const QString ftpServerAddress = "192.168.10.105";
const int ftpPort = 21;

const QString lcdTestAppPath = "/usr/local/avicon-31/test_avicon/lcdtestfb ";

const QString releasedStyle = "color: black; background-color: white;";
const QString pressedStyle = "color: white; background-color: black;";
const QString sliderStyle = "QSlider::groove:horizontal {border:none; margin-top: 1px; margin-bottom: 1px; height: 10px; } \
                             QSlider::sub-page {background: rgb(164, 192, 2);} \
                             QSlider::add-page {background: white;} \
                             QSlider::handle {background: black; border: 2px solid black; width: 35px; margin: -30px 0;}";

const QString sourcePath = "/tmp/test_file";
const QString destinationPath = "/media/usb0/test_file";

const QString uploadFilePath = "/tmp/test_file";
const QString downloadFilePath = "/tmp/test_file2";

const QString pointerCalPath = "/etc/pointercal";
const QString pointerCalSavePath = "/etc/pointercal_default";

const int minHeatingTime = 1;
const int maxHeatingTime = 10;

static const float BRIGHTNESS_MIN = 46.0f;
static const float BRIGHTNESS_MAX = 128.0f;

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , _externalKeyboard(Q_NULLPTR)
    , _command(QString())
    , _heatingMinutes(minHeatingTime)
    , _audioOutput(new AudioOutput())
    , _tcpSocket(Q_NULLPTR)
    , _udpSocket(Q_NULLPTR)
    , _umuSerialNumber(QString())
    , _umuFirmwareVersion(QString())
    , _umuPlicFirmwareVersion(QString())
    , _wirelessInterface(new WifiManager(this))
    , _networkManager(new QNetworkAccessManager(this))
    , _ftpUrl(QUrl())
    , _isUpload(true)
    , _isFileUploading(false)
    , _isFileDownloading(false)
    , _resultComTest(false)
    , _btManager(Q_NULLPTR)
    , _lastPairDevice(QString())
{
    ui->setupUi(this);

    saveDefaultTsCalibration();
    ui->stackedWidget->setCurrentIndex(0);
    ui->bit5Button->setDisabled(true);
    ui->screenBrightnessSlider->setStyleSheet(sliderStyle);
    ui->volumeSlider->setStyleSheet(sliderStyle);
    setReleasedStyleForAllButtons();
    ui->displaySectionButton->setStyleSheet(pressedStyle);
    _widget = new QWidget();
    _widget->hide();

    ui->screenBrightnessSlider->setValue(50);

    ui->heatingTimeLcdNumber->display(_heatingMinutes);
    ui->stopHeatingTimerButton->setChecked(true);
    ui->stopHeatingTimerButton->setDisabled(true);

    ui->cameraPage->findChild<QPushButton*>("takeAPhotoPushButton")->hide();
    ui->cameraPage->findChild<QPushButton*>("recordVideoPushButton")->hide();

    _externalKeyboard = new ExternalKeyboard(this);
    connect(_externalKeyboard, SIGNAL(toggled(bool, int)), this, SLOT(externalKeyToggled(bool, int)));

    _internalTemperature = new LM75(1, QString("0x4f"));
    _externalTemperature = new GenericTempSensor();
    _updateTimer = new QTimer(this);
    connect(_updateTimer, SIGNAL(timeout()), this, SLOT(updateTemperature()));
    _updateTimer->setInterval(500);
    _updateTimer->start();


    _heatingTimer = new QTimer(this);
    connect(_heatingTimer, SIGNAL(timeout()), this, SLOT(stopHeatingTimer()));

    _checkHeatingTimerTimer = new QTimer(this);
    _checkHeatingTimerTimer->setInterval(100);
    connect(_checkHeatingTimerTimer, SIGNAL(timeout()), this, SLOT(updateTimerLcd()));

    _geoPosition = GeoPosition::instance("/dev/ttymxc1");
    connect(_geoPosition, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
    connect(_geoPosition, SIGNAL(satellitesInUseUpdated(QList<QGeoSatelliteInfo>)), this, SLOT(satellitesInUseUpdated(QList<QGeoSatelliteInfo>)));
    connect(_geoPosition, SIGNAL(antennaStatusChanged(GeoPosition::AntennaStatus)), this, SLOT(antennaStatusChanged(GeoPosition::AntennaStatus)));

    connect(this, SIGNAL(doSetVolume(double)), _audioOutput, SLOT(setVolume(double)));
    ui->volumeSlider->setValue(20);

    connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(onStackedWidgetChanged(int)));

    _checkVoltageTimer = new QTimer(this);
    _checkVoltageTimer->setInterval(3000);
    connect(_checkVoltageTimer, SIGNAL(timeout()), this, SLOT(updateVoltage()));

    umuPower(false);

    _audioRecorder = new QAudioRecorder(this);
    connect(_audioRecorder, SIGNAL(durationChanged(qint64)), this, SLOT(updateRecordProgress(qint64)));
    connect(_audioRecorder, SIGNAL(stateChanged(QMediaRecorder::State)), this, SLOT(updateState(QMediaRecorder::State)));

    _audioPlayer = new QMediaPlayer(this);
    connect(_audioPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChanged(QMediaPlayer::State)));

    _serialPort = new QSerialPort(this);
    connect(_serialPort, SIGNAL(readyRead()), this, SLOT(readData()));

    _bluetoothTimer = new QTimer(this);
    _bluetoothTimer->setInterval(1500);

    connect(_bluetoothTimer, SIGNAL(timeout()), &_bluetooth, SLOT(checkStatus()));

    _bluetooth.init();
    connect(&_bluetooth, SIGNAL(bluetoothStatusChanged(bool)), this, SLOT(onBluetoothStatusChanged(bool)));
    connect(&_bluetooth, SIGNAL(dbusStatusChanged(bool)), this, SLOT(onDbusStatusChanged(bool)));
    connect(&_bluetooth, SIGNAL(bluetoothdStatusChanged(bool)), this, SLOT(onBluetoothBluetoothdStatusChanged(bool)));
    connect(&_bluetooth, SIGNAL(hciDeviceStatusChanged(bool)), this, SLOT(onBluetoothHCIDeviceStatusChanged(bool)));
    onBluetoothStatusChanged(_bluetooth.getBluetoothStatus());
    _bluetoothTimer->start();

    _ftpUrl.setScheme("ftp");
    _ftpUrl.setHost(ftpServerAddress);
    _ftpUrl.setPath("upload/test_file");
    _ftpUrl.setPort(ftpPort);

    _checkComTimer = new QTimer(this);
    _checkComTimer->setInterval(3000);
    connect(_checkComTimer, SIGNAL(timeout()), this, SLOT(onCheckComTimerTimeout()));

    _btManager = new Avicon31Bluetooth();
    connect(_btManager, SIGNAL(scanListChanged(QStringList)), this, SLOT(onBtScanListChanged(QStringList)));
}

Widget::~Widget()
{
    delete ui;
    closeSerialPort();
}

void Widget::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if (_widget->isHidden() == false) {
        _widget->hide();
        repaint();
        QProcess process;
        process.startDetached("killall lcdtestfb");
    }
}

void Widget::saveDefaultTsCalibration()
{
    if (QFile(pointerCalSavePath).exists() == false) {
        bool res = QFile::copy(pointerCalPath, pointerCalSavePath);
        sync();
        qDebug() << "Save pointercal: " << res;
    }
}

void Widget::restoreDefaultTsCalibration()
{
    ui->defaultCalibrationButton->setEnabled(false);
    QFile(pointerCalPath).remove();
    sync();
    bool res = QFile::copy(pointerCalSavePath, pointerCalPath);
    sync();
    qDebug() << "Restore pointercal: " << res;
    ui->defaultCalibrationButton->setEnabled(true);
}

void Widget::setReleasedStyleForAllButtons()
{
    ui->displaySectionButton->setStyleSheet(releasedStyle);
    ui->touchScreenSectionButton->setStyleSheet(releasedStyle);
    ui->temperatureSectionButton->setStyleSheet(releasedStyle);
    ui->gpsSectionButton->setStyleSheet(releasedStyle);
    ui->soundSystemSectionButton->setStyleSheet(releasedStyle);
    ui->cameraSectionButton->setStyleSheet(releasedStyle);
    ui->externalKeyboardSectionButton->setStyleSheet(releasedStyle);
    ui->wifiSectionButton->setStyleSheet(releasedStyle);
    ui->bluetoothSectionButton->setStyleSheet(releasedStyle);
    ui->usbSectionButton->setStyleSheet(releasedStyle);
    ui->comSectionButton->setStyleSheet(releasedStyle);
    ui->lanSectionButton->setStyleSheet(releasedStyle);
    ui->powerSectionButton->setStyleSheet(releasedStyle);
    ui->exitSectionButton->setStyleSheet(releasedStyle);
}

bool Widget::isFlashMount()
{
    QProcess process;
    process.start("sh");
    process.waitForStarted();
    process.write("df | grep sda | tr -s \" \" \" \" | cut -d \" \" -f1");
    process.closeWriteChannel();
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput();
    process.close();
    if (output.isEmpty()) {
        return false;
    }
    else {
        return true;
    }
}

bool Widget::copyFileToFlash()
{
    generateFile(10000000);
    QCryptographicHash sourceHash(QCryptographicHash::Md5);
    QFile hashFile(sourcePath);
    sourceHash.addData(&hashFile);
    _hashWriteFile = sourceHash.result();
    removeFileIfExist(destinationPath);
    bool res = QFile::copy(sourcePath, destinationPath);
    sync();
    return res;
}

bool Widget::readFileFromFlash()
{
    removeFileIfExist(sourcePath);
    QFile::copy(destinationPath, sourcePath);
    sync();
    QCryptographicHash destinationHash(QCryptographicHash::Md5);
    QFile destinationFile(destinationPath);
    destinationHash.addData(&destinationFile);
    _hashReadFile = destinationHash.result();
    if (_hashWriteFile == _hashReadFile) {
        return true;
    }
    else {
        return false;
    }
}

void Widget::umuPower(bool isOn)
{
    GpioOutput umuPower(3);
    isOn ? umuPower.setValue(1) : umuPower.setValue(0);
}

void Widget::blockAudioChannelsButtons(bool isBlock)
{
    ui->test500HzFlashLeftButton->setDisabled(isBlock);
    ui->test500HzLeftButton->setDisabled(isBlock);
    ui->test1000HzLeftButton->setDisabled(isBlock);
    ui->test2000HzLeftButton->setDisabled(isBlock);
    ui->test2000HzFlashLeftButton->setDisabled(isBlock);
    ui->test500HzFlashRightButton->setDisabled(isBlock);
    ui->test500HzRightButton->setDisabled(isBlock);
    ui->test1000HzRightButton->setDisabled(isBlock);
    ui->test2000HzRightButton->setDisabled(isBlock);
    ui->test2000HzFlashRightButton->setDisabled(isBlock);
}

void Widget::setAudioChannelsToggledFalseButtons()
{
    ui->test500HzFlashLeftButton->setChecked(false);
    ui->test500HzLeftButton->setChecked(false);
    ui->test1000HzLeftButton->setChecked(false);
    ui->test2000HzLeftButton->setChecked(false);
    ui->test2000HzFlashLeftButton->setChecked(false);
    ui->test500HzFlashRightButton->setChecked(false);
    ui->test500HzRightButton->setChecked(false);
    ui->test1000HzRightButton->setChecked(false);
    ui->test2000HzRightButton->setChecked(false);
    ui->test2000HzFlashRightButton->setChecked(false);
}

bool Widget::openSerialPort()
{
    _serialPort->setPortName("/dev/ttymxc1");
    _serialPort->setBaudRate(QSerialPort::Baud9600);
    _serialPort->setDataBits(QSerialPort::Data8);
    _serialPort->setStopBits(QSerialPort::OneStop);
    _serialPort->setFlowControl(QSerialPort::NoFlowControl);
    _serialPort->setParity(QSerialPort::NoParity);
    _serialPort->open(QIODevice::ReadWrite);
    return _serialPort->isOpen();
}

void Widget::closeSerialPort()
{
    if (_serialPort->isOpen()) {
        _serialPort->close();
    }
}

quint64 Widget::testWriteToSerialPort()
{
    QByteArray array("test");
    return _serialPort->write(array);
}

void Widget::clearUmuData()
{
    _umuSerialNumber = QString();
    _umuFirmwareVersion = QString();
    _umuPlicFirmwareVersion = QString();
}

void Widget::clearDisplayData()
{
    ui->umuSerialNumberLineEdit->clear();
    ui->umuFirmwareVersionLineEdit->clear();
    ui->umuPlicFirmwareVersionLineEdit->clear();
}

void Widget::displayUmuData()
{
    ui->umuSerialNumberLineEdit->setText(_umuSerialNumber);
    ui->umuFirmwareVersionLineEdit->setText(_umuFirmwareVersion);
    ui->umuPlicFirmwareVersionLineEdit->setText(_umuPlicFirmwareVersion);
}

void Widget::on_displaySectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->lcdScreenPage);
    setReleasedStyleForAllButtons();
    ui->displaySectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_touchScreenSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->touchscreenPage);
    setReleasedStyleForAllButtons();
    ui->touchScreenSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_defaultCalibrationButton_released()
{
    restoreDefaultTsCalibration();
}

void Widget::on_temperatureSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->temperaturePage);
    setReleasedStyleForAllButtons();
    ui->temperatureSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_gpsSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->gpsPage);
    setReleasedStyleForAllButtons();
    ui->gpsSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_soundSystemSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->soundSystemPage);
    setReleasedStyleForAllButtons();
    ui->soundSystemSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_cameraSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->cameraPage);
    setReleasedStyleForAllButtons();
    ui->cameraSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_externalKeyboardSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->externalKeyboardPage);
    setReleasedStyleForAllButtons();
    ui->externalKeyboardSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_wifiSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->wifiPage);
    setReleasedStyleForAllButtons();
    ui->wifiSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_bluetoothSectionButton_released()
{
    //    ui->stackedWidget->setCurrentWidget(ui->bluetoothPage);
    ui->stackedWidget->setCurrentWidget(ui->btPage);
    setReleasedStyleForAllButtons();
    ui->bluetoothSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_usbSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->usbPage);
    setReleasedStyleForAllButtons();
    ui->usbSectionButton->setStyleSheet(pressedStyle);
    ui->usbTestListWidget->clear();
}

void Widget::on_comSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->comPage);
    setReleasedStyleForAllButtons();
    ui->comSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_lanSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->lanPage);
    setReleasedStyleForAllButtons();
    ui->lanSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_powerSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->powerPage);
    setReleasedStyleForAllButtons();
    ui->powerSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_exitSectionButton_released()
{
    ui->stackedWidget->setCurrentWidget(ui->exitPage);
    setReleasedStyleForAllButtons();
    ui->exitSectionButton->setStyleSheet(pressedStyle);
}

void Widget::on_touchScreenTestButton_released()
{
    QProcess process;
    process.startDetached("/usr/local/avicon-31/test_avicon/ts_test.sh");
    qApp->quit();
}

void Widget::on_touchScreenCalibrationButton_released()
{
    QProcess process;
    process.startDetached("/usr/local/avicon-31/test_avicon/ts_calibration.sh");
    qApp->quit();
}

void Widget::externalKeyToggled(bool isPressed, int keyCode)
{
    switch (keyCode) {
    case 0:
        isPressed ? ui->trackMarkLabel->setStyleSheet(pressedStyle) : ui->trackMarkLabel->setStyleSheet(releasedStyle);
        isPressed ? ui->trackMarkLabel->setText(tr("Pressed")) : ui->trackMarkLabel->setText(tr("Released"));
        break;
    case 1:
        isPressed ? ui->serviceMarkLabel->setStyleSheet(pressedStyle) : ui->serviceMarkLabel->setStyleSheet(releasedStyle);
        isPressed ? ui->serviceMarkLabel->setText(tr("Pressed")) : ui->serviceMarkLabel->setText(tr("Released"));
        break;
    case 2:
        isPressed ? ui->boltJointLabel->setStyleSheet(pressedStyle) : ui->boltJointLabel->setStyleSheet(releasedStyle);
        isPressed ? ui->boltJointLabel->setText(tr("Pressed")) : ui->boltJointLabel->setText(tr("Released"));
        break;
    }
}

void Widget::on_usbTestButton_released()
{
    bool resultMountFlash = false;
    bool resultOfCopyToFlash = false;
    bool resultOfReadFromFlash = false;
    ui->usbTestListWidget->clear();
    ui->usbTestListWidget->addItem(tr("START"));
    qApp->processEvents();
    qApp->processEvents();
    QThread::sleep(1);

    resultMountFlash = isFlashMount();
    if (resultMountFlash) {
        resultOfCopyToFlash = copyFileToFlash();
        resultOfReadFromFlash = readFileFromFlash();
    }

    resultMountFlash ? ui->usbTestListWidget->addItem(QString(tr("Check connection ... OK"))) : ui->usbTestListWidget->addItem(QString(tr("Check connection ... FAIL")));

    resultOfCopyToFlash ? ui->usbTestListWidget->addItem(QString(tr("Check write operation ... OK"))) : ui->usbTestListWidget->addItem(QString(tr("Check write operation ... FAIL")));

    resultOfReadFromFlash ? ui->usbTestListWidget->addItem(QString(tr("Check read operation ... OK"))) : ui->usbTestListWidget->addItem(QString(tr("Check read operation ... FAIL")));

    ui->usbTestListWidget->addItems(QStringList() << QString() << QString());

    if (resultMountFlash == true && resultOfCopyToFlash == true && resultOfReadFromFlash == true) {
        ui->usbTestListWidget->addItem(QString(tr("Test passed ... OK")));
    }
    else {
        ui->usbTestListWidget->addItem(QString(tr("Test passed ... FAIL")));
    }
    ui->usbTestListWidget->addItem(tr("FINISH"));
}

void Widget::updateTemperature()
{
    ui->internalLcdNumber->display(QString::number(_internalTemperature->getTemperature(), 'f', 2));
    ui->externalLcdNumber->display(QString::number(_externalTemperature->getTemperature(), 'f', 2));
}

void Widget::updateVoltage()
{
    _powerManager.update();
    ui->voltageLcdNumber->display(QString::number(_powerManager.batteryVoltage(), 'f', 2));
    ui->voltage3vLcdNumber->display(QString::number(_powerManager.voltage3v3(), 'f', 2));
    ui->voltage5vLcdNumber->display(QString::number(_powerManager.voltage5v(), 'f', 2));
    ui->voltage12vLcdNumber->display(QString::number(_powerManager.voltage12v(), 'f', 2));
    ui->cpuTemperatureLcdNumber->display(QString::number(_powerManager.temperatureSoc(), 'f', 2));
}

void Widget::on_leftBrightnessButton_released()
{
    ui->screenBrightnessSlider->setValue(ui->screenBrightnessSlider->value() - 1);
}

void Widget::on_rightBrightnessButton_released()
{
    ui->screenBrightnessSlider->setValue(ui->screenBrightnessSlider->value() + 1);
}

void Widget::on_screenBrightnessSlider_valueChanged(int value)
{
    setScreenBrightness(value);
    ui->brightnessLcdNumber->display(value);
}

void Widget::on_redButton_toggled(bool checked)
{
    if (checked) {
        ui->greenButton->setChecked(false);
        ui->blueButton->setChecked(false);
        ui->bit5Button->setDisabled(true);
        ui->bit5Button->setChecked(false);
    }
}

void Widget::on_greenButton_toggled(bool checked)
{
    if (checked) {
        ui->redButton->setChecked(false);
        ui->blueButton->setChecked(false);
        ui->bit5Button->setEnabled(true);
    }
}

void Widget::on_blueButton_toggled(bool checked)
{
    if (checked) {
        ui->greenButton->setChecked(false);
        ui->redButton->setChecked(false);
        ui->bit5Button->setDisabled(true);
        ui->bit5Button->setChecked(false);
    }
}

void Widget::on_bit0Button_toggled(bool checked)
{
    if (checked) {
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_bit1Button_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_bit2Button_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_bit3Button_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_bit4Button_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_bit5Button_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_allBitsButton_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_gradationToRightButton_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_gradationToLeftButton_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_gradationToUpButton_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToDownButton->setChecked(false);
    }
}

void Widget::on_gradationToDownButton_toggled(bool checked)
{
    if (checked) {
        ui->bit0Button->setChecked(false);
        ui->bit1Button->setChecked(false);
        ui->bit2Button->setChecked(false);
        ui->bit3Button->setChecked(false);
        ui->bit4Button->setChecked(false);
        ui->bit5Button->setChecked(false);
        ui->allBitsButton->setChecked(false);
        ui->gradationToLeftButton->setChecked(false);
        ui->gradationToRightButton->setChecked(false);
        ui->gradationToUpButton->setChecked(false);
    }
}

void Widget::on_testRgbLcdButton_released()
{
    _command = QString();
    if (ui->redButton->isChecked()) {
        if (ui->bit0Button->isChecked()) {
            _command += QString("--fillscreen 1 0 0");
        }
        else if (ui->bit1Button->isChecked()) {
            _command += QString("--fillscreen 2 0 0");
        }
        else if (ui->bit2Button->isChecked()) {
            _command += QString("--fillscreen 4 0 0");
        }
        else if (ui->bit3Button->isChecked()) {
            _command += QString("--fillscreen 8 0 0");
        }
        else if (ui->bit4Button->isChecked()) {
            _command += QString("--fillscreen 16 0 0");
        }
        else if (ui->allBitsButton->isChecked()) {
            _command += QString("--fillscreen 63 0 0");
        }
        else if (ui->gradationToRightButton->isChecked()) {
            _command += QString("--gradation red right");
        }
        else if (ui->gradationToLeftButton->isChecked()) {
            _command += QString("--gradation red left");
        }
        else if (ui->gradationToUpButton->isChecked()) {
            _command += QString("--gradation red up");
        }
        else if (ui->gradationToDownButton->isChecked()) {
            _command += QString("--gradation red down");
        }
    }
    else if (ui->greenButton->isChecked()) {
        if (ui->bit0Button->isChecked()) {
            _command += QString("--fillscreen 0 1 0");
        }
        else if (ui->bit1Button->isChecked()) {
            _command += QString("--fillscreen 0 2 0");
        }
        else if (ui->bit2Button->isChecked()) {
            _command += QString("--fillscreen 0 4 0");
        }
        else if (ui->bit3Button->isChecked()) {
            _command += QString("--fillscreen 0 8 0");
        }
        else if (ui->bit4Button->isChecked()) {
            _command += QString("--fillscreen 0 16 0");
        }
        else if (ui->bit5Button->isChecked()) {
            _command += QString("--fillscreen 0 32 0");
        }
        else if (ui->allBitsButton->isChecked()) {
            _command += QString("--fillscreen 0 63 0");
        }
        else if (ui->gradationToRightButton->isChecked()) {
            _command += QString("--gradation green right");
        }
        else if (ui->gradationToLeftButton->isChecked()) {
            _command += QString("--gradation green left");
        }
        else if (ui->gradationToUpButton->isChecked()) {
            _command += QString("--gradation green up");
        }
        else if (ui->gradationToDownButton->isChecked()) {
            _command += QString("--gradation green down");
        }
    }
    else if (ui->blueButton->isChecked()) {
        if (ui->bit0Button->isChecked()) {
            _command += QString("--fillscreen 0 0 1");
        }
        else if (ui->bit1Button->isChecked()) {
            _command += QString("--fillscreen 0 0 2");
        }
        else if (ui->bit2Button->isChecked()) {
            _command += QString("--fillscreen 0 0 4");
        }
        else if (ui->bit3Button->isChecked()) {
            _command += QString("--fillscreen 0 0 8");
        }
        else if (ui->bit4Button->isChecked()) {
            _command += QString("--fillscreen 0 0 16");
        }
        else if (ui->allBitsButton->isChecked()) {
            _command += QString("--fillscreen 0 0 63");
        }
        else if (ui->gradationToRightButton->isChecked()) {
            _command += QString("--gradation blue right");
        }
        else if (ui->gradationToLeftButton->isChecked()) {
            _command += QString("--gradation blue left");
        }
        else if (ui->gradationToUpButton->isChecked()) {
            _command += QString("--gradation blue up");
        }
        else if (ui->gradationToDownButton->isChecked()) {
            _command += QString("--gradation blue down");
        }
    }

    if (_command.isEmpty()) {
        return;
    }

    _widget->setParent(this);
    _widget->showFullScreen();
    QTimer::singleShot(500, this, SLOT(execCommand()));
}

void Widget::execCommand()
{
    QProcess process;
    process.startDetached(lcdTestAppPath + _command);
}

void Widget::on_plusHeatingTimeButton_released()
{
    if (_heatingMinutes < maxHeatingTime) {
        ++_heatingMinutes;
        ui->heatingTimeLcdNumber->display(_heatingMinutes);
    }
}

void Widget::on_minusHeatingTimeButton_released()
{
    if (_heatingMinutes > minHeatingTime) {
        --_heatingMinutes;
        ui->heatingTimeLcdNumber->display(_heatingMinutes);
    }
}

void Widget::on_stopHeatingTimerButton_released()
{
    _heatingTimer->stop();
    _checkHeatingTimerTimer->stop();
    ui->startHeatingTimerButton->setChecked(false);
    ui->startHeatingTimerButton->setDisabled(false);
    ui->stopHeatingTimerButton->setChecked(true);
    ui->stopHeatingTimerButton->setDisabled(true);

    ui->minutesHeatingTimerLcd->display(0);
    ui->secondsHeatingTimerLcd->display(0);

    ui->heatingInfoLabel->clear();

    GpioOutput heaterPin(0);
    heaterPin.setValue(0);
}

void Widget::on_startHeatingTimerButton_toggled(bool checked)
{
    if (checked) {
        _heatingTimer->setInterval(_heatingMinutes * 60000);
        _heatingTimer->start();
        _checkHeatingTimerTimer->start();
        ui->startHeatingTimerButton->setChecked(true);
        ui->startHeatingTimerButton->setDisabled(true);
        ui->stopHeatingTimerButton->setChecked(false);
        ui->stopHeatingTimerButton->setDisabled(false);

        ui->heatingInfoLabel->setText(tr("Heating ..."));

        GpioOutput heaterPin(0);
        heaterPin.setValue(1);
    }
}

void Widget::stopHeatingTimer()
{
    on_stopHeatingTimerButton_released();
}

void Widget::updateTimerLcd()
{
    int currentSeconds = _heatingTimer->remainingTime() / 1000;
    ui->minutesHeatingTimerLcd->display(currentSeconds / 60);
    ui->secondsHeatingTimerLcd->display(currentSeconds - ((currentSeconds / 60) * 60));
}

void Widget::positionUpdated(const QGeoPositionInfo& info)
{
    QGeoCoordinate coordinate = info.coordinate();
    if (coordinate.type() == QGeoCoordinate::Coordinate2D) {
        ui->coordinatesLineEdit->setText(info.coordinate().toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere));
    }

    if (info.timestamp().isValid()) {
        ui->timeStampLineEdit->setText(info.timestamp().toString(Qt::LocaleDate));
    }
}

void Widget::satellitesInUseUpdated(const QList<QGeoSatelliteInfo>& satellites)
{
    if ((satellites.count() > 0) && (satellites.at(0).satelliteSystem() == QGeoSatelliteInfo::GPS)) {
        ui->satellitesLineEdit->setText(QString::number(satellites.count()));
    }
}

void Widget::antennaStatusChanged(GeoPosition::AntennaStatus antennaStatus)
{
    QString status;

    switch (antennaStatus) {
    case GeoPosition::AntennaStatusUnknown:
        status = tr("Unknown status");
        break;
    case GeoPosition::AntennaStatusConnected:
        status = tr("Connected");
        break;
    case GeoPosition::AntennaStatusDisconnected:
        status = tr("Disconnected");
        break;
    default:
        status = tr("Unknown status");
        break;
    }
    ui->antennaStatusLineEdit->setText(status);
}

void Widget::on_test500HzFlashLeftButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::left500HzBlink);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::left500HzBlink);
        _audioOutput->play();
    }
}

void Widget::on_test500HzFlashRightButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::right500HzBlink);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::right500HzBlink);
        _audioOutput->play();
    }
}

void Widget::on_test500HzLeftButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::left500Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::left500Hz);
        _audioOutput->play();
    }
}

void Widget::on_test500HzRightButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::right500Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::right500Hz);
        _audioOutput->play();
    }
}

void Widget::on_test1000HzLeftButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::left1000Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::left1000Hz);
        _audioOutput->play();
    }
}

void Widget::on_test1000HzRightButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::right1000Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::right1000Hz);
        _audioOutput->play();
    }
}

void Widget::on_test2000HzLeftButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::left2000Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::left2000Hz);
        _audioOutput->play();
    }
}

void Widget::on_test2000HzRightButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::right2000Hz);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::right2000Hz);
        _audioOutput->play();
    }
}

void Widget::on_test2000HzFlashLeftButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::left2000HzBlink);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::left2000HzBlink);
        _audioOutput->play();
    }
}

void Widget::on_test2000HzFlashRightButton_toggled(bool checked)
{
    if (checked) {
        _audioOutput->setTone(AudioGenerator::right2000HzBlink);
        _audioOutput->play();
    }
    else {
        _audioOutput->clearTone(AudioGenerator::right2000HzBlink);
        _audioOutput->play();
    }
}

void Widget::on_volumeSlider_valueChanged(int value)
{
    emit doSetVolume(value);
    ui->volumeLcdNumber->display(value);
}

void Widget::onStackedWidgetChanged(int index)
{
    if (ui->stackedWidget->widget(index)->objectName() != QString("soundSystemPage")) {
        setAudioChannelsToggledFalseButtons();
    }
    else {
        setWireOutput();
    }

    if (ui->stackedWidget->widget(index)->objectName() == QString("powerPage")) {
        updateVoltage();
        _checkVoltageTimer->start();
    }
    else {
        _checkVoltageTimer->stop();
    }

    if (ui->stackedWidget->widget(index)->objectName() == QString("lanPage")) {
        createLanConnection();
        _audioOutput->stop();
    }

    if (ui->stackedWidget->widget(index)->objectName() == QString("bluetoothPage")) {
        _bluetoothTimer->start();
    }
    else {
        _bluetoothTimer->stop();
    }

    if (ui->stackedWidget->widget(index)->objectName() == QString("temperaturePage")) {
        _updateTimer->start();
    }
    else {
        _updateTimer->stop();
    }

    if (ui->stackedWidget->widget(index)->objectName() == QString("wifiPage")) {
        clearWifiPageWidgets();
    }
}

void Widget::on_umuPowerButton_toggled(bool checked)
{
    if (checked) {
        ui->umuPowerLabel->setPixmap(QPixmap(":/resources/umuOn.png"));
        ui->umuPowerMessage->setText(tr("ON"));
    }
    else {
        ui->umuPowerLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
        ui->umuPowerMessage->setText(tr("OFF"));
    }
    umuPower(checked);
}

void Widget::updateRecordProgress(qint64 duration)
{
    if (duration >= 31000) {  // 30 seconds
        _audioRecorder->stop();
        on_stopRecordButton_toggled(true);
    }
    int currentSeconds = duration / 1000;
    ui->secondsRecordingTimerLcd->display(currentSeconds - ((currentSeconds / 60) * 60));
}

void Widget::stateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        _audioOutput->stop();
        blockAudioChannelsButtons(true);
        ui->startRecordButton->setDisabled(true);
        break;
    case QMediaPlayer::StoppedState:
        _audioOutput->resume();
        blockAudioChannelsButtons(false);
        ui->startRecordButton->setDisabled(false);
        break;
    default:
        break;
    }
}

void Widget::updateState(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        _audioOutput->stop();
        ui->startRecordButton->setChecked(true);
        ui->startRecordButton->setDisabled(true);
        ui->stopRecordButton->setChecked(false);
        ui->stopRecordButton->setDisabled(false);
        blockAudioChannelsButtons(true);
        break;
    case QMediaRecorder::StoppedState:
        _audioOutput->resume();
        ui->startRecordButton->setChecked(false);
        ui->startRecordButton->setDisabled(false);
        ui->stopRecordButton->setChecked(true);
        ui->stopRecordButton->setDisabled(true);
        blockAudioChannelsButtons(false);
        break;
    case QMediaRecorder::PausedState:
        break;
    }
}

void Widget::on_startRecordButton_toggled(bool checked)
{
    if (checked) {
        setAudioChannelsToggledFalseButtons();
        _audioRecorder->setAudioInput("default:");
        QAudioEncoderSettings settings;
        settings.setCodec("audio/PCM");
        settings.setSampleRate(32000);
        settings.setBitRate(32000);
        settings.setChannelCount(1);
        settings.setQuality(QMultimedia::NormalQuality);
        settings.setEncodingMode(QMultimedia::ConstantQualityEncoding);
        _audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), "wav");
        _audioRecorder->setOutputLocation(QString("/usr/local/avicon-31/test_avicon/testMicRecord.wav"));
        _audioRecorder->record();
    }
}

void Widget::on_stopRecordButton_toggled(bool checked)
{
    if (checked) {
        _audioRecorder->stop();
    }
}

void Widget::on_startPlayButton_toggled(bool checked)
{
    if (checked) {
        //        _audioPlayer->setMedia(QUrl::fromLocalFile(QString("/usr/local/avicon-31/test_avicon/testMicRecord.wav")));
        //        _audioPlayer->play();
        _audioOutput->playSound(QString("/usr/local/avicon-31/test_avicon/testMicRecord.wav"));
    }
}

void Widget::on_startPlayButton_released()
{
    setAudioChannelsToggledFalseButtons();
    _audioOutput->playSound(QString("/usr/local/avicon-31/test_avicon/testMicRecord.wav"));
}

void Widget::readData()
{
    _checkComTimer->stop();
    QByteArray test("test");
    QByteArray data = _serialPort->readAll();
    _resultComTest = data == test;
    if (_resultComTest == true) {
        ui->comTestListWidget->addItem(QString(tr("Check read operation ... OK")));
        ui->comTestListWidget->addItems(QStringList() << QString() << QString());
        ui->comTestListWidget->addItem(QString(tr("Test passed ... OK")));
    }
    else {
        ui->comTestListWidget->addItem(QString(tr("Check read operation ... FAIL")));
        ui->comTestListWidget->addItems(QStringList() << QString() << QString());
        ui->comTestListWidget->addItem(QString(tr("Test passed ... FAIL")));
    }
    ui->comTestListWidget->addItem(tr("FINISH"));
}

void Widget::on_testComButton_released()
{
    ui->comTestListWidget->clear();
    _resultComTest = false;
    _checkComTimer->start();
    ui->comTestListWidget->addItem(tr("START"));
    qApp->processEvents();
    qApp->processEvents();
    QThread::sleep(1);
    openSerialPort() ? ui->comTestListWidget->addItem(QString(tr("Check connection ... OK"))) : ui->comTestListWidget->addItem(QString(tr("Check connection ... FAIL")));
    testWriteToSerialPort() ? ui->comTestListWidget->addItem(QString(tr("Check write operation ... OK"))) : ui->comTestListWidget->addItem(QString(tr("Check write operation ... FAIL")));
}

void Widget::onCheckComTimerTimeout()
{
    _checkComTimer->stop();
    ui->comTestListWidget->addItem(QString(tr("Check read operation ... FAIL")));
    ui->comTestListWidget->addItems(QStringList() << QString() << QString());
    ui->comTestListWidget->addItem(QString(tr("Test passed ... FAIL")));
    ui->comTestListWidget->addItem(tr("FINISH"));
}

void Widget::onTcpStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::ClosingState:
    case QAbstractSocket::UnconnectedState:
        ui->connectionMessageLabel->setText(tr("Connection disconnected"));
        ui->connectionStatusLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
        clearUmuData();
        break;
    case QAbstractSocket::HostLookupState:
    case QAbstractSocket::ConnectingState:
        ui->connectionMessageLabel->setText(tr("Establishing a connection ..."));
        ui->connectionStatusLabel->setPixmap(QPixmap(":/resources/connectingStatus.png"));
        clearUmuData();
        break;
    case QAbstractSocket::ConnectedState:
        ui->connectionMessageLabel->setText(tr("Connection established"));
        ui->connectionStatusLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
        on_testLanButton_released();
        break;
    default:
        break;
    }
}

void Widget::readDatagrams()
{
    clearUmuData();
    QByteArray data;

    quint8 firstNumUmuFirmVer = 0;
    quint8 secondNumUmuFirmVer = 0;
    quint8 thirdNumUmuFirmVer = 0;

    quint8 firstNumPlicFirmVer = 0;
    quint8 secondNumPlicFirmVer = 0;
    quint8 thirdNumPlicFirmVer = 0;
    quint8 fourthNumPlicFirmVer = 0;

    while (_udpSocket->hasPendingDatagrams()) {
        data.resize(_udpSocket->pendingDatagramSize());
        _udpSocket->readDatagram(data.data(), data.size());
    }
    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == char(0xDF)) {
            thirdNumUmuFirmVer = data[i + 6];
            secondNumUmuFirmVer = data[i + 7];
            firstNumUmuFirmVer = data[i + 8];

            fourthNumPlicFirmVer = data[i + 9];
            thirdNumPlicFirmVer = data[i + 10];
            secondNumPlicFirmVer = data[i + 11];
            firstNumPlicFirmVer = data[i + 12];
        }
        if (data[i] == char(0xDB)) {
            _umuSerialNumber = QString::number(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(data.mid(i + 6, 2).data())));
        }
    }

    _umuFirmwareVersion = QString::number(firstNumUmuFirmVer) + QString(".") + QString::number(secondNumUmuFirmVer) + QString(".") + QString::number(thirdNumUmuFirmVer);
    _umuPlicFirmwareVersion = QString::number(firstNumPlicFirmVer) + QString(".") + QString::number(secondNumPlicFirmVer) + QString(".") + QString::number(thirdNumPlicFirmVer) + QString(".") + QString::number(fourthNumPlicFirmVer);
    displayUmuData();
}

void Widget::createLanConnection()
{
    if (_tcpSocket != 0) {
        disconnect(_tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onTcpStateChanged(QAbstractSocket::SocketState)));
        _tcpSocket->close();
        delete _tcpSocket;
        _tcpSocket = 0;
    }
    if (_udpSocket != 0) {
        disconnect(_udpSocket, SIGNAL(readyRead()), this, SLOT(readDatagrams()));
        _udpSocket->close();
        delete _udpSocket;
        _udpSocket = 0;
    }
    _tcpSocket = new QTcpSocket(this);
    connect(_tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onTcpStateChanged(QAbstractSocket::SocketState)));
    _tcpSocket->connectToHost("192.168.100.100", 43000);

    _udpSocket = new QUdpSocket(this);
    _udpSocket->bind(43001);
    connect(_udpSocket, SIGNAL(readyRead()), this, SLOT(readDatagrams()));
}

void Widget::reboot()
{
    QProcess process;
    process.startDetached("reboot");
    qApp->quit();
}

void Widget::poweroff()
{
    QProcess process;
    process.startDetached("poweroff");
    qApp->quit();
}

bool Widget::saveToDisk(const QString& filename, QIODevice* data)
{
    QFile checkExistFile(filename);
    if (checkExistFile.exists()) {
        checkExistFile.remove();
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (data->size() != 0) {
        qDebug() << "Write: " << file.write(data->readAll());
        file.close();
        return true;
    }
    else {
        return false;
    }
}

bool Widget::compareHash()
{
    QCryptographicHash uploadHashObj(QCryptographicHash::Md5);
    QFile uploadFile(uploadFilePath);
    uploadHashObj.addData(&uploadFile);
    _hashUploadFile = uploadHashObj.result();
    qDebug() << _hashUploadFile.toHex();

    QCryptographicHash downloadHashObj(QCryptographicHash::Md5);
    QFile downloadFile(downloadFilePath);
    downloadHashObj.addData(&downloadFile);
    _hashDownloadFile = downloadHashObj.result();
    qDebug() << _hashDownloadFile.toHex();

    if (_hashUploadFile == _hashDownloadFile) {
        return true;
    }
    else {
        return false;
    }
}

void Widget::generateFile(int bytes)
{
    removeFileIfExist(sourcePath);
    QFile file(sourcePath);
    file.open(QIODevice::Truncate | QIODevice::Text | QIODevice::WriteOnly);
    file.resize(bytes);
    file.close();
}

void Widget::clearWifiPageWidgets()
{
    ui->accessPointsListWidget->clear();
    ui->interfacesListWidget->clear();
    ui->wifiTestListWidget->clear();
}

void Widget::printTestResult(const QString& str, bool isPassed)
{
    QString resultTest;
    isPassed ? resultTest = tr("OK") : resultTest = tr("FAIL");

    ui->wifiTestListWidget->addItem(str + QString(" ... ") + resultTest);

    if (isPassed == false) {
        ui->wifiTestListWidget->addItems(QStringList() << QString() << QString());
        ui->wifiTestListWidget->addItem(tr("Test passed ... FAIL"));
        ui->wifiTestListWidget->addItem(tr("FINISH"));
    }
}

void Widget::removeFileIfExist(const QString& path)
{
    QFile checkExistFile(path);
    if (checkExistFile.exists()) {
        checkExistFile.remove();
    }
}

void Widget::removeRunFile()
{
    QFile file("/etc/run");
    file.remove();
    file.close();
}

void Widget::setEnabledBtPageButtons(bool isEnabled)
{
    ui->btResetButton->setEnabled(isEnabled);
    ui->btScanButton->setEnabled(isEnabled);
    ui->btPairButton->setEnabled(isEnabled);
    ui->btTestButton->setEnabled(isEnabled);
}

void Widget::setBtWireOutput()
{
    _audioOutput->stop();
    qputenv("ALSA_DEVICE", "both_softvol");
    qDebug() << "ALSA_DEVICE = " << qgetenv("ALSA_DEVICE");
    //    _audioOutput->resume();
}

void Widget::setWireOutput()
{
    _audioOutput->stop();
    qputenv("ALSA_DEVICE", "");
    qDebug() << "ALSA_DEVICE = " << qgetenv("ALSA_DEVICE");
    _audioOutput->resume();
}

void Widget::setBtOutput()
{
    _audioOutput->stop();
    qputenv("ALSA_DEVICE", "bt");
    qDebug() << "ALSA_DEVICE = " << qgetenv("ALSA_DEVICE");
    //    _audioOutput->resume();
}

void Widget::on_testLanButton_released()
{
    clearDisplayData();
    if (_tcpSocket != 0) {
        qApp->processEvents();
        qApp->processEvents();
        QThread::sleep(1);
        if (_tcpSocket->state() == QAbstractSocket::ConnectedState) {
            QByteArray array;
            array[0] = 0x43;
            array[1] = 0x02;
            array[2] = 0x02;
            array[3] = 0x00;
            array[4] = 0x00;
            array[5] = 0x00;
            array[6] = 0xDE;
            array[7] = 0x00;
            qDebug() << "Write: " << _tcpSocket->write(array);
        }
    }
}

void Widget::on_reconnectButton_released()
{
    _tcpSocket->close();
    _udpSocket->close();
    _tcpSocket->connectToHost("192.168.100.100", 43000);
    _udpSocket->bind(43001);
}

void Widget::on_rebootButton_released()
{
    reboot();
}

void Widget::on_quitButton_released()
{
    poweroff();
}

void Widget::on_developerModeButton_released()
{
    qApp->quit();
}

void Widget::on_rebootWithActivateMainProgramButton_released()
{
    removeRunFile();
    reboot();
}

void Widget::on_quitWithActivateMainProgramButton_released()
{
    removeRunFile();
    poweroff();
}

void Widget::onBluetoothStatusChanged(bool status)
{
    if (status) {
        ui->bluetoothStatusLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    }
    else {
        ui->bluetoothStatusLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
    }
}

void Widget::onDbusStatusChanged(bool status)
{
    if (status) {
        ui->bluetoothDbusStatusLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    }
    else {
        ui->bluetoothDbusStatusLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
    }
}

void Widget::onBluetoothBluetoothdStatusChanged(bool status)
{
    if (status) {
        ui->bluetoothBluetootdStatusLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    }
    else {
        ui->bluetoothBluetootdStatusLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
    }
}

void Widget::onBluetoothHCIDeviceStatusChanged(bool status)
{
    if (status) {
        ui->bluetoothHCIStatusLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    }
    else {
        ui->bluetoothHCIStatusLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
    }
}

void Widget::on_enableBluetooth_released()
{
    _bluetooth.enableBluetooth(true);
}

void Widget::on_disableBluetooth_released()
{
    _bluetooth.enableBluetooth(false);
}

void Widget::on_scanWifiButton_released()
{
    ui->interfacesListWidget->clear();
    ui->accessPointsListWidget->clear();
    qApp->processEvents();
    qApp->processEvents();
    QThread::sleep(1);

    _wirelessInterface->scanInterfaces();
    QStringList interfacesList = _wirelessInterface->availableInterfaces();
    int interfacesCount = interfacesList.count();
    qDebug() << "available inrterfaces: " << interfacesCount;
    for (unsigned short i = 0; i < interfacesCount; ++i) {
        ui->interfacesListWidget->addItem(interfacesList[i] + QString(" ") + QString((_wirelessInterface->isInterfaceConnected(i) ? "connected" : "disconnected")));
    }

    _wirelessInterface->scanAcessPoints();
    int apCount = _wirelessInterface->apCount();
    for (unsigned short i = 0; i < apCount; ++i) {
        if (_wirelessInterface->ssid(i) == "\"Test\"") {
            ui->accessPointsListWidget->addItem(_wirelessInterface->ssid(i) + QString(" ") + _wirelessInterface->address(i) + QString(" ") + _wirelessInterface->encryption(i));
            break;
        }
        continue;
    }
}

void Widget::connectWifi()
{
    int apCount = _wirelessInterface->apCount();
    for (unsigned short i = 0; i < apCount; ++i) {
        if (_wirelessInterface->ssid(i) == "\"Test\"") {
            _wirelessInterface->connectToNetwork(i);
            ui->accessPointsListWidget->addItem(_wirelessInterface->ssid(i) + QString(" ") + _wirelessInterface->address(i) + QString(" ") + _wirelessInterface->encryption(i));
            break;
        }
        continue;
    }

    ui->interfacesListWidget->clear();
    QStringList interfacesList = _wirelessInterface->availableInterfaces();
    int interfacesCount = interfacesList.count();
    for (unsigned short i = 0; i < interfacesCount; ++i) {
        ui->interfacesListWidget->addItem(interfacesList[i] + QString(" ") + QString((_wirelessInterface->isInterfaceConnected(i) ? "connected" : "disconnected")));
    }
}

void Widget::disconnectWifi()
{
    _wirelessInterface->disconnectFromToNetwork();

    ui->interfacesListWidget->clear();
    QStringList interfacesList = _wirelessInterface->availableInterfaces();
    int interfacesCount = interfacesList.count();
    for (unsigned short i = 0; i < interfacesCount; ++i) {
        ui->interfacesListWidget->addItem(interfacesList[i] + QString(" ") + QString((_wirelessInterface->isInterfaceConnected(i) ? "connected" : "disconnected")));
    }
}

void Widget::uploadFileViaWifi()
{
    _networkManager->clearAccessCache();
    _isUpload = true;
    _isFileUploading = true;
    generateFile(1000000);
    QFile* testFile = new QFile(sourcePath);
    testFile->open(QIODevice::ReadOnly);
    QNetworkRequest uploadReq(_ftpUrl);
    QNetworkReply* reply = _networkManager->put(uploadReq, testFile);
    disconnect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    disconnect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(uploadFinished(QNetworkReply*)));
    connect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(uploadFinished(QNetworkReply*)));
    connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(onUploadProgress(qint64, qint64)));
}

void Widget::onUploadProgress(qint64 done, qint64 total)
{
    int procent = (static_cast<double>(done) / total) * 100;
    ui->uploadLineEdit->setText(QString::number(procent) + QString("%"));
    qApp->processEvents();
    qApp->processEvents();
}

void Widget::onDownloadProgress(qint64 done, qint64 total)
{
    int procent = (static_cast<double>(done) / total) * 100;
    ui->downloadLineEdit->setText(QString::number(procent) + QString("%"));
    qApp->processEvents();
    qApp->processEvents();
}

void Widget::onReplyError(QNetworkReply::NetworkError error)
{
    qDebug() << error;
}

void Widget::uploadFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        qDebug() << reply->error() << " FAIL:" << reply->errorString();
    }
    else {
        if (_isFileUploading) {
            printTestResult(QString(tr("Check upload")), true);
        }
        else {
            printTestResult(QString(tr("Check upload")), false);
            return;
        }
    }
    emit fileIsUploaded();
}

void Widget::downloadFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        qDebug() << reply->error() << " FAIL:" << reply->errorString();
    }
    else {
        if (saveToDisk(downloadFilePath, reply)) {
            printTestResult(QString(tr("Check download")), true);
        }
        else {
            printTestResult(QString(tr("Check download")), false);
            return;
        }

        if (compareHash()) {
            ui->wifiTestListWidget->addItem(tr("Check checksum ... OK"));
            ui->wifiTestListWidget->addItems(QStringList() << QString() << QString());
            ui->wifiTestListWidget->addItem(tr("Test passed ... OK"));
        }
        else {
            ui->wifiTestListWidget->addItem(tr("Check checksum ... FAIL"));
            ui->wifiTestListWidget->addItems(QStringList() << QString() << QString());
            ui->wifiTestListWidget->addItem(tr("Test passed ... FAIL"));
        }
        ui->wifiTestListWidget->addItem(tr("FINISH"));
    }
}

void Widget::downloadFileViaWifi()
{
    _networkManager->clearAccessCache();
    _isUpload = false;
    _isFileDownloading = true;
    _networkManager->clearAccessCache();
    QNetworkRequest downloadRequest(_ftpUrl);
    QNetworkReply* reply = _networkManager->get(downloadRequest);
    disconnect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(uploadFinished(QNetworkReply*)));
    disconnect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    connect(_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
}

void Widget::on_testWifiButton_released()
{
    ui->wifiTestListWidget->clear();
    ui->uploadLineEdit->clear();
    ui->downloadLineEdit->clear();
    ui->wifiTestListWidget->addItem(tr("START"));
    qApp->processEvents();
    qApp->processEvents();
    disconnectWifi();
    connectWifi();

    QStringList interfacesList = _wirelessInterface->availableInterfaces();
    int interfacesCount = interfacesList.count();
    if (interfacesCount > 0) {
        printTestResult(QString(tr("Scan interfaces")), true);
    }
    else {
        printTestResult(QString(tr("Scan interfaces")), false);
        return;
    }

    if (_wirelessInterface->isInterfaceConnected(0)) {
        printTestResult(QString(tr("Check connection interfaces")), true);
    }
    else {
        printTestResult(QString(tr("Check connection interfaces")), false);
        return;
    }

    uploadFileViaWifi();
    disconnect(this, SIGNAL(fileIsUploaded()), this, SLOT(downloadFileViaWifi()));
    connect(this, SIGNAL(fileIsUploaded()), this, SLOT(downloadFileViaWifi()));
}

void Widget::on_restartCameraButton_released()
{
    ui->cameraWidget->stopCamera();
    ui->cameraWidget->startCamera();
}

void Widget::on_btScanButton_released()
{
    ui->btScanList->clear();
    setEnabledBtPageButtons(false);
    _btManager->startScan();
    setEnabledBtPageButtons(true);
}

void Widget::on_btPairButton_released()
{
    _lastPairDevice.clear();
    setEnabledBtPageButtons(false);
    QList<QListWidgetItem*> items = ui->btScanList->selectedItems();
    if (!items.empty()) {
        QListWidgetItem* item = items.first();
        QString str = item->text();
        QString mac = str.right(17);
        _lastPairDevice = mac;
        bool res = _btManager->pair(mac);
        res ? ui->pairBtLabel->setPixmap(QPixmap(":/resources/connectedStatus.png")) : ui->pairBtLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
        _btManager->addAudioDevice(mac);
    }
    setEnabledBtPageButtons(true);
}

void Widget::on_btTestButton_released()
{
    QString mediaPath = "/usr/local/avicon-31/test_avicon/noac.wav";
    _audioOutput->playSound(mediaPath);
}

void Widget::on_btResetButton_released()
{
    ui->btScanList->clear();
    setEnabledBtPageButtons(false);
    _audioOutput->stop();

    QProcess process;
    process.start("sh",
                  QStringList() << "-c"
                                << "rm -rf /usr/var/lib/bluetooth/*");
    process.waitForStarted();
    process.waitForFinished();
    qDebug() << "OUTPUT rm: " << process.readAllStandardOutput();
    QThread::sleep(1);

    _btManager->resetDevice();
    setEnabledBtPageButtons(true);
    on_setWireOutputButton_released();
    ui->pairBtLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
    _audioOutput->resume();
}

void Widget::onBtScanListChanged(QStringList devicesList)
{
    ui->btScanList->clear();
    ui->btScanList->addItems(devicesList);
}

void Widget::on_setBtOutputButton_released()
{
    setBtOutput();
    ui->btOutputLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    ui->btWireOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
    ui->wireOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
}

void Widget::on_setBtWireOutputButton_released()
{
    setBtWireOutput();
    ui->btOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
    ui->btWireOutputLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
    ui->wireOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
}

void Widget::on_setWireOutputButton_released()
{
    setWireOutput();
    ui->btOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
    ui->btWireOutputLabel->setPixmap(QPixmap(":/resources/umuOff.png"));
    ui->wireOutputLabel->setPixmap(QPixmap(":/resources/connectedStatus.png"));
}

void Widget::on_btUnPairButton_released()
{
    if (_lastPairDevice.isEmpty() == false) {
        _audioOutput->stop();
        _btManager->unpair(_lastPairDevice);
        ui->pairBtLabel->setPixmap(QPixmap(":/resources/disconnectStatus.png"));
    }
}
