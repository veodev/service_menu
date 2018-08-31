#ifndef WIDGET_H
#define WIDGET_H

#include "geoposition.h"
#include "audiooutput.h"
#include "utsvupowermanagement.h"
#include "avicon31bluetooth.h"
#include "bluetoothmanager.h"

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QKeyEvent>
#include <QMediaRecorder>
#include <QMediaPlayer>
#include <QAbstractSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class ExternalKeyboard;
class LM75;
class GenericTempSensor;
class UtsvuPowermanagement;
class QAudioRecorder;
class QSerialPort;
class QTcpSocket;
class QUdpSocket;
class WifiManager;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

protected:
    void mouseReleaseEvent(QMouseEvent * e);

private:
    void saveDefaultTsCalibration();
    void restoreDefaultTsCalibration();
    void setReleasedStyleForAllButtons();
    bool isFlashMount();
    bool copyFileToFlash();
    bool readFileFromFlash();
    void umuPower(bool isOn);
    void blockAudioChannelsButtons(bool isBlock);
    void setAudioChannelsToggledFalseButtons();
    bool openSerialPort();
    void closeSerialPort();
    quint64 testWriteToSerialPort();
    void clearUmuData();
    void clearDisplayData();
    void createLanConnection();
    void reboot();
    void poweroff();
    bool saveToDisk(const QString &filename, QIODevice * data);
    bool compareHash();
    void generateFile(int bytes);
    void clearWifiPageWidgets();
    void printTestResult(const QString & str, bool isPassed);
    void removeFileIfExist(const QString & path);
    void removeRunFile();
    void setEnabledBtPageButtons(bool isEnabled);
    void setBtWireOutput();
    void setWireOutput();
    void setBtOutput();

signals:
    void doSetVolume(double);
    void fileIsUploaded();

