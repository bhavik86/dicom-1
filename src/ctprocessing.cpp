#include "ctprocessing.h"

CtProcessing::CtProcessing(CtData & ctData) : _ctData(ctData) {
    _ctData.offset = _ctData.bytesAllocated * _ctData.width * _ctData.height;
}


void CtProcessing<T>::operator ()(const cv::Range & r) const {


}

