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

contains(QMAKE_HOST.arch, x86):{
QMAKE_LFLAGS *= /MACHINE:X86
}

contains(QMAKE_HOST.arch, x86_64):{
QMAKE_LFLAGS *= /MACHINE:X64
    INCLUDEPATH += C:\opencv\build\include\ \
                   "C:\Program Files (x86)\GDCM 2.4\include\gdcm-2.4"
    LIBS += -L"C:\opencv\build\x64\vc11\lib"
    LIBS += -L"C:\Users\nlog1_000\Downloads\gdcm\bin\Debug"
    LIBS += -lopencv_core248 \
            -lopencv_highgui248 \
            -lopencv_imgproc248 \
            -lopencv_video248 \
            -lopencv_objdetect248
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
}

QMAKE_CXXFLAGS_CXX11 = -std=c++11

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
