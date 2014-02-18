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

#define RADON_DEGREE_RANGE 180

typedef struct _LoaderData {
    std::vector<cv::/*ocl::ocl*/Mat*> * ctImages;
    std::vector<cv::Mat*> * images;
    std::vector<cv::Mat*> * sinograms;
    int bytesAllocated;
    int width;
    int height;
    int offset;
    int type;

    int64_t minValue;
    int64_t maxValue;
    int64_t windowCenter;
    int64_t windowWidth;

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
        int width = _loaderData.width / 2;
        int height = _loaderData.height / 2;

        int pad = std::max(width, height);

        int widthPad = std::ceil((pad - width) / 2.0);
        int heightPad = std::ceil((pad - height) / 2.0);

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


                    /* similar as http://code.google.com/p/pydicom/source/browse/source/dicom/contrib/pydicom_PIL.py*/
                    pixel = _loaderData.slope * pixel + _loaderData.intercept;

                    if (pixel <= (_loaderData.windowCenter - 0.5 - (_loaderData.windowWidth - 1) / 2.0)) {
                        pixel = (T)_loaderData.minValue;
                    }
                    else if (pixel > (_loaderData.windowCenter - 0.5 + (_loaderData.windowWidth - 1) / 2.0)) {
                        pixel = (T)_loaderData.maxValue;
                    }
                    else {
                        pixel = ((pixel - _loaderData.windowCenter + 0.5) / (_loaderData.windowWidth - 1) + 0.5) *
                                (_loaderData.maxValue - _loaderData.minValue);
                    }

                    //MONOCHROME2 - high value -> brighter, -1 high -> blacker
                    if (_loaderData.inverseNeeded) {
                        data->at<T>(x, y) = ~pixel;
                    }
                    else {
                        data->at<T>(x, y) = pixel;
                    }
                }
            }


            cv::resize(*data, *data, cv::Size(width, height));

            //cv::GaussianBlur(*data, *data, cv::Size(9, 9), 5);
            //cv::dilate(*data, *data, cv::Mat(3, 3, CV_8UC1));
            //cv::Scharr(*data, *data, -1, 1, 0);

            cv::Mat data8(width, height, CV_8UC1);
            data->convertTo(data8, CV_8UC1, 1/256.0);

            _loaderData.images->at(i) = data;

            //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(*data8);

            std::vector<std::vector<cv::Point> > contours;
            std::vector<cv::Vec4i> hierarchy;

            cv::Mat * contourImage = new cv::Mat(cv::Mat::zeros(height, width, CV_8UC1));

            cv::GaussianBlur(data8, *contourImage, cv::Size(5, 5), 5);

            //---radon---//

            cv::Mat imgPad(cv::Mat::zeros(height + heightPad, width + widthPad, CV_16UC1));
            cv::Mat imgPadRotated(cv::Mat::zeros(height + heightPad, width + widthPad, CV_16UC1));

            cv::Mat * sinogram = new cv::Mat(cv::Mat::zeros(height + heightPad, 0, CV_32FC1));

            data->copyTo(imgPad(cv::Rect(ceil(widthPad / 2.0), ceil(heightPad / 2.0), width, height)));
            cv::Point2i center = cv::Point2i((width + widthPad) / 2, (height + heightPad) / 2);

            cv::Mat rotationMatrix;
            cv::Mat colSum(cv::Mat::zeros(imgPad.cols, 1, CV_32FC1));

            cv::Size size(imgPad.rows, imgPad.cols);

            for (int angle = 0; angle != RADON_DEGREE_RANGE; angle ++) {
                rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
                cv::warpAffine(imgPad, imgPadRotated, rotationMatrix, size, INTER_LINEAR | WARP_INVERSE_MAP, BORDER_TRANSPARENT);

                cv::reduce(imgPadRotated, colSum, 0, CV_REDUCE_SUM, CV_32FC1);
                sinogram->push_back(colSum);
            }

            delete data;

            _loaderData.images->at(i) = contourImage;

            _loaderData.sinograms->at(i) = sinogram;

            //cv::medianBlur(*data8, *contourImage, 5);
            //cv::Canny(*contourImage, *contourImage, CANNY_LOWER, 3 * CANNY_LOWER, 3);

            //cv::threshold(*contourImage, *contourImage, 250, 255, CV_THRESH_OTSU);
            //cv::dilate(*contourImage, *contourImage, 19);

            //cv::Mat * steger = new cv::Mat();
            //stegerEdges(*contourImage, *steger, 5, 0, 10.0);

            //cv::adaptiveThreshold(*contourImage, *contourImage, 200, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 3, 1);
