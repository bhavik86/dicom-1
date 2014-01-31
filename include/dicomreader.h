#ifndef DICOMREADER_H
#define DICOMREADER_H

#include <QObject>

#include <QImage>

#include "gdcmImage.h"

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ocl/ocl.hpp"

#define DICOM_ALL_OK 0
#define DICOM_FILE_NOT_READABLE 1

#define OPENCL_ALL_OK 0
#define OPENCL_NOT_INITIALIZED 1

#define CANNY_LOWER 50

class DicomReader : public QObject {
  Q_OBJECT
public:
  explicit DicomReader(QObject * parent = 0);
  explicit DicomReader(const QString & dicomFile, QObject * parent = 0);

  ~DicomReader();

  static int gImageToMat(const gdcm::Image & gImage, std::vector<cv::/*ocl::ocl*/Mat*> & ctImages);

  static void findContours(std::vector<cv::Mat *> &ctImages, std::vector<cv::Mat*> & contourImages);

  QImage dQImage();
  cv::Mat dCImage();

  void decImageNumber();
  void incImageNumber();

  void showImageWithNumber();

  void reset();

private:
  int _imageNumber;
  QImage _dQImage;
  std::vector<cv::/*ocl::ocl*/Mat*>_ctImages;
  std::vector<cv::Mat*>_contourImages;

  int initOpenCL();

signals:

public slots:
  int readFile(const QString & dicomFile);
  int readFileByHand(const QString & dicomFileName);
};

#endif // DICOMREADER_H
