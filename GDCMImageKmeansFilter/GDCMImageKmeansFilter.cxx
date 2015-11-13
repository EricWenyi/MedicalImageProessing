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

//itk向量的头文件
#include "itkVector.h"

//采样集的头文件
#include "itkListSample.h"

//kd树生成器的头文件
#include "itkWeightedCentroidKdTreeGenerator.h"

//kd树估计值生成器的头文件
#include "itkKdTreeBasedKmeansEstimator.h"

//判断规则的头文件
#include "itkMinimumDecisionRule.h"

//采样分类器过滤器的头文件
#include "itkSampleClassifierFilter.h"

int main( int argc, char * argv[] ){

	//判断参数个数，若个数不够提示需要的参数
	if( argc < 3 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "originImageFile outputImageFile" << std::endl;
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
	ReaderType::Pointer originReader = ReaderType::New();

	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );

	//更新读取器，并捕获异常

	try{
		originReader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject originReader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	//设定输入区域大小
	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::RegionType originRegion3D = originImage3D->GetLargestPossibleRegion();
	
	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType originIterator( originImage3D, originRegion3D );
	originIterator.SetFirstDirection(0);
	originIterator.SetSecondDirection(1);
	
	//设置图片组合器
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(originImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(originImage3D->GetSpacing()[2]);
	
	for( originIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice() ){
		//获取切片序号
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();

		//获取每张切片的大小，并设置每张切片的Z轴为0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;

		//设定每张切片的大小和序号
		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );
		
		//指定过滤器类型
		ExtractFilterType::Pointer originExtractFilter = ExtractFilterType::New();
		originExtractFilter->SetExtractionRegion( originSliceRegion );
		
		//到位并开启过滤器，修改方向坍塌策略为子矩阵
		originExtractFilter->InPlaceOn();
		originExtractFilter->SetDirectionCollapseToSubmatrix();
		originExtractFilter->SetInput( originReader->GetOutput() );
		
		//更新过滤器，并捕获异常
		try{
			originExtractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//设定图像区域迭代器，用于读写像素点值
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType origin( originExtractFilter->GetOutput(), originExtractFilter->GetOutput()->GetRequestedRegion() );

		//设定观测向量和样本集，每个观测向量的两个维度分别对应中值滤波图像和原始图像，样本集有序的储存观测向量
		typedef itk::Vector< double , 1 > MeasurementVectorType;
		typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
		SampleType::Pointer sample = SampleType::New();
		sample->SetMeasurementVectorSize( 1 );
		MeasurementVectorType mv;
		for ( origin.GoToBegin(); !origin.IsAtEnd(); origin++ ){
			mv[0] = ( double ) origin.Get();
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
		EstimatorType::ParametersType initialMeans( 2 );
		initialMeans[0] = 0.0;
		initialMeans[1] = 0.0;
		estimator->SetParameters( initialMeans );
		estimator->SetKdTree( treeGenerator->GetOutput() );
		estimator->SetCentroidPositionChangesThreshold(0.0);
		estimator->StartOptimization();

		//从estimator获取最终的各类中心的均值并输出
		EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();
		for ( unsigned int i = 0; i < 2; ++i ){
			std::cout << "cluster[" << i << "] " << std::endl;
			std::cout << "	estimated mean : " << estimatedMeans[i] << std::endl;
		}

		//类成员判断规则，这里设定的是最小值判断规则，离谁近归谁
		typedef itk::Statistics::MinimumDecisionRule DecisionRuleType;
		DecisionRuleType::Pointer decisionRule = DecisionRuleType::New();

		//设定分类器的输入和类成员判定规则
		typedef itk::Statistics::SampleClassifierFilter< SampleType > ClassifierType;
		ClassifierType::Pointer classifier = ClassifierType::New();
		classifier->SetDecisionRule( decisionRule );
		classifier->SetInput( sample );
		classifier->SetNumberOfClasses( 2 );

		//类标签的设定
		typedef ClassifierType::ClassLabelVectorObjectType ClassLabelVectorObjectType;
		typedef ClassifierType::ClassLabelVectorType ClassLabelVectorType;
		typedef ClassifierType::ClassLabelType ClassLabelType;
		ClassLabelVectorObjectType::Pointer classLabelsObject = ClassLabelVectorObjectType::New();
		ClassLabelVectorType &classLabelsVector = classLabelsObject->Get();
		classLabelsVector.push_back( 0 );
		classLabelsVector.push_back( 1 );

		//为分类器设定类标签
		classifier->SetClassLabels( classLabelsObject );

		//将估值器得到的最终的各类中心的均值传入隶属度函数
		typedef ClassifierType::MembershipFunctionVectorObjectType MembershipFunctionVectorObjectType;
		typedef ClassifierType::MembershipFunctionVectorType MembershipFunctionVectorType;
		MembershipFunctionVectorObjectType::Pointer membershipFunctionVectorObject = MembershipFunctionVectorObjectType::New();
		MembershipFunctionVectorType& membershipFunctionVector = membershipFunctionVectorObject->Get();
		int index = 0;
		for ( unsigned int i = 0; i < 2; i++ ){
			typedef itk::Statistics::DistanceToCentroidMembershipFunction< MeasurementVectorType > MembershipFunctionType;
			MembershipFunctionType::Pointer membershipFunction = MembershipFunctionType::New();
			MembershipFunctionType::CentroidType centroid( sample->GetMeasurementVectorSize() );
			for ( unsigned int j = 0; j < sample->GetMeasurementVectorSize(); j++ ){
				centroid[j] = estimatedMeans[index++];
			}
			membershipFunction->SetCentroid( centroid );
			membershipFunctionVector.push_back( membershipFunction.GetPointer() );
		}

		//为分类器设定隶属度函数，更新分类器，并捕获异常
		classifier->SetMembershipFunctions( membershipFunctionVectorObject );
		try{
			classifier->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject classifier->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		//将分类器输出的隶属样本中的类标签提取出来，并使用图像区域迭代器写入原始图像相应位置
		const ClassifierType::MembershipSampleType *membershipSample = classifier->GetOutput();
		ClassifierType::MembershipSampleType::ConstIterator membershipSampleIterator = membershipSample->Begin();
		for ( origin.GoToBegin(); !origin.IsAtEnd(); origin++ ){
			origin.Set( membershipSampleIterator.GetClassLabel() );
			membershipSampleIterator++;
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
