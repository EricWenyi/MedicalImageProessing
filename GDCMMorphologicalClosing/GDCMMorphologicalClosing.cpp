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

//区域增长的头文件
#include "itkConnectedThresholdImageFilter.h"

//腐蚀的头文件
#include "itkBinaryErodeImageFilter.h"

//膨胀的头文件
#include "itkBinaryDilateImageFilter.h"

//领域结构的头文件
#include "itkBinaryBallStructuringElement.h"

int main( int argc, char* argv[] ){
	if( argc < 3 ){
		std::cerr << "Usage: " << argv[0] << " InputImageFile OutputImageFile" << std::endl;
		return EXIT_FAILURE;
    }

	//设定图像类型
	typedef itk::Image< signed short, 3 > ImageType3D;
	typedef itk::Image< signed short, 2 > ImageType2D;

	//设定输入图像的类型
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//设定读取器类型及设定读取的图像类型和图像地址
	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetImageIO( gdcmIO );
	reader->SetFileName( argv[1] );

	try{
		reader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer image3D = reader->GetOutput();

	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType iterator( image3D, image3D->GetLargestPossibleRegion() );
	iterator.SetFirstDirection(0);
	iterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( image3D->GetOrigin()[2] );
	joinSeries->SetSpacing( image3D->GetSpacing()[2] );

	for( iterator.GoToBegin(); !iterator.IsAtEnd(); iterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = iterator.GetIndex();
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = iterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = iterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( reader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		typedef itk::BinaryBallStructuringElement< signed short, 2 > StructuringElementType;
		StructuringElementType structuringElement;
		structuringElement.SetRadius( 1 );
		structuringElement.CreateStructuringElement();
		
		typedef itk::BinaryErodeImageFilter< ImageType2D, ImageType2D, StructuringElementType > ErodeFilterType;
		ErodeFilterType::Pointer binaryErode = ErodeFilterType::New();
		typedef itk::BinaryDilateImageFilter< ImageType2D, ImageType2D, StructuringElementType > DilateFilterType;
		DilateFilterType::Pointer binaryDilate = DilateFilterType::New();

		binaryDilate->SetKernel( structuringElement );
		binaryErode->SetKernel( structuringElement );
		 
		binaryDilate->SetDilateValue( 0 );
		binaryErode->SetErodeValue( 0 );
		
		binaryDilate->SetInput( extractor->GetOutput() );
		binaryErode->SetInput( binaryErode->GetOutput() );

		try{
			binaryDilate->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: binaryDilate->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		try{
			binaryErode->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: binaryErode->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		joinSeries->PushBackInput( binaryDilate->GetOutput() );
	}

	try{
		joinSeries->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries->Update(); caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::ImageFileWriter<ImageType3D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( argv[2] );
	writer->SetInput( joinSeries->GetOutput() );

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}