/*
            cv::findContours(data8, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_TC89_KCOS, cv::Point(0, 0));

            for (uint k = 0; k < contours.size(); k ++) {
                if (contours.at(k).size() > 300) {
                    cv::drawContours(*contourImage, contours, k, cv::Scalar(0x00FF), 2, 8, hierarchy, 0, cv::Point());
                }
            }
*/
            //oclData->download(*contourImage);

            //delete contourImage;
            //delete oclData;

            _loaderData.ctImages->at(i) = contourImage;
        }

    }
};

typedef struct _SinogramData {
    std::vector<cv::Mat*> ctImages;
    cv::Mat * sinogram;

    int rows;
    int cols;
}SinogramData;

class SinogramMaker : public cv::ParallelLoopBody {
private:
    SinogramData _sinogramData;
public:
    SinogramMaker(SinogramData & sinogramData) : _sinogramData(sinogramData) {
        _sinogramData.rows = _sinogramData.ctImages.at(0)->rows;
        _sinogramData.cols = _sinogramData.ctImages.at(0)->cols;
    }

    virtual void operator ()(const cv::Range & r) const {
        //---radon---//
        for (register int i = r.start; i < r.end; i ++) {
            cv::Mat colSum(cv::Mat::zeros(_sinogramData.cols, 1, CV_32FC1));
            cv::reduce(*_sinogramData.ctImages.at(i), colSum, 0, CV_REDUCE_SUM, CV_32FC1);

            colSum.row(0).copyTo(_sinogramData.sinogram->row(i));
        }
    }
};

DicomReader::DicomReader(QObject * parent) :
    QObject(parent),
    _imageNumber(0),
    _sinogram(NULL) {
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
    reset(_ctImages, _images, _sinograms, &_sinogram);
}

void DicomReader::reset(std::vector<cv::Mat*> & ctImages,
                        std::vector<cv::Mat*> & images,
                        std::vector<cv::Mat*> & sinograms,
                        cv::Mat ** sinogram) {
    qDeleteAll(ctImages.begin(), ctImages.end());
    ctImages.clear();
    qDeleteAll(images.begin(), images.end());
    images.clear();
    qDeleteAll(sinograms.begin(), sinograms.end());
    sinograms.clear();
    if (*sinogram != NULL) {
        delete *sinogram;
    }
}

