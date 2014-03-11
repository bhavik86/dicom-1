#-------------------------------------------------
#
# Project created by QtCreator 2014-01-24T14:23:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dicom
TEMPLATE = app

CONFIG += c++11

INCLUDEPATH += include

unix {
    INCLUDEPATH += /usr/include/gdcm-2.4

    CONFIG += link_pkgconfig
    PKGCONFIG += opencv
}

win32 {
    INCLUDEPATH += C:\opencv\build\include \
                   C:\GDCM\gdcm\include

    !contains(QMAKE_HOST.arch, x86_64) {
            QMAKE_LFLAGS *= /MACHINE:X86
            LIBS += -L"C:\opencv\build\x86\vc12\lib"
    }
    else {
        contains(QMAKE_HOST.arch, x86_64):{
            QMAKE_LFLAGS *= /MACHINE:X64
            LIBS += -L"C:\opencv\build\x64\vc12\lib"
        }
    }

    LIBS += -L"C:\GDCM\gdcm\bin\Debug"

    LIBS += -lopencv_core248 \
            -lopencv_highgui248 \
            -lopencv_imgproc248 \
            -lopencv_ocl248
}

LIBS += -lgdcmcharls \
        -lgdcmjpeg12 \
        -lgdcmCommon \
        -lgdcmjpeg16 \
        -lgdcmDICT \
        -lgdcmjpeg8 \
        -lgdcmDSED \
        -lgdcmMSFF \
        -lgdcmIOD\
        -lgdcmopenjpeg

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
