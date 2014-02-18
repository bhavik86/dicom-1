#ifndef DICOMREADER_H
#define DICOMREADER_H

#include <QObject>

#include "gdcmImage.h"
#include "gdcmFile.h"

#include "ctprocessing.hpp"

#include "opencv2/highgui/highgui.hpp"

#define DICOM_ALL_OK 0
#define DICOM_FILE_NOT_READABLE 1

#define OPENCL_ALL_OK 0
#define OPENCL_NOT_INITIALIZED 1

#define CANNY_LOWER 200

class DicomReader : public QObject {
  Q_OBJECT
public:
  explicit DicomReader(QObject * parent = 0);
  explicit DicomReader(const QString & dicomFile, QObject * parent = 0);

  ~DicomReader();

  int readImage(gdcm::File &dFile,
                  const gdcm::Image & dImage,
                  std::vector<cv::/*ocl::ocl*/Mat*> & ctImages,
                  std::vector<cv::Mat *> &images,
                  std::vector<cv::Mat*> & sinograms);

  QImage dQImage();
  cv::Mat dCImage();

  void decImageNumber();
  void incImageNumber();

  void reset(std::vector<cv::Mat*> & ctImages,
             std::vector<cv::Mat*> & images,
             std::vector<cv::Mat*> & sinograms);

private:
  int _imageNumber;
  std::vector<cv::/*ocl::ocl*/Mat*>_ctImages;
  std::vector<cv::Mat*>_images;
  std::vector<cv::Mat*>_sinograms;

  cv::ocl::Context * _context;

  int initOpenCL();

  void showImageWithNumber(const int & imageNumber);

signals:

public slots:
  int readFile(const QString & dicomFile);
};

#endif // DICOMREADER_H
