#-------------------------------------------------
#
# Project created by QtCreator 2017-05-11T17:16:52
#
#-------------------------------------------------

QT       += core gui sensors positioning serialport multimedia network bluetooth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test_avicon
TEMPLATE = app

QMAKE_LFLAGS+=-static-libstdc++
CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lasound

INCLUDEPATH += \
    ../avicon/core/externalkeyboard \
    ../avicon/core/gpio \
    ../avicon \
    ../avicon/core \
    ../avicon/core/temperature \
    ../avicon/core/temperature/lm75 \
    ../avicon/core/temperature/generic \
    ../avicon/core/screen \
    ../avicon/core/geoposition \
    ../avicon/core/audio \
    ../avicon/ui/widgets/camera/mxccamerapage \
    ../avicon/core/mxccamera \
    ../avicon/core/powermanagement \
    ../avicon/core/wifimanager \
    ../avicon/core/bluetooth

HEADERS  += widget.h \
    ../avicon/core/externalkeyboard/externalkeyboard.h \
    ../avicon/core/gpio/gpio.h \
    ../avicon/debug.h \
    ../avicon/core/temperature/temperaturesensor.h \
    ../avicon/core/temperature/lm75/lm75.h \
    ../avicon/core/temperature/generic/genericTempSensor.h \
    ../avicon/core/screen/screen.h \
    ../avicon/core/geoposition/geoposition.h \
    ../avicon/core/geoposition/locationutils.h \
    ../avicon/core/geoposition/nmeapositioninfosource.h \
    ../avicon/core/geoposition/nmeareader.h \
    ../avicon/core/geoposition/nmeasatelliteinfosource.h \
    ../avicon/core/audio/audiogenerator.h \
    ../avicon/core/audio/audiooutput.h \
    ../avicon/ui/widgets/camera/mxccamerapage/camerapage.h \
    ../avicon/core/mxccamera/mxccamera.h \
    ../avicon/core/mxccamera/mxccameraimagecapture.h \
    ../avicon/core/mxccamera/mxcmediarecorder.h \
    ../avicon/core/appdatapath.h \
    ../avicon/core/powermanagement/utsvupowermanagement.h \
    ../avicon/core/powermanagement/powermanagement.h \
    ../avicon/core/bluetooth/bluetoothmanager.h \
    ../avicon/core/bluetooth/avicon31bluetooth.h \
    ../avicon/core/wifimanager/wifimanager.h \
    ../avicon/core/audio/sounddata.h

SOURCES += main.cpp \
        widget.cpp \
    ../avicon/core/externalkeyboard/externalkeyboard.cpp \
    ../avicon/core/gpio/gpio.cpp \
    ../avicon/core/temperature/lm75/lm75.cpp \
    ../avicon/core/temperature/generic/genericTempSensor.cpp \
    ../avicon/core/screen/brightness_utsvu.cpp \
    ../avicon/core/geoposition/geoposition.cpp \
    ../avicon/core/geoposition/locationutils.cpp \
    ../avicon/core/geoposition/nmeapositioninfosource.cpp \
    ../avicon/core/geoposition/nmeareader.cpp \
    ../avicon/core/geoposition/nmeasatelliteinfosource.cpp \
    ../avicon/core/audio/audiogenerator.cpp \
    ../avicon/core/audio/audiooutput.cpp \
    ../avicon/ui/widgets/camera/mxccamerapage/camerapage.cpp \
    ../avicon/core/mxccamera/mxccamera.cpp \
    ../avicon/core/mxccamera/mxccameraimagecapture.cpp \
    ../avicon/core/mxccamera/mxcmediarecorder.cpp \
    ../avicon/core/appdatapath.cpp \
    ../avicon/core/powermanagement/utsvupowermanagement.cpp \
    ../avicon/core/powermanagement/powermanagement.cpp \
    ../avicon/core/bluetooth/avicon31bluetooth.cpp \
    ../avicon/core/wifimanager/wifimanager.cpp \
    ../avicon/core/audio/sounddata.cpp

FORMS    += widget.ui \
    ../avicon/ui/widgets/camera/mxccamerapage/camerapage.ui

linux-imx51-g++ | linux-buildroot-g++{
    DEFINES += IMX_DEVICE
    target.path = /usr/local/avicon-31/test_avicon
    INSTALLS += target
}
linux-buildroot-g++ {
    DEFINES += IMX_DEVICE
    target.path = /usr/local/avicon-31/test_avicon
    INSTALLS += target
}

RESOURCES += \
    resourses.qrc

TRANSLATIONS = /translations/russian.ts

