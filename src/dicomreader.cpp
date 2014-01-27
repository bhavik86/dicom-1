#include <QtCore/QDebug>

#include "dicomreader.h"

#include "gdcmReader.h"
#include "gdcmImageReader.h"
#include "gdcmSmartPointer.h"
#include "gdcmDataSetHelper.h"
#include "gdcmStringFilter.h"
#include "gdcmDICOMDIR.h"

DicomReader::DicomReader(QObject * parent) :
    QObject(parent) {
}

DicomReader::DicomReader(const QString & dicomFile, QObject * parent) :
    QObject(parent) {
    readFile(dicomFile);
}

QImage DicomReader::convertToFormat_RGB888(gdcm::Image const & gimage, char * buffer, int & error) {

  QImage image;
  const unsigned int* dimension = gimage.GetDimensions();

  unsigned int dimX = dimension[0];
  unsigned int dimY = dimension[1];

  gimage.GetBuffer(buffer);

  error = 0;

  std::cout << gimage.GetPhotometricInterpretation() << std::endl;

  // Let's start with the easy case:
  if( gimage.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::RGB )
    {
    if( gimage.GetPixelFormat() != gdcm::PixelFormat::UINT8 )
      {
        error = 1;
      return QImage();
      }
    unsigned char *ubuffer = (unsigned char*)buffer;
    // QImage::Format_RGB888  13  The image is stored using a 24-bit RGB format (8-8-8).
        return QImage((unsigned char *)ubuffer, dimX, dimY, 3*dimX, QImage::Format_RGB888);
    }
  else if(gimage.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME2 )
    {
    if( gimage.GetPixelFormat() == gdcm::PixelFormat::UINT8 )
      {
      // We need to copy each individual 8bits into R / G and B:
      unsigned char *ubuffer = new unsigned char[dimX*dimY*3];
      unsigned char *pubuffer = ubuffer;
      for(unsigned int i = 0; i < dimX*dimY; i++)
        {
        *pubuffer++ = *buffer;
        *pubuffer++ = *buffer;
        *pubuffer++ = *buffer++;
        }

        return QImage(ubuffer, dimX, dimY, QImage::Format_RGB888);
      }
    else if (gimage.GetPixelFormat() == gdcm::PixelFormat::UINT16) {
      // We need to copy each individual 16bits into R / G and B (truncate value)
      short *buffer16 = (short*)buffer;
      unsigned char *ubuffer = new unsigned char[dimX*dimY*3];
      unsigned char *pubuffer = ubuffer;
      for(unsigned int i = 0; i < dimX*dimY; i++) {
        // Scalar Range of gdcmData/012345.002.050.dcm is [0,192], we could simply do:
        // *pubuffer++ = *buffer16;
        // *pubuffer++ = *buffer16;
        // *pubuffer++ = *buffer16;
        // instead do it right:
        *pubuffer++ = (unsigned char)std::min(255, (32768 + *buffer16) / 255);
        *pubuffer++ = (unsigned char)std::min(255, (32768 + *buffer16) / 255);
        *pubuffer++ = (unsigned char)std::min(255, (32768 + *buffer16) / 255);
        buffer16++;
        }

      return QImage(ubuffer, dimX, dimY, QImage::Format_RGB888);
      }

    else {
      std::cerr << "Pixel Format is: " << gimage.GetPixelFormat() << std::endl;
      error = 1;
    return QImage();
    }
    }
  else
    {
    std::cerr << "Unhandled PhotometricInterpretation: " << gimage.GetPhotometricInterpretation() << std::endl;
    error = 1;
  return QImage();
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


    std::vector<char> vbuffer;
    vbuffer.resize(dImage.GetBufferLength());
    char *buffer = &vbuffer[0];
    int error = 0;

    std::cout << dImage.GetColumns() << " " << dImage.GetNumberOfDimensions() << std::endl;

    _dQImage = convertToFormat_RGB888(dImage, buffer, error);

  /*  if (dHeader.FindDataElement(gdcm::Tag(0x2, 0x13))) {
        gdcm::DataElement & dE = dHeader.GetDataElement(gdcm::Tag(0x2, 0x13));
    }
*/
    return DICOM_ALL_OK;
}


QImage DicomReader::dQImage() {
    return _dQImage;
}
