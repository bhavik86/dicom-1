#if defined(_WIN32)

#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

#endif

#include <QFile>

#include <QDebug>
#include <QDateTime>

#include "dicomreader.h"

#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmSmartPointer.h"
#include "gdcmDataSetHelper.h"
#include "gdcmStringFilter.h"
#include "gdcmDICOMDIR.h"

#define WINDOW_CV_IMAGE "cvimage"

typedef struct _LoaderData {
    std::vector<cv::/*ocl::ocl*/Mat*> * ctImages;
    int byteSize;
    int width;
    int height;
    int offset;
    int type;
    double slope;
    double intercept;
    char * buffer;
}LoaderData;

class ParallelLoader : public cv::ParallelLoopBody {
private:
    LoaderData _loaderData;

public:
    ParallelLoader(LoaderData loaderData) : _loaderData(loaderData) {
        _loaderData.offset = _loaderData.byteSize * _loaderData.width * _loaderData.height;
    }

    virtual void operator ()(const cv::Range & r) const {
        for (register int i = r.start; i < r.end; i ++) {

            cv::Mat * data = new cv::Mat(_loaderData.width, _loaderData.height, _loaderData.type);
            u_int16_t pixel;

            char * bufferImageI = _loaderData.buffer + _loaderData.offset * i;

            for (int x = 0; x < _loaderData.width; x ++) {
                for (int y = 0; y < _loaderData.height; y ++) {
                    pixel ^= pixel;

                    pixel = ((u_int16_t) *(bufferImageI + 1) << 8) | *bufferImageI;
                    bufferImageI += 2;

                    data->at<u_int16_t>(x, y) = _loaderData.slope * pixel + _loaderData.intercept;
                }
            }

            //cv::resize(*data, *data, cv::Size(_width / 4, _height / 4));

            _loaderData.ctImages->at(i) = data;
        }

    }
};

int DicomReader::initOpenCL() {
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

DicomReader::DicomReader(QObject * parent) :
    QObject(parent),
    _imageNumber(0) {
    if (!initOpenCL()) {
        cv::namedWindow(WINDOW_CV_IMAGE,cv::WINDOW_AUTOSIZE);
    }
    else {
        std::cerr << "OPENCL_NOT_INITIALIZED" << std::endl;
        exit(0);
    }

}

DicomReader::DicomReader(const QString & dicomFile, QObject * parent) :
    DicomReader(parent) {
    readFile(dicomFile);
}

DicomReader::~DicomReader() {
    reset();
}

void DicomReader::reset() {
    qDeleteAll(_ctImages.begin(), _ctImages.end());
    _ctImages.clear();
}

int DicomReader::gImageToMat(const gdcm::Image & gImage, std::vector<cv::Mat*> & ctImages) {

    int imagesCount = gImage.GetDimension(2);

    //clear previous "garbage"
    qDeleteAll(ctImages.begin(), ctImages.end());
    ctImages.clear();

    std::vector<char>vbuffer;
    vbuffer.resize(gImage.GetBufferLength());
    char * buffer = &vbuffer[0];

    gImage.GetBuffer(buffer);
    ctImages.resize(imagesCount);

    qint64 time = QDateTime::currentMSecsSinceEpoch();

    LoaderData loaderData;
    loaderData.ctImages = &ctImages;
    //MONOCHROME2
    if (gImage.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME2) {
        loaderData.byteSize = 2;
        loaderData.type = CV_16SC1;
    }
    loaderData.width = gImage.GetDimension(0);
    loaderData.height = gImage.GetDimension(1);
    loaderData.slope = gImage.GetSlope();
    loaderData.intercept = gImage.GetIntercept();
    loaderData.buffer = buffer;

    cv::parallel_for_(cv::Range(0, imagesCount), ParallelLoader(loaderData));

    std::cout << "processing done" << std::endl;
    cv::imshow(WINDOW_CV_IMAGE, *(ctImages[0]));

    return DICOM_ALL_OK;
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

    gImageToMat(dImage, _ctImages);

    //findContours(_dClImages, _contourImages);


  /*  if (dHeader.FindDataElement(gdcm::Tag(0x2, 0x13))) {
        gdcm::DataElement & dE = dHeader.GetDataElement(gdcm::Tag(0x2, 0x13));
    }
*/
    return DICOM_ALL_OK;
}

void DicomReader::findContours(std::vector<cv::/*ocl::ocl*/Mat*> & dClImages, std::vector<cv::Mat*> & contourImages) {

    cv::/*ocl::ocl*/Mat * image;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;

    for (uint i = 0; i < dClImages.size(); i ++) {
        image = dClImages[i];

        cv::Mat * contourImage = new cv::Mat(image->rows, image->cols, CV_16UC1);

        std::cout << "heRE" << std::endl;

        cv::threshold(*image, *contourImage, 128, 255, CV_THRESH_BINARY);
        //cv::GaussianBlur(*image, *contourImage, cv::Size(3,3), 5);
        //cv::Canny(*image, *image, CANNY_LOWER, 3 * CANNY_LOWER, 3);
        /*cv::findContours(*image, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        for (uint k = 0; k < contours.size(); k ++) {
            cv::drawContours(*contourImage, contours, k, cv::Scalar(0x4FFFFFFF), 2, 8, hierarchy, 0, cv::Point());
        }*/

        contourImages.push_back(contourImage);
    }
/*
    for (uint i = 1; i < dCvImages.size(); i ++) {
        contourImages.push_back(new cv::Mat(*(dCvImages[i])));
    }
*/
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
    _imageNumber = ((_imageNumber) ? _imageNumber : _ctImages.size()) - 1;
    showImageWithNumber();
}

void DicomReader::incImageNumber() {
    ++_imageNumber %= _ctImages.size();
    showImageWithNumber();
}

void DicomReader::showImageWithNumber() {
    cv::imshow(WINDOW_CV_IMAGE, *(_ctImages[_imageNumber]));
    cv::waitKey(10);
}

QImage DicomReader::dQImage() {
    return _dQImage;
}
