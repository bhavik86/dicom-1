#-------------------------------------------------
#
# Project created by QtCreator 2014-01-24T14:23:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dicom
TEMPLATE = app

INCLUDEPATH += include

unix {
    INCLUDEPATH += /usr/include/gdcm-2.4

    CONFIG += link_pkgconfig warn_on
    PKGCONFIG += opencv

    LIBS += -lgdcmcharls \
            -lgdcmjpeg12 \
            -lgdcmCommon \
            -lgdcmjpeg16 \
            -lgdcmDICT \
            -lgdcmjpeg8 \
            -lgdcmDSED \
            -lgdcmMSFF \
            -lgdcmIOD\
            -lgdcmopenjpeg\
            -ltbb_debug
}

win32 {
    INCLUDEPATH += C:\opencv\build\include\
    LIBS += -L C:\opencv\build\x64\mingw\lib -libopencv_core245 \
                                         -libopencv_highgui245 \
                                         -libopencv_imgproc245 \
                                         -libopencv_video245 \
                                         -libopencv_objdetect245
}

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += src/main.cpp\
        src/mainwindow.cpp\
        src/dicomreader.cpp\
        src/normal.cpp\
        src/GaussDeriv.cpp\
        src/StegerLines.cpp

HEADERS  += include/mainwindow.h\
            include/dicomreader.h\
            include/normal.h\
            include/GaussDeriv.h\
            include/StegerLines.h \
    include/ctprocessing.hpp

FORMS    += ui/mainwindow.ui
