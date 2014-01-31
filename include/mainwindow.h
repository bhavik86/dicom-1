#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QKeyEvent>

#include "dicomreader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget * parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow * ui;

    DicomReader * _dicomReader;

    void fetchConnections();
    void keyPressEvent(QKeyEvent * event);

signals:
    void signalDicomToRead(const QString & dicomFile);

private slots:
    void readDicom();
};

#endif // MAINWINDOW_H
