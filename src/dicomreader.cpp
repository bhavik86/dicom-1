#if defined(_WIN32)

#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

#endif

#include "CL/cl.hpp"

#include "dicomreader.h"

#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmSmartPointer.h"
#include "gdcmDataSetHelper.h"
#include "gdcmStringFilter.h"
#include "gdcmDICOMDIR.h"

#define WINDOW_DICOM_IMAGE "ctimage"
#define WINDOW_CONTOUR_IMAGE "contour"

typedef struct _LoaderData {
    std::vector<cv::/*ocl::ocl*/Mat*> * ctImages;
    std::vector<cv::Mat*> * images;
    int bytesAllocated;
    int width;
    int height;
    int offset;
    int type;
    bool isLittleEndian;
    bool inverseNeeded;
    double slope;
    double intercept;
    char * buffer;
}LoaderData;

template <typename T>
class ParallelLoader : public cv::ParallelLoopBody {
private:
    LoaderData _loaderData;

public:
    ParallelLoader(LoaderData & loaderData) : _loaderData(loaderData) {
        _loaderData.offset = _loaderData.bytesAllocated * _loaderData.width * _loaderData.height;
    }

    virtual void operator ()(const cv::Range & r) const {
        for (register int i = r.start; i < r.end; i ++) {

            cv::Mat * data = new cv::Mat(_loaderData.width, _loaderData.height, _loaderData.type);
            T pixel;

            char * bufferImageI = _loaderData.buffer + _loaderData.offset * i;

            for (int x = 0; x < _loaderData.width; x ++) {
                for (int y = 0; y < _loaderData.height; y ++) {
                    pixel ^= pixel;

                    if (_loaderData.isLittleEndian) {
                        for (int k = 0; k < _loaderData.bytesAllocated; k ++) {
                            pixel |= (T)*(bufferImageI + k) << (8 * k);
                        }
                    }
                    else {
                        for (int k = _loaderData.bytesAllocated - 1; k > 0 ; k --) {
                            pixel |= (T)*(bufferImageI + k) << (8 * (_loaderData.bytesAllocated - k + 1));
                        }
                    }

                    bufferImageI += _loaderData.bytesAllocated;

                    //MONOCHROME2 - high value -> brighter, -1 high -> blacker

                    if (_loaderData.inverseNeeded) {
                        data->at<T>(x, y) = ~(T)(_loaderData.slope * pixel + _loaderData.intercept);
                    }
                    else {
                        data->at<T>(x, y) = _loaderData.slope * pixel + _loaderData.intercept;
                    }
                }
            }

            //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(*data);


            //cv::resize(*data, *data, cv::Size(_loaderData.width / 2, _loaderData.height / 2));
            //cv::GaussianBlur(*data, *data, cv::Size(9, 9), 5);
            //cv::dilate(*data, *data, cv::Mat(3, 3, CV_8UC1));
            //cv::Scharr(*data, *data, -1, 1, 0);

            cv::Mat * data8 = new cv::Mat(_loaderData.width, _loaderData.height, CV_8UC1);
            data->convertTo(*data8, CV_8UC1, 1/256.0);


            _loaderData.images->at(i) = data;
            //delete data;

            //cv::Canny(*data8, *data8, CANNY_LOWER, 3 * CANNY_LOWER, 5);


            //cv::ocl::GaussianBlur(*oclData, *oclData, cv::Size(5, 5), 5);
            //cv::ocl::Canny(*data, *data, CANNY_LOWER, 3 * CANNY_LOWER, 5);
            std::vector<std::vector<cv::Point> > contours;
            std::vector<cv::Vec4i> hierarchy;

            cv::Mat * contourImage = new cv::Mat(_loaderData.width, _loaderData.height, CV_8UC1, cv::Scalar(0));
            cv::Mat * laplace16 = new cv::Mat(_loaderData.width, _loaderData.height, CV_8UC1, cv::Scalar(0));

            //cv::threshold(*data8, *contourImage, 128, 255, CV_THRESH_BINARY);
            cv::GaussianBlur(*data8, *data8, cv::Size(5, 5), 5);
            //cv::Canny(*data8, *contourImage, CANNY_LOWER, 3 * CANNY_LOWER, 3);
            //cv::Sobel(*data8, *contourImage, -1, 1, 0);
            cv::Laplacian(*data8, *laplace16, CV_16SC1, 3);
            cv::convertScaleAbs(*laplace16, *contourImage);
            /*
            cv::findContours(*data8, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

            for (uint k = 0; k < contours.size(); k ++) {
                if (contours.at(k).size() > 10) {
                    cv::drawContours(*contourImage, contours, k, cv::Scalar(0x00FF), 2, 8, hierarchy, 0, cv::Point());
                }
            }*/

            delete data8;

            _loaderData.ctImages->at(i) = contourImage;
        }

    }
};

