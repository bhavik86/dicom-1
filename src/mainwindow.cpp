#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QDebug>

#include <QtGui/QPixmap>
#include <QtGui/QSurfaceFormat>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);

    _dicomReader = new DicomReader(this);
    //_dicomReader->reset(_images);

    QSurfaceFormat format;
    format.setSamples(16);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);

    _glviewer = new GLviewer;

    _glviewer->setSurfaceType(QSurface::OpenGLSurface);
    _glviewer->setFormat(format);
    _glviewer->resize(640, 480);
    _glviewer->show();

    fetchConnections();
}

MainWindow::~MainWindow() {
    delete ui;
    delete _glviewer;
}

void MainWindow::fetchConnections() {
    QObject::connect(ui->actionLoadDicom, SIGNAL(triggered()), this, SLOT(readDicom()));
}

void MainWindow::readDicom() {
    QString dicomFileName = QFileDialog::getOpenFileName(this, tr("Load DICOM file for processing"), "/home/walkindude/dicomF/");
    if (dicomFileName != "") {
        _dicomReader->readFile(dicomFileName, _images);
        _glviewer->loadModel(_images.ctImages);
    }
    else {
        QMessageBox::critical(this, "Error", "Empty file", QMessageBox::Ok);
        return;
    }
}

void MainWindow::keyPressEvent(QKeyEvent * event) {
    switch(event->key()) {
        case Qt::Key_Left : _dicomReader->decImageNumber(); break;
        case Qt::Key_Right : _dicomReader->incImageNumber(); break;
        case Qt::Key_Space : _dicomReader->readFile("/home/walkindude/dicomF/Martyshkina_Svetlana_Martyshkina, Svetlana 05.03.120001.dcm",
                                                    _images); break;
        case Qt::Key_Escape : qApp->quit();
    }
}
