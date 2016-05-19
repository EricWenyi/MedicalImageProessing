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

//采样集的头文件
#include "itkListSample.h"

//kd树生成器的头文件
#include "itkWeightedCentroidKdTreeGenerator.h"

//kd树估计值生成器的头文件
#include "KdTreeBasedKmeansEstimator.h"

int main( int argc, char * argv[] ){

	//判断参数个数，若个数不够提示需要的参数
	if( argc < 6 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "medianImageFile originImageFile outputImageFile indexBetweenUpperAndMiddle indexBetweenMiddleAndLower" << std::endl;
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
	ReaderType::Pointer medianReader = ReaderType::New();
	ReaderType::Pointer originReader = ReaderType::New();

	medianReader->SetImageIO( gdcmIO );
	medianReader->SetFileName( argv[1] );

	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[2] );

	//更新读取器，并捕获异常
	try{
		medianReader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject medianReader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	try{
		originReader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject originReader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	//设定输入区域大小
	ImageType3D::ConstPointer medianImage3D = medianReader->GetOutput();
	ImageType3D::RegionType medianRegion3D = medianImage3D->GetLargestPossibleRegion();

	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::RegionType originRegion3D = originImage3D->GetLargestPossibleRegion();
	
	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType medianIterator( medianImage3D, medianRegion3D );
	medianIterator.SetFirstDirection( 0 );
	medianIterator.SetSecondDirection( 1 );

	SliceIteratorType originIterator( originImage3D, originRegion3D );
	originIterator.SetFirstDirection( 0 );
	originIterator.SetSecondDirection( 1 );
	
	//设置图片组合器
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( originImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	//获取上区与中区，中区与下区的临界切片序号
	double R;
 
	for( medianIterator.GoToBegin(), originIterator.GoToBegin(); !medianIterator.IsAtEnd(); medianIterator.NextSlice(), originIterator.NextSlice() ){
		//获取切片序号
		ImageType3D::IndexType medianSliceIndex = medianIterator.GetIndex();
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();

		//如果位于中区，则设定R=4.0，否则为8.0
		if ( medianSliceIndex[2] >= atoi(argv[4]) - 1 && medianSliceIndex[2] < atoi(argv[5])){
			R = 4.0f;
		} else {
			R = 8.0f;
		}

		//获取每张切片的大小，并设置每张切片的Z轴为0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType medianSliceRegion = medianIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType medianSliceSize = medianSliceRegion.GetSize();
		medianSliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;

		//设定每张切片的大小和序号
		medianSliceRegion.SetSize( medianSliceSize );
		medianSliceRegion.SetIndex( medianSliceIndex );

		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );
		
		//指定过滤器类型
		ExtractFilterType::Pointer medianExtractFilter = ExtractFilterType::New();
		medianExtractFilter->SetExtractionRegion( medianSliceRegion );

		ExtractFilterType::Pointer originExtractFilter = ExtractFilterType::New();
		originExtractFilter->SetExtractionRegion( originSliceRegion );
		
		//到位并开启过滤器，修改方向坍塌策略为子矩阵
		medianExtractFilter->InPlaceOn();
		medianExtractFilter->SetDirectionCollapseToSubmatrix();
		medianExtractFilter->SetInput( medianReader->GetOutput() );
		
		originExtractFilter->InPlaceOn();
		originExtractFilter->SetDirectionCollapseToSubmatrix();
		originExtractFilter->SetInput( originReader->GetOutput() );
		
		//更新过滤器，并捕获异常
		try{
			medianExtractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: medianExtractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		try{
			originExtractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//设定图像区域迭代器，用于读写像素点值
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType median( medianExtractFilter->GetOutput(), medianExtractFilter->GetOutput()->GetRequestedRegion() );
		IteratorType origin( originExtractFilter->GetOutput(), originExtractFilter->GetOutput()->GetRequestedRegion() );

		//设定观测向量和样本集，每个观测向量的两个维度分别对应中值滤波图像和原始图像，样本集有序的储存观测向量
		typedef itk::Vector< double , 2 > MeasurementVectorType;
		typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
		SampleType::Pointer sample = SampleType::New();
		sample->SetMeasurementVectorSize( 2 );
		MeasurementVectorType mv;
		for ( median.GoToBegin(), origin.GoToBegin(); !median.IsAtEnd(); median++, origin++ ){
			mv[0] = ( double ) median.Get();
			mv[1] = ( double ) origin.Get();
			sample->PushBack( mv );
		}
		
		//将设定的样本集传入kd树，并设置cell的大小
		typedef itk::Statistics::WeightedCentroidKdTreeGenerator< SampleType > TreeGeneratorType;
		TreeGeneratorType::Pointer treeGenerator = TreeGeneratorType::New();
		treeGenerator->SetSample( sample );
		treeGenerator->SetBucketSize( 16 );
		treeGenerator->Update();

		//将kd树传入kmeans估值器，设定各初始类中心的均值，以及算法停止的阈值设定
		typedef TreeGeneratorType::KdTreeType TreeType;
		typedef itk::Statistics::KdTreeBasedKmeansEstimator< TreeType > EstimatorType;
		EstimatorType::Pointer estimator = EstimatorType::New();
		EstimatorType::ParametersType initialMeans( 4 );
		initialMeans[0] = 0.0;
		initialMeans[1] = 0.0;
		initialMeans[2] = 0.0;
		initialMeans[3] = 0.0;
		estimator->SetParameters( initialMeans );
		estimator->SetKdTree( treeGenerator->GetOutput() );
		estimator->SetCentroidPositionChangesThreshold( 0.0 );
		estimator->StartOptimization( R );

		//从estimator获取最终的各类中心的均值并输出
		EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();

		std::cout<<"slice No."<<medianSliceIndex[2]+1<<std::endl;

		for ( unsigned int i = 0; i < 4; ++i ){
			std::cout << "cluster[" << i << "] " << std::endl;
			std::cout << "	estimated mean : " << estimatedMeans[i] << std::endl;
		}

		//设定欧氏距离度量以及最终的背景点中心
		typedef itk::Array< double > ArrayType;
		typedef itk::Statistics::EuclideanDistanceMetric< ArrayType > DistanceMetricType;
		DistanceMetricType::Pointer distanceMetric = DistanceMetricType::New();
		ArrayType means0( 2 );
		means0[0] = estimatedMeans[0];
		means0[1] = estimatedMeans[1];
		ArrayType means1( 2 );
		means1[0] = estimatedMeans[2];
		means1[1] = estimatedMeans[3];

		//分配像素点至background或nodule
		for ( median.GoToBegin(), origin.GoToBegin(); !median.IsAtEnd(); median++, origin++ ){
			ArrayType pixelArray( 2 );
			pixelArray[0] = ( double ) median.Get();
			pixelArray[1] = ( double ) origin.Get();
			double distance0 = distanceMetric->Evaluate( pixelArray, means0 );
			double distance1 = distanceMetric->Evaluate( pixelArray, means1 );
			if ( distance0 / distance1 > R ){
				origin.Set( 1 );
			} else {
				origin.Set( 0 );
			}
		}

		//设定图片组合器的输入
		joinSeries->PushBackInput( originExtractFilter->GetOutput() );
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
	writer->SetFileName( argv[3] );	
	
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
