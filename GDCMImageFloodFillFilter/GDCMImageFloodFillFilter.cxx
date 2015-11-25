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

//填坑的头文件
#include "itkVotingBinaryIterativeHoleFillingImageFilter.h"

int main( int argc, char * argv[] ){

	//判断参数个数，若个数不够提示需要的参数
	if( argc < 3 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "kmeansImageFile outputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//指定输入输出的图像类型
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

	//更新读取器，并捕获异常

	try{
		reader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject reader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	//设定输入区域大小
	ImageType3D::ConstPointer image3D = reader->GetOutput();
	ImageType3D::RegionType region3D = image3D->GetLargestPossibleRegion();
	
	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType iterator( image3D, region3D );
	iterator.SetFirstDirection( 0 );
	iterator.SetSecondDirection( 1 );
	
	//设置图片组合器
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( image3D->GetOrigin()[2] );
	joinSeries->SetSpacing( image3D->GetSpacing()[2] );
	
	for( iterator.GoToBegin(); !iterator.IsAtEnd(); iterator.NextSlice() ){
		//获取切片序号
		ImageType3D::IndexType sliceIndex = iterator.GetIndex();
		std::cout<<sliceIndex[2]+1<<std::endl;

		//获取每张切片的大小，并设置每张切片的Z轴为0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType sliceRegion = iterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = sliceRegion.GetSize();
		sliceSize[2] = 0;

		//设定每张切片的大小和序号
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		//指定过滤器类型
		ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
		extractFilter->SetExtractionRegion( sliceRegion );
		
		//到位并开启过滤器，修改方向坍塌策略为子矩阵
		extractFilter->InPlaceOn();
		extractFilter->SetDirectionCollapseToSubmatrix();
		extractFilter->SetInput( reader->GetOutput() );
		
		//更新过滤器，并捕获异常
		try{
			extractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//设定填坑过滤器的前景，背景点
		typedef itk::VotingBinaryIterativeHoleFillingImageFilter< ImageType2D > BinaryIterativeHoleFillingImageFilter;
		BinaryIterativeHoleFillingImageFilter::Pointer holeFillingImageFilter = BinaryIterativeHoleFillingImageFilter::New();
		ImageType2D::SizeType indexRadius;
		indexRadius.Fill( 2 );//默认半径全为1，设置为大于3时会丢失边缘特征
		holeFillingImageFilter->SetRadius( indexRadius );
		holeFillingImageFilter->SetBackgroundValue( 0 );
		holeFillingImageFilter->SetForegroundValue( 1 );
		holeFillingImageFilter->SetMajorityThreshold( 1 );//默认阈值为1,经测试，设置其他的会丢失边缘特征，变得方块化
		holeFillingImageFilter->SetMaximumNumberOfIterations( 20 );//默认是10
		holeFillingImageFilter->SetInput( extractFilter->GetOutput() );

		//更新填坑过滤器，并捕获异常
		try{
			holeFillingImageFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: holeFillingFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//设定图片组合器的输入
		joinSeries->PushBackInput( holeFillingImageFilter->GetOutput() );
	}
	
	//更新图片组合器，并捕获异常
	try{
		joinSeries->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject joinSeries->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	//设定写入器输入
	typedef itk::ImageFileWriter< ImageType3D > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput( joinSeries->GetOutput() );
	writer->SetFileName( argv[2] );	
	
	//更新写入器，并捕获异常
	try{
		writer->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject writer->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
