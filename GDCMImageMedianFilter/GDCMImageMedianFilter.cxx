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

//中值滤波器的头文件
#include "itkMedianImageFilter.h"

int main( int argc, char * argv[] ){

	//判断参数个数，若个数不够提示需要的参数
	if( argc < 3 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "inputImageFile outputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//指定输入输出像素点类型
	typedef signed short PixelType;

	//指定输入输出的图像类型
	typedef itk::Image< PixelType, 3 > ImageType3D;
	typedef itk::Image< PixelType, 2 > ImageType2D;

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
	iterator.SetFirstDirection(0);
	iterator.SetSecondDirection(1);

	//设置图片组合器
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(image3D->GetOrigin()[2]);
	joinSeries->SetSpacing(image3D->GetSpacing()[2]);
	
	for( iterator.GoToBegin(); !iterator.IsAtEnd(); iterator.NextSlice() ){
		//获取切片序号
		ImageType3D::IndexType sliceIndex = iterator.GetIndex();

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

		//设定原图的中值滤波器的输入输出图像类型
		typedef itk::MedianImageFilter< ImageType2D, ImageType2D >  MedianFilterType;
		MedianFilterType::Pointer medianFilter = MedianFilterType::New();

		//设定水平半径与竖直半径
		ImageType2D::SizeType indexRadius;
		indexRadius.Fill(2);
		medianFilter->SetRadius( indexRadius );

		//设定中值滤波器输入
		medianFilter->SetInput( extractFilter->GetOutput() );
		
		//更新中值滤波器，并捕获异常
		try{
			medianFilter->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject medianFilter->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}
		
		joinSeries->PushBackInput( medianFilter->GetOutput() );
	}
	
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