private slots:
    void on_displaySectionButton_released();
    void on_touchScreenSectionButton_released();
    void on_defaultCalibrationButton_released();
    void on_temperatureSectionButton_released();
    void on_gpsSectionButton_released();
    void on_soundSystemSectionButton_released();
    void on_cameraSectionButton_released();
    void on_externalKeyboardSectionButton_released();
    void on_wifiSectionButton_released();
    void on_bluetoothSectionButton_released();
    void on_usbSectionButton_released();
    void on_comSectionButton_released();
    void on_lanSectionButton_released();
    void on_powerSectionButton_released();
    void on_exitSectionButton_released();
    void on_touchScreenTestButton_released();
    void on_touchScreenCalibrationButton_released();
    void externalKeyToggled(bool isPressed, int keyCode);
    void on_usbTestButton_released();
    void updateTemperature();
    void updateVoltage();
    void on_leftBrightnessButton_released();
    void on_rightBrightnessButton_released();
    void on_screenBrightnessSlider_valueChanged(int value);
    void on_redButton_toggled(bool checked);
    void on_greenButton_toggled(bool checked);
    void on_blueButton_toggled(bool checked);
    void on_bit0Button_toggled(bool checked);
    void on_bit1Button_toggled(bool checked);
    void on_bit2Button_toggled(bool checked);
    void on_bit3Button_toggled(bool checked);
    void on_bit4Button_toggled(bool checked);
    void on_bit5Button_toggled(bool checked);
    void on_allBitsButton_toggled(bool checked);
    void on_gradationToRightButton_toggled(bool checked);
    void on_gradationToLeftButton_toggled(bool checked);
    void on_gradationToUpButton_toggled(bool checked);
    void on_gradationToDownButton_toggled(bool checked);

    void on_testRgbLcdButton_released();
    void execCommand();

    void on_plusHeatingTimeButton_released();
    void on_minusHeatingTimeButton_released();
    void on_stopHeatingTimerButton_released();
    void on_startHeatingTimerButton_toggled(bool checked);
    void stopHeatingTimer();
    void updateTimerLcd();

    void positionUpdated(const QGeoPositionInfo& info);
    void satellitesInUseUpdated(const QList<QGeoSatelliteInfo> &satellites);
    void antennaStatusChanged(GeoPosition::AntennaStatus antennaStatus);

    void on_test500HzFlashLeftButton_toggled(bool checked);
    void on_test500HzLeftButton_toggled(bool checked);
    void on_test1000HzLeftButton_toggled(bool checked);
    void on_test2000HzLeftButton_toggled(bool checked);
    void on_test2000HzFlashLeftButton_toggled(bool checked);
    void on_test500HzFlashRightButton_toggled(bool checked);
    void on_test500HzRightButton_toggled(bool checked);
    void on_test1000HzRightButton_toggled(bool checked);
    void on_test2000HzRightButton_toggled(bool checked);
    void on_test2000HzFlashRightButton_toggled(bool checked);

    void on_volumeSlider_valueChanged(int value);
    void onStackedWidgetChanged(int index);

    void on_umuPowerButton_toggled(bool checked);

    void updateRecordProgress(qint64 duration);
    void stateChanged(QMediaPlayer::State state);
    void updateState(QMediaRecorder::State state);
    void on_startRecordButton_toggled(bool checked);
    void on_stopRecordButton_toggled(bool checked);
    void on_startPlayButton_toggled(bool checked);
    void on_startPlayButton_released();

    void readData();
    void on_testComButton_released();
    void onCheckComTimerTimeout();
    void onTcpStateChanged(QAbstractSocket::SocketState state);
    void readDatagrams();
    void on_testLanButton_released();
    void on_reconnectButton_released();
    void displayUmuData();

    void on_rebootButton_released();
    void on_quitButton_released();
    void on_rebootWithActivateMainProgramButton_released();
    void on_quitWithActivateMainProgramButton_released();
    void on_developerModeButton_released();

    void onBluetoothStatusChanged(bool status);
    void onDbusStatusChanged(bool status);
    void onBluetoothBluetoothdStatusChanged(bool status);
    void onBluetoothHCIDeviceStatusChanged(bool status);
    void on_enableBluetooth_released();
    void on_disableBluetooth_released();

    void on_scanWifiButton_released();
    void connectWifi();
    void disconnectWifi();
    void uploadFileViaWifi();
    void onUploadProgress(qint64 done, qint64 total);
    void onDownloadProgress(qint64 done, qint64 total);
    void uploadFinished(QNetworkReply* reply);
    void downloadFinished(QNetworkReply* reply);
    void onReplyError(QNetworkReply::NetworkError error);
    void downloadFileViaWifi();
    void on_testWifiButton_released();

    void on_restartCameraButton_released();

    void on_btScanButton_released();
    void on_btPairButton_released();
    void on_btTestButton_released();
    void on_btResetButton_released();
    void onBtScanListChanged(QStringList devicesList);
    void on_setBtOutputButton_released();
    void on_setBtWireOutputButton_released();
    void on_setWireOutputButton_released();

    void on_btUnPairButton_released();

private:
    Ui::Widget *ui;

    ExternalKeyboard * _externalKeyboard;
    QByteArray _hashWriteFile;
    QByteArray _hashReadFile;
    QByteArray _hashUploadFile;
    QByteArray _hashDownloadFile;
    LM75 * _internalTemperature;
    GenericTempSensor * _externalTemperature;
    QTimer * _updateTimer;
    QWidget * _widget;
    QString _command;
    int _heatingMinutes;
    QTimer * _heatingTimer;
    QTimer * _checkHeatingTimerTimer;
    QTimer * _checkVoltageTimer;
    GeoPosition * _geoPosition;
    AudioOutput * _audioOutput;
    QThread * _audioOutputThread;
    UtsvuPowermanagement _powerManager;
    QTimer * _bluetoothTimer;
    Avicon31Bluetooth _bluetooth;

    QAudioRecorder * _audioRecorder;
    QMediaPlayer * _audioPlayer;
    QSerialPort * _serialPort;

    QTcpSocket * _tcpSocket;
    QUdpSocket * _udpSocket;

    QString _umuSerialNumber;
    QString _umuFirmwareVersion;
    QString _umuPlicFirmwareVersion;
    WifiManager * _wirelessInterface;

    QNetworkAccessManager * _networkManager;
    QUrl _ftpUrl;
    bool _isUpload;
    bool _isFileUploading;
    bool _isFileDownloading;

    bool _resultComTest;
    QTimer * _checkComTimer;

    BluetoothManager * _btManager;
    QString _lastPairDevice;
};

#endif // WIDGET_H
