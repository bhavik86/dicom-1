#if defined(_WIN32)

#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

#endif

#include "dicomreader.h"
#include "StegerLines.h"

#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmAttribute.h"
#include "gdcmDataSetHelper.h"
#include "gdcmStringFilter.h"

#define WINDOW_DICOM_IMAGE "ctimage"
#define WINDOW_CONTOUR_IMAGE "contour"
#define WINDOW_RADON_2D "sinogram2d"

DicomReader::DicomReader(QObject * parent) :
    QObject(parent),
    _imageNumber(0) {
    if (!initOpenCL()) {
        cv::namedWindow(WINDOW_CONTOUR_IMAGE, cv::WINDOW_AUTOSIZE);//| cv::WINDOW_OPENGL);
        cv::namedWindow(WINDOW_DICOM_IMAGE, cv::WINDOW_AUTOSIZE);// | cv::WINDOW_OPENGL);
    }
    else {
        std::cerr << "OpenCL is not initialized... aborted" << std::endl;
        exit(0);
    }
}

int DicomReader::initOpenCL() {
    cv::ocl::PlatformsInfo platforms;

    if (cv::ocl::getOpenCLPlatforms(platforms)) {

        cv::ocl::DevicesInfo devices;

        cv::ocl::DeviceType deviceType[2] = {cv::ocl::CVCL_DEVICE_TYPE_GPU, cv::ocl::CVCL_DEVICE_TYPE_CPU};

        // for now just take the first OpenCL capable GPU or CPU

        for (uint i = 0; i < 2; i ++) {
            for (uint platformN = 0; platformN < platforms.size(); platformN ++) {

                if (cv::ocl::getOpenCLDevices(devices, deviceType[i], platforms[platformN])) {
                    try {
                        cv::ocl::setDevice(devices[0]);

                        std::cout << cv::ocl::Context::getContext()->getDeviceInfo().deviceName << " " <<
                                    cv::ocl::Context::getContext()->getDeviceInfo().deviceProfile << std::endl;

                        return OPENCL_ALL_OK;
                    }
                    catch (cv::Exception &){
                        continue;
                    }
                }
            }
        }

        return OPENCL_NOT_INITIALIZED;
    }
    else {
        return OPENCL_NOT_INITIALIZED;
    }
}

DicomReader::DicomReader(const QString & dicomFile, QObject * parent) :
    DicomReader(parent) {
    readFile(dicomFile);
}

DicomReader::~DicomReader() {
    reset(_ctImages, _images, _sinograms);
}

void DicomReader::reset(std::vector<cv::Mat*> & ctImages,
                        std::vector<cv::Mat*> & images,
                        std::vector<cv::Mat*> & sinograms) {
    qDeleteAll(ctImages.begin(), ctImages.end());
    ctImages.clear();
    qDeleteAll(images.begin(), images.end());
    images.clear();
    qDeleteAll(sinograms.begin(), sinograms.end());
    sinograms.clear();
}

int DicomReader::readImage(gdcm::File & dFile, const gdcm::Image & dImage, std::vector<cv::Mat*> & ctImages,
                             std::vector<cv::Mat*> & images, std::vector<cv::Mat*> & sinograms) {

    //clear previous "garbage"
    reset(ctImages, images, sinograms);

    std::vector<char>vbuffer;
    vbuffer.resize(dImage.GetBufferLength());
    char * buffer = &vbuffer[0];

    gdcm::StringFilter dStringFilter;
    dStringFilter.SetFile(dFile);

    gdcm::DataSet & dDataSet = dFile.GetDataSet();

    CtData ctData;

    gdcm::Tag tagFind(0x0028, 0x1050);
    if (dDataSet.FindDataElement(tagFind)) {
        ctData.windowCenter = std::stoi(dStringFilter.ToString(tagFind));
    }
    else {
        ctData.windowCenter = 0;
    }

    tagFind.SetElementTag(0x0028, 0x1051);
    if (dDataSet.FindDataElement(tagFind)) {
        ctData.windowWidth = std::stoi(dStringFilter.ToString(tagFind));
    }
    else {
        ctData.windowWidth = 0;
    }

    int imagesCount = dImage.GetDimension(2);

    dImage.GetBuffer(buffer);
    ctImages.resize(imagesCount);
    images.resize(imagesCount);
    sinograms.resize(imagesCount);

    ctData.ctImages = &ctImages;
    ctData.images = &images;
    ctData.sinograms = &sinograms;
    //MONOCHROME2

    gdcm::PhotometricInterpretation photometricInterpretation = dImage.GetPhotometricInterpretation();

    if (photometricInterpretation == gdcm::PhotometricInterpretation::MONOCHROME2) {
        ctData.inverseNeeded = true;
    }
    else {
        ctData.inverseNeeded = false;
    }

    gdcm::PixelFormat pixelFormat = dImage.GetPixelFormat();
    if (pixelFormat.GetScalarType() == gdcm::PixelFormat::UINT16) {
        ctData.type = CV_16UC1;
    }
    ctData.bytesAllocated = pixelFormat.GetBitsAllocated() / 8;
    ctData.minValue = pixelFormat.GetMin();
    ctData.maxValue = pixelFormat.GetMax();

    ctData.isLittleEndian = (pixelFormat.GetBitsAllocated() - pixelFormat.GetHighBit() == 1) ? true : false;

    ctData.width = dImage.GetDimension(0);
    ctData.height = dImage.GetDimension(1);

    try {
        ctData.slope = dImage.GetSlope();
        ctData.intercept = dImage.GetIntercept();
    }
    catch(...) {
        ctData.slope = 1.0;
        ctData.intercept = 0.0;
    }

    ctData.buffer = buffer;

    std::cout << "processing start" <<std::endl;

    //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(500, 500, CV_16UC1);
    //cv::ocl::GaussianBlur(*oclData, *oclData, cv::Size(5, 5), 5);

    cv::parallel_for_(cv::Range(0, imagesCount), CtProcessing<u_int16_t>(ctData));

    std::cout << "loading done" << std::endl;

    showImageWithNumber(0);

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

    readImage(dFile, dImage, _ctImages, _images, _sinograms);

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
    //cv::imshow(WINDOW_DICOM_IMAGE, *(_images[imageNumber]));
    cv::imshow(WINDOW_RADON_2D, *(_sinograms[imageNumber]));
    cv::waitKey(1);
}
