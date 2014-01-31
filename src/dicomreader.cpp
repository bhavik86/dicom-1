#if defined(_WIN32)

#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

#endif

#include <QFile>

#include <QDebug>

#include "dicomreader.h"

#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmSmartPointer.h"
#include "gdcmDataSetHelper.h"
#include "gdcmStringFilter.h"
#include "gdcmDICOMDIR.h"

#define WINDOW_CV_IMAGE "cvimage"

class ParallelLoader : public cv::ParallelLoopBody {
private:
    std::vector<cv::/*ocl::ocl*/Mat*> * _matArray;
    int _width;
    int _height;
    int _type;
    double _slope;
    double _intercept;
    char * _buffer;

public:
    ParallelLoader(std::vector<cv::/*ocl::ocl*/Mat*> * matArray,
                   const int width,
                   const int height,
                   const int type,
                   const double slope,
                   const double intercept,
                   char * buffer) :
        _matArray(matArray),
        _width(width),
        _height(height),
        _type(type),
        _slope(slope),
        _intercept(intercept),
        _buffer(buffer) {

    }

    virtual void operator ()(const cv::Range & r) const {
        for (register int i = r.start; i < r.end; i ++) {

            cv::Mat * data = new cv::Mat(_width, _height, _type);
            u_int16_t pixel;

            char * bufferElem = _buffer + 2 * _width * _height * i;

            for (int x = 0; x < _width; x ++) {
                for (int y = 0; y < _height; y ++) {
                    pixel ^= pixel;

                    pixel = ((u_int16_t) *(bufferElem + 1) << 8) | *bufferElem;
                    bufferElem += 2;

                    data->at<u_int16_t>(x, y) = _slope * pixel + _intercept;
                }
            }

            _matArray->at(i) = data;
        }

    }
};

DicomReader::DicomReader(QObject * parent) :
    QObject(parent),
    _imageNumber(0) {
    initializeOpenCL();
    cv::namedWindow(WINDOW_CV_IMAGE, cv::WINDOW_OPENGL | cv::WINDOW_AUTOSIZE);
}

DicomReader::DicomReader(const QString & dicomFile, QObject * parent) :
    DicomReader(parent) {
    readFile(dicomFile);
}

DicomReader::~DicomReader() {
    reset();
}

void DicomReader::reset() {
    qDeleteAll(_dClImages.begin(),_dClImages.end());
    _dClImages.clear();
}

int DicomReader::gImageToMat(const gdcm::Image & gImage, std::vector<cv::Mat*> &dClImages) {

    int width = gImage.GetDimension(0);
    int height = gImage.GetDimension(1);
    int imagesCount = gImage.GetDimension(2);

    // What types to expect here besides UINT16?
    int type = CV_16UC1;
    /*
    switch (gImage.GetPixelFormat()) {
        case gdcm::PixelFormat::UINT16: type = CV_16UC1; break;
    }
    */

    //clear previous "garbage"
    qDeleteAll(dClImages.begin(), dClImages.end());
    dClImages.clear();

    double slope = gImage.GetSlope();
    double intercept = gImage.GetIntercept();

    qDebug() << slope << " " << intercept;

    std::vector<char>vbuffer;
    vbuffer.resize(gImage.GetBufferLength());
    char * buffer = &vbuffer[0];

    gImage.GetBuffer(buffer);
    dClImages.resize(imagesCount);

    cv::parallel_for_(cv::Range(0, imagesCount), ParallelLoader(&dClImages, width, height, type, slope, intercept, buffer));

    std::cout << "here" << std::endl;
    cv::imshow(WINDOW_CV_IMAGE, *(dClImages[0]));

    return DICOM_ALL_OK;
}

int DicomReader::initializeOpenCL() {
    cv::ocl::PlatformsInfo platforms;

    if (cv::ocl::getOpenCLPlatforms(platforms)) {

        std::cout << platforms[1]->platformName << std::endl;

        cv::ocl::DevicesInfo devices;

        // for now just take the first OpenCL capable GPU
        for (uint platformN = 0; platformN < platforms.size(); platformN ++) {

            if (cv::ocl::getOpenCLDevices(devices, cv::ocl::CVCL_DEVICE_TYPE_GPU, platforms[platformN])) {
                cv::ocl::setDevice(devices[0]);

                std::cout << devices[0]->deviceName << " " << devices[0]->deviceDriverVersion << std::endl;

                return OPENCL_ALL_OK;
            }
        }

        return OPENCL_NOT_INITIALIZED;
    }
    else {
        return OPENCL_NOT_INITIALIZED;
    }
}

