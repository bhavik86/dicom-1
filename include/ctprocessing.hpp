#ifndef CTPROCESSING_H
#define CTPROCESSING_H

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/ocl/ocl.hpp"

#define RADON_DEGREE_RANGE 180

typedef struct _CtData {
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
}CtData;

template <typename T>
class CtProcessing : public cv::ParallelLoopBody {
private:
    CtData _ctData;

    static inline cv::Mat radon(const cv::Mat & ctImage, const int & theta,
                         const int & width, const int & height,
                         const int & widthPad, const int & heightPad) {

        cv::Mat imgPad(cv::Mat::zeros(height + heightPad, width + widthPad, CV_16UC1));
        cv::Mat imgPad8(cv::Mat::zeros(height + heightPad, width + widthPad, CV_8UC1));
        cv::Mat imgPadRotated(cv::Mat::zeros(height + heightPad, width + widthPad, CV_8UC1));

        cv::Mat sinogram(cv::Mat::zeros(height + heightPad, 0, CV_32FC1));

        ctImage.copyTo(imgPad(cv::Rect(ceil(widthPad / 2.0), ceil(heightPad / 2.0), width, height)));

        imgPad.convertTo(imgPad8, CV_8UC1, 1/256.0);
        cv::Point2i center = cv::Point2i((width + widthPad) / 2, (height + heightPad) / 2);

        cv::Mat rotationMatrix;
        cv::Mat colSum(cv::Mat::zeros(imgPad.cols, 1, CV_32FC1));

        cv::Size size(imgPad.rows, imgPad.cols);

        cv::ocl::oclMat imgPadOcl(imgPad8);
        cv::ocl::oclMat imgPadRotatedOcl(imgPadRotated);

        for (int angle = 0; angle != theta; angle ++) {
            rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
            //cv::warpAffine(imgPad, imgPadRotated, rotationMatrix, size, cv::INTER_LINEAR | cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT);

            cv::ocl::warpAffine(imgPadOcl, imgPadRotatedOcl, rotationMatrix, size, cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);

            imgPadRotatedOcl.download(imgPadRotated);
            cv::reduce(imgPadRotated, colSum, 0, CV_REDUCE_SUM, CV_32FC1);
            sinogram.push_back(colSum);
        }

        return sinogram;
    }

    static inline cv::Mat backproject(const cv::Mat & sinogram,
                         const int & width, const int & height) {
        cv::Mat backproj(cv::Mat::zeros(width, height, CV_16UC1));

        for (int angle = 0; angle != sinogram.rows; angle ++) {

        }

        return backproject;
    }

public:
    CtProcessing(CtData & ctData) : _ctData(ctData) {
        _ctData.offset = _ctData.bytesAllocated * _ctData.width * _ctData.height;
    }

    virtual void operator ()(const cv::Range & r) const {
        int width = _ctData.width / 1;
        int height = _ctData.height / 1;

        int pad = std::max(width, height);

        int widthPad = std::ceil((pad - width) / 2.0);
        int heightPad = std::ceil((pad - height) / 2.0);

        for (register int i = r.start; i < r.end; i ++) {

            cv::Mat * data = new cv::Mat(_ctData.width, _ctData.height, _ctData.type);
            T pixel;

            char * bufferImageI = _ctData.buffer + _ctData.offset * i;

            for (int x = 0; x < _ctData.width; x ++) {
                for (int y = 0; y < _ctData.height; y ++) {
                    pixel ^= pixel;

                    if (_ctData.isLittleEndian) {
                        for (int k = 0; k < _ctData.bytesAllocated; k ++) {
                            pixel |= (T)*(bufferImageI + k) << (8 * k);
                        }
                    }
                    else {
                        for (int k = _ctData.bytesAllocated - 1; k > 0 ; k --) {
                            pixel |= (T)*(bufferImageI + k) << (8 * (_ctData.bytesAllocated - k + 1));
                        }
                    }

                    bufferImageI += _ctData.bytesAllocated;


                    /* similar as http://code.google.com/p/pydicom/source/browse/source/dicom/contrib/pydicom_PIL.py*/
                    pixel = _ctData.slope * pixel + _ctData.intercept;

                    if (pixel <= (_ctData.windowCenter - 0.5 - (_ctData.windowWidth - 1) / 2.0)) {
                        pixel = (T)_ctData.minValue;
                    }
                    else if (pixel > (_ctData.windowCenter - 0.5 + (_ctData.windowWidth - 1) / 2.0)) {
                        pixel = (T)_ctData.maxValue;
                    }
                    else {
                        pixel = ((pixel - _ctData.windowCenter + 0.5) / (_ctData.windowWidth - 1) + 0.5) *
                                (_ctData.maxValue - _ctData.minValue);
                    }

                    //MONOCHROME2 - high value -> brighter, -1 high -> blacker
                    if (_ctData.inverseNeeded) {
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

            _ctData.images->at(i) = data;

            //cv::ocl::oclMat * oclData = new cv::ocl::oclMat(*data8);

            cv::Mat * contourImage = new cv::Mat(cv::Mat::zeros(height, width, CV_8UC1));

            cv::GaussianBlur(data8, *contourImage, cv::Size(5, 5), 5);

            //---radon---//

            cv::Mat * sinogram = new cv::Mat(radon(*data, RADON_DEGREE_RANGE, width, height, widthPad, heightPad));
            delete data;

            _ctData.images->at(i) = contourImage;

            _ctData.sinograms->at(i) = sinogram;

            //cv::medianBlur(*data8, *contourImage, 5);
            //cv::Canny(*contourImage, *contourImage, CANNY_LOWER, 3 * CANNY_LOWER, 3);

            //cv::threshold(*contourImage, *contourImage, 250, 255, CV_THRESH_OTSU);
            //cv::dilate(*contourImage, *contourImage, 19);

            //cv::Mat * steger = new cv::Mat();
            //stegerEdges(*contourImage, *steger, 5, 0, 10.0);

            //cv::adaptiveThreshold(*contourImage, *contourImage, 200, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 3, 1);
    /*
            std::vector<std::vector<cv::Point> > contours;
            std::vector<cv::Vec4i> hierarchy;

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

            _ctData.ctImages->at(i) = contourImage;
        }
    }
};

#endif // CTPROCESSING_H
