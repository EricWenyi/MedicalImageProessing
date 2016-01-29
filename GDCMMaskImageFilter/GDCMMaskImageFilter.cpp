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

int main( int argc, char* argv[] ){
	if( argc < 4 ){
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

	ReaderType::Pointer originReader = ReaderType::New();
	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );

	ReaderType::Pointer maskReader = ReaderType::New();
	maskReader->SetImageIO( gdcmIO );
	maskReader->SetFileName( argv[2] );

	try{
		originReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: originReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	try{
		maskReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: maskReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::ConstPointer maskImage3D = maskReader->GetOutput();

	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType originIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	originIterator.SetFirstDirection(0);
	originIterator.SetSecondDirection(1);

	SliceIteratorType maskIterator( maskImage3D, maskImage3D->GetLargestPossibleRegion() );
	maskIterator.SetFirstDirection(0);
	maskIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( originImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	for( originIterator.GoToBegin(), maskIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice(), maskIterator.NextSlice() ){
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();
		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;
		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );

		ImageType3D::IndexType maskSliceIndex = maskIterator.GetIndex();
		ExtractFilterType::InputImageRegionType maskSliceRegion = maskIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType maskSliceSize = maskSliceRegion.GetSize();
		maskSliceSize[2] = 0;
		maskSliceRegion.SetSize( maskSliceSize );
		maskSliceRegion.SetIndex( maskSliceIndex );
		
		ExtractFilterType::Pointer originExtractor = ExtractFilterType::New();
		originExtractor->SetExtractionRegion( originSliceRegion );
		originExtractor->InPlaceOn();
		originExtractor->SetDirectionCollapseToSubmatrix();
		originExtractor->SetInput( originReader->GetOutput() );

		ExtractFilterType::Pointer maskExtractor = ExtractFilterType::New();
		maskExtractor->SetExtractionRegion( maskSliceRegion );
		maskExtractor->InPlaceOn();
		maskExtractor->SetDirectionCollapseToSubmatrix();
		maskExtractor->SetInput( maskReader->GetOutput() );

		try{
			originExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		try{
			maskExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: maskExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//设定图像区域迭代器，用于读写像素点值
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType origin( originExtractor->GetOutput(), originExtractor->GetOutput()->GetRequestedRegion() );
		IteratorType mask( maskExtractor->GetOutput(), maskExtractor->GetOutput()->GetRequestedRegion() );

		for ( origin.GoToBegin(), mask.GoToBegin(); !mask.IsAtEnd(); origin++, mask++ ){
			if( mask.Get() == 0 ){
				origin.Set(0);
			}
		}

		joinSeries->PushBackInput( originExtractor->GetOutput() );
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
	writer->SetFileName( argv[3] );
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