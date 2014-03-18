#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

#include <QtGui/QKeyEvent>

#include "dicomreader.h"
#include "glviewer.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget * parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow * ui;

    DicomReader * _dicomReader;

    GLviewer * _glviewer;

    QSurfaceFormat format;

    void fetchConnections();
    void keyPressEvent(QKeyEvent * event);

    Images _images;

signals:
    void signalDicomToRead(const QString & dicomFile);

private slots:
    void readDicom();
};

#endif // MAINWINDOW_H