int DicomReader::initOpenCL() {
    cv::ocl::PlatformsInfo platforms;

    if (cv::ocl::getOpenCLPlatforms(platforms)) {

        cv::ocl::DevicesInfo devices;

        // for now just take the first OpenCL capable GPU
        for (uint platformN = 0; platformN < platforms.size(); platformN ++) {

            if (cv::ocl::getOpenCLDevices(devices, cv::ocl::CVCL_DEVICE_TYPE_GPU, platforms[platformN])) {
                cv::ocl::setDevice(devices[0]);

                std::cout << cv::ocl::Context::getContext()->getDeviceInfo().deviceName << " " <<
                             cv::ocl::Context::getContext()->getDeviceInfo().deviceProfile << std::endl;

                return OPENCL_ALL_OK;
            }
        }

        // if no OpenCL gpu-capable, take first capable CPU, otherwise - not initialized OpenCL
        for (uint platformN = 0; platformN < platforms.size(); platformN ++) {

            if (cv::ocl::getOpenCLDevices(devices, cv::ocl::CVCL_DEVICE_TYPE_CPU, platforms[platformN])) {
                cv::ocl::setDevice(devices[0]);

                std::cout << cv::ocl::Context::getContext()->getDeviceInfo().deviceName << " " <<
                             cv::ocl::Context::getContext()->getDeviceInfo().deviceProfile << std::endl;

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
        cv::namedWindow(WINDOW_CONTOUR_IMAGE, cv::WINDOW_AUTOSIZE | cv::WINDOW_OPENGL);
        cv::namedWindow(WINDOW_DICOM_IMAGE, cv::WINDOW_AUTOSIZE | cv::WINDOW_OPENGL);
    }
    else {
        std::cerr << "OpenCL is not initialized... aborted" << std::endl;
        exit(0);
    }

}

DicomReader::DicomReader(const QString & dicomFile, QObject * parent) :
    DicomReader(parent) {
    readFile(dicomFile);
}

DicomReader::~DicomReader() {
    reset(_ctImages, _images);
}

void DicomReader::reset(std::vector<cv::Mat*> & ctImages,
                        std::vector<cv::Mat*> & images) {
    qDeleteAll(ctImages.begin(), ctImages.end());
    ctImages.clear();
    qDeleteAll(images.begin(), images.end());
    images.clear();
}

int DicomReader::gImageToMat(const gdcm::Image & gImage, std::vector<cv::Mat*> & ctImages,
                             std::vector<cv::Mat*> & images) {

    int imagesCount = gImage.GetDimension(2);

    //clear previous "garbage"
    reset(ctImages, images);

    std::vector<char>vbuffer;
    vbuffer.resize(gImage.GetBufferLength());
    char * buffer = &vbuffer[0];

    gImage.GetBuffer(buffer);
    ctImages.resize(imagesCount);
    images.resize(imagesCount);

    LoaderData loaderData;
    loaderData.ctImages = &ctImages;
    loaderData.images = &images;
    //MONOCHROME2

    gdcm::PhotometricInterpretation photometricInterpretation = gImage.GetPhotometricInterpretation();
    if (photometricInterpretation == gdcm::PhotometricInterpretation::MONOCHROME2) {
        loaderData.inverseNeeded = true;
    }
    else {
        loaderData.inverseNeeded = false;
    }

    gdcm::PixelFormat pixelFormat = gImage.GetPixelFormat();
    if (pixelFormat.GetScalarType() == gdcm::PixelFormat::UINT16) {
        loaderData.type = CV_16UC1;
    }
    loaderData.bytesAllocated = pixelFormat.GetBitsAllocated() / 8;

    loaderData.isLittleEndian = (pixelFormat.GetBitsAllocated() - pixelFormat.GetHighBit() == 1) ? true : false;

    loaderData.width = gImage.GetDimension(0);
    loaderData.height = gImage.GetDimension(1);

    if (pixelFormat.GetSamplesPerPixel() == 1) {
        loaderData.slope = 1.0;
        loaderData.intercept = 0.0;
    }
    else {
        loaderData.slope = gImage.GetSlope();
        loaderData.intercept = gImage.GetIntercept();
    }

    loaderData.buffer = buffer;

    std::cout << "processing start" <<std::endl;

    //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(500, 500, CV_16UC1);
    //cv::ocl::GaussianBlur(*oclData, *oclData, cv::Size(5, 5), 5);

    cv::parallel_for_(cv::Range(0, imagesCount), ParallelLoader<u_int16_t>(loaderData));

    std::cout << "processing done" << std::endl;
    showImageWithNumber(0);

    return DICOM_ALL_OK;
}

int DicomReader::readFile(const QString & dicomFile) {
    gdcm::ImageReader dIReader;

    dIReader.SetFileName(dicomFile.toStdString().c_str());

    if (!dIReader.Read()) {
        return DICOM_FILE_NOT_READABLE;
    }

  //  gdcm::File & dFile = dIReader.GetFile();
    gdcm::Image & dImage = dIReader.GetImage();
/*
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
*/
    gImageToMat(dImage, _ctImages, _images);

    //findContours(_dClImages, _contourImages);


  /*  if (dHeader.FindDataElement(gdcm::Tag(0x2, 0x13))) {
        gdcm::DataElement & dE = dHeader.GetDataElement(gdcm::Tag(0x2, 0x13));
    }
*/
    return DICOM_ALL_OK;
}

void DicomReader::decImageNumber() {
    _imageNumber = ((_imageNumber) ? _imageNumber : _ctImages.size()) - 1;
    showImageWithNumber(_imageNumber);
}

void DicomReader::incImageNumber() {
    ++_imageNumber %= _ctImages.size();
    showImageWithNumber(_imageNumber);
}

void DicomReader::showImageWithNumber(const int & imageNumber) {
    cv::imshow(WINDOW_CONTOUR_IMAGE, *(_ctImages[imageNumber]));
    cv::imshow(WINDOW_DICOM_IMAGE, *(_images[imageNumber]));
    cv::waitKey(10);
}

QImage DicomReader::dQImage() {
    return _dQImage;
}
