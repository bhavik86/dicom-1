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
    QString dicomFile = QFileDialog::getOpenFileName(this, tr("Load DICOM file for processing"), "./");
    if (dicomFile != "") {
        if (_dicomReader->readFile(dicomFile) == DICOM_ALL_OK) {
            ui->gImage->setPixmap(QPixmap::fromImage(_dicomReader->dQImage()));
        }
    }
    else {
        QMessageBox::critical(this, "Error", "Empty file", QMessageBox::Ok);
        return;
    }
}