int DicomReader::readFile(const QString & dicomFile) {
    gdcm::ImageReader dIReader;

    dIReader.SetFileName(dicomFile.toStdString().c_str());

    if (!dIReader.Read()) {
        return DICOM_FILE_NOT_READABLE;
    }

    gdcm::File & dFile = dIReader.GetFile();
    gdcm::Image & dImage = dIReader.GetImage();

    gdcm::StringFilter dStringFilter;
    dStringFilter.SetFile(dFile);

    gdcm::FileMetaInformation & dHeader = dFile.GetHeader();

    gdcm::Tag tagTransfSyntax(0x0002, 0x0010);
    if (dHeader.FindDataElement(tagTransfSyntax)) {
        //gdcm::DataElement & dTransfSyntax = dHeader.GetDataElement(tagTransfSyntax);
        //1.2.840.10008.1.2.4.70 - JPEG Lossless process 14
        std::cout << dStringFilter.ToString(tagTransfSyntax) << std::endl;
    }
    else {
        return DICOM_FILE_NOT_READABLE;
    }

    gImageToMat(dImage, _dClImages);

    //findContours(_dClImages, _contourImages);

    //_dQImage = convertToFormat_RGB888(dImage, buffer, error);

  /*  if (dHeader.FindDataElement(gdcm::Tag(0x2, 0x13))) {
        gdcm::DataElement & dE = dHeader.GetDataElement(gdcm::Tag(0x2, 0x13));
    }
*/
    return DICOM_ALL_OK;
}

void DicomReader::findContours(std::vector<cv::ocl::oclMat*> & dCvImages, std::vector<cv::Mat*> & contourImages) {

    cv::ocl::oclMat * image;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;

    for (uint i = 0; i < dCvImages.size(); i ++) {
        image = dCvImages[i];

        cv::Mat * contourImage = new cv::Mat(image->rows, image->cols, CV_32SC1, cv::Scalar(0x00000000));

        std::cout << "heRE" << std::endl;

        //cv::Canny(*image, *image, CANNY_LOWER, 3 * CANNY_LOWER, 3);
        cv::findContours(*image, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        for (uint k = 0; k < contours.size(); k ++) {
            cv::drawContours(*contourImage, contours, k, cv::Scalar(0xFFFFFFFF), 2, 8, hierarchy, 0, cv::Point());
        }

        contourImages.push_back(contourImage);
    }

}

int DicomReader::readFileByHand(const QString & dicomFileName) {
    QFile dicomFile(dicomFileName);
    char * buffer = new char[4];

    if (!dicomFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        dicomFile.close();
        return DICOM_FILE_NOT_READABLE;
    }

    dicomFile.seek(128);
    dicomFile.read(buffer, 4);

    if (strcmp(buffer, "DICM")) {
        qDebug() << "not_ok";
        dicomFile.close();
        return DICOM_FILE_NOT_READABLE;
    }
    else {
        std::cout << buffer << std::endl;
    }

    char * group = new char[2];
    char * number = new char[2];
    char * vr = new char[2];

    quint64 pos = 132;
    dicomFile.seek(pos);
    dicomFile.read(group, 2);
    dicomFile.read(number, 2);

    while (!dicomFile.atEnd() && !(*group ^= 0x0002) && !(*number ^= 0x0010)) {
        dicomFile.read(vr, 2);
        
    }

    std::cout << "HERE";
    dicomFile.close();
    return DICOM_ALL_OK;
}

void DicomReader::decImageNumber() {
    _imageNumber = ((_imageNumber) ? _imageNumber : _dClImages.size()) - 1;
    showImageWithNumber();
}

void DicomReader::incImageNumber() {
    ++_imageNumber %= _dClImages.size();
    showImageWithNumber();
}

void DicomReader::showImageWithNumber() {
    std::cout << _imageNumber << std::endl;
    cv::imshow(WINDOW_CV_IMAGE, *(_dClImages[_imageNumber]));
    cv::waitKey(10);
}

QImage DicomReader::dQImage() {
    return _dQImage;
}
