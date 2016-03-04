#include "itkGDCMImageIO.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkOpenCVImageBridge.h"



int main( int argc, char* argv[] ){
	if( argc < 2 ){
		std::cerr << "Usage: " << argv[0] << " OutputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	typedef unsigned char PixelType2D;
	typedef itk::Image< PixelType2D, 2 > ImageType2D;

	
	using namespace cv;
	

	//int m[25]={255,255,0,0,0,255,255,0,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,255,255};
	float m[25]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0};
	Mat drawing(5,5,CV_8UC1,m);


	ImageType2D::Pointer itkDrawing;
	try{
			itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( drawing );
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVMatToITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}




	typedef itk::ImageFileWriter<ImageType2D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( argv[1] );
	writer->SetInput(itkDrawing);

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}