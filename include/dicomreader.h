#ifndef DICOMREADER_H
#define DICOMREADER_H

#include <QtCore/QObject>

#include <QtGui/QImage>

#include "gdcmImage.h"

#define DICOM_ALL_OK 0
#define DICOM_FILE_NOT_READABLE 1

class DicomReader : public QObject {
    Q_OBJECT
public:
    explicit DicomReader(QObject * parent = 0);
    explicit DicomReader(const QString & dicomFile, QObject * parent = 0);
    static QImage convertToFormat_RGB888(gdcm::Image const & gimage, char * buffer, int & error);

    QImage dQImage();

private:
    QImage _dQImage;

signals:

public slots:
    int readFile(const QString & dicomFile);
};

#endif // DICOMREADER_H