int DicomReader::readImage(gdcm::File & dFile, const gdcm::Image & dImage, std::vector<cv::Mat*> & ctImages,
                             std::vector<cv::Mat*> & images, std::vector<cv::Mat*> & sinograms, cv::Mat ** sinogram) {

    //clear previous "garbage"
    reset(ctImages, images, sinograms, sinogram);

    std::vector<char>vbuffer;
    vbuffer.resize(dImage.GetBufferLength());
    char * buffer = &vbuffer[0];

    gdcm::StringFilter dStringFilter;
    dStringFilter.SetFile(dFile);

    gdcm::DataSet & dDataSet = dFile.GetDataSet();

    LoaderData loaderData;

    gdcm::Tag tagFind(0x0028, 0x1050);
    if (dDataSet.FindDataElement(tagFind)) {
        loaderData.windowCenter = std::stoi(dStringFilter.ToString(tagFind));
    }
    else {
        loaderData.windowCenter = 0;
    }

    tagFind.SetElementTag(0x0028, 0x1051);
    if (dDataSet.FindDataElement(tagFind)) {
        loaderData.windowWidth = std::stoi(dStringFilter.ToString(tagFind));
    }
    else {
        loaderData.windowWidth = 0;
    }

    int imagesCount = dImage.GetDimension(2);

    dImage.GetBuffer(buffer);
    ctImages.resize(imagesCount);
    images.resize(imagesCount);
    sinograms.resize(imagesCount);

    loaderData.ctImages = &ctImages;
    loaderData.images = &images;
    loaderData.sinograms = &sinograms;
    //MONOCHROME2

    gdcm::PhotometricInterpretation photometricInterpretation = dImage.GetPhotometricInterpretation();

    if (photometricInterpretation == gdcm::PhotometricInterpretation::MONOCHROME2) {
        loaderData.inverseNeeded = true;
    }
    else {
        loaderData.inverseNeeded = false;
    }

    gdcm::PixelFormat pixelFormat = dImage.GetPixelFormat();
    if (pixelFormat.GetScalarType() == gdcm::PixelFormat::UINT16) {
        loaderData.type = CV_16UC1;
    }
    loaderData.bytesAllocated = pixelFormat.GetBitsAllocated() / 8;
    loaderData.minValue = pixelFormat.GetMin();
    loaderData.maxValue = pixelFormat.GetMax();

    loaderData.isLittleEndian = (pixelFormat.GetBitsAllocated() - pixelFormat.GetHighBit() == 1) ? true : false;

    loaderData.width = dImage.GetDimension(0);
    loaderData.height = dImage.GetDimension(1);

    try {
        loaderData.slope = dImage.GetSlope();
        loaderData.intercept = dImage.GetIntercept();
    }
    catch(...) {
        loaderData.slope = 1.0;
        loaderData.intercept = 0.0;
    }

    loaderData.buffer = buffer;

    std::cout << "processing start" <<std::endl;

    //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(500, 500, CV_16UC1);
    //cv::ocl::GaussianBlur(*oclData, *oclData, cv::Size(5, 5), 5);

    cv::parallel_for_(cv::Range(0, imagesCount), ParallelLoader<u_int16_t>(loaderData));

    std::cout << "loading done" << std::endl;

    createSinogram(ctImages, sinogram);

    std::cout << "sinogram done" << std::endl;
    showImageWithNumber(0);

    return DICOM_ALL_OK;
}

void DicomReader::createSinogram(const std::vector<Mat*> & ctImages, cv::Mat ** sinogram) {
    SinogramData sinogramData;

    *sinogram = new cv::Mat(cv::Mat::zeros(ctImages.size(), ctImages.at(0)->cols, CV_32FC1));

    sinogramData.ctImages = ctImages;
    sinogramData.sinogram = *sinogram;

    cv::parallel_for_(cv::Range(0, ctImages.size()), SinogramMaker(sinogramData));
}

int DicomReader::readFile(const QString & dicomFile) {
    gdcm::ImageReader dIReader;

    dIReader.SetFileName(dicomFile.toStdString().c_str());

    if (!dIReader.Read()) {
        return DICOM_FILE_NOT_READABLE;
    }

    gdcm::File & dFile = dIReader.GetFile();
    gdcm::Image & dImage = dIReader.GetImage();

    readImage(dFile, dImage, _ctImages, _images, _sinograms, &_sinogram);

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
    //cv::imshow(WINDOW_DICOM_IMAGE, *(_images[imageNumber]));
    cv::imshow(WINDOW_RADON_2D, *(_sinograms[imageNumber]));
    //cv::imshow(WINDOW_RADON_2D, *_sinogram);
    cv::waitKey(1);
}

QImage DicomReader::dQImage() {
    return _dQImage;
}
