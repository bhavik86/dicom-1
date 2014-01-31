#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QDebug>
#include <QtGui/QPixmap>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    _dicomReader = new DicomReader;

    fetchConnections();
}

MainWindow::~MainWindow() {
    delete ui;
    delete _dicomReader;
}

void MainWindow::fetchConnections() {
    QObject::connect(ui->actionLoadDicom, SIGNAL(triggered()), this, SLOT(readDicom()));
}

void MainWindow::readDicom() {
    QString dicomFileName = QFileDialog::getOpenFileName(this, tr("Load DICOM file for processing"), "./");
    if (dicomFileName != "") {
        _dicomReader->readFile(dicomFileName);
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
    }
}
