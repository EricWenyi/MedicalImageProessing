//文件读写的头文件
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//GDCM类型文件读写的头文件
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"

//切片迭代器的头文件
#include "itkImageSliceConstIteratorWithIndex.h"

//提取切片的头文件
#include "itkExtractImageFilter.h"

//切片文件集合成3D图像的头文件
#include "itkJoinseriesImageFilter.h"

//mesh的头文件
#include "itkBinaryMask3DMeshSource.h"

int main( int argc, char * argv[] ){

	//判断参数个数，若个数不够提示需要的参数
	if( argc < 2 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "inputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//指定输入输出的图像类型
	typedef itk::Image< signed short, 3 > ImageType3D;

	//设定输入图像的类型
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//设定读取器类型及设定读取的图像类型和图像地址
	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer reader = ReaderType::New();

	reader->SetImageIO( gdcmIO );
	reader->SetFileName( argv[1] );

	//更新读取器，并捕获异常
	try{
		reader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject reader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	typedef itk::Mesh< double > MeshType;
	typedef itk::BinaryMask3DMeshSource< ImageType3D, MeshType > MeshSourceType;
	MeshSourceType::Pointer meshSource = MeshSourceType::New();
	meshSource->SetObjectValue(255);
	meshSource->SetInput( reader->GetOutput() );

	try{
		meshSource->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject meshSource->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	std::cout << "Nodes = " << meshSource->GetNumberOfNodes() << std::endl;
	std::cout << "Cells = " << meshSource->GetNumberOfCells() << std::endl;

	return EXIT_SUCCESS;
}