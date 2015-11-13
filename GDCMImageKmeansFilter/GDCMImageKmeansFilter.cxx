//�ļ���д��ͷ�ļ�
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//GDCM�����ļ���д��ͷ�ļ�
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"

//��Ƭ��������ͷ�ļ�
#include "itkImageSliceConstIteratorWithIndex.h"

//��ȡ��Ƭ��ͷ�ļ�
#include "itkExtractImageFilter.h"

//��Ƭ�ļ����ϳ�3Dͼ���ͷ�ļ�
#include "itkJoinseriesImageFilter.h"

//itk������ͷ�ļ�
#include "itkVector.h"

//��������ͷ�ļ�
#include "itkListSample.h"

//kd����������ͷ�ļ�
#include "itkWeightedCentroidKdTreeGenerator.h"

//kd������ֵ��������ͷ�ļ�
#include "itkKdTreeBasedKmeansEstimator.h"

//�жϹ����ͷ�ļ�
#include "itkMinimumDecisionRule.h"

//������������������ͷ�ļ�
#include "itkSampleClassifierFilter.h"

int main( int argc, char * argv[] ){

	//�жϲ���������������������ʾ��Ҫ�Ĳ���
	if( argc < 3 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "originImageFile outputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//ָ������������ص�����
	typedef signed short PixelType;

	//ָ�����������ͼ������
	typedef itk::Image< PixelType, 3 > ImageType3D;
	typedef itk::Image< PixelType, 2 > ImageType2D;

	//�趨����ͼ�������
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//�趨��ȡ�����ͼ��趨��ȡ��ͼ�����ͺ�ͼ���ַ
	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer originReader = ReaderType::New();

	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );

	//���¶�ȡ�����������쳣

	try{
		originReader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject originReader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	//�趨���������С
	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::RegionType originRegion3D = originImage3D->GetLargestPossibleRegion();
	
	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType originIterator( originImage3D, originRegion3D );
	originIterator.SetFirstDirection(0);
	originIterator.SetSecondDirection(1);
	
	//����ͼƬ�����
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(originImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(originImage3D->GetSpacing()[2]);
	
	for( originIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice() ){
		//��ȡ��Ƭ���
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();

		//��ȡÿ����Ƭ�Ĵ�С��������ÿ����Ƭ��Z��Ϊ0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;

		//�趨ÿ����Ƭ�Ĵ�С�����
		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );
		
		//ָ������������
		ExtractFilterType::Pointer originExtractFilter = ExtractFilterType::New();
		originExtractFilter->SetExtractionRegion( originSliceRegion );
		
		//��λ���������������޸ķ���̮������Ϊ�Ӿ���
		originExtractFilter->InPlaceOn();
		originExtractFilter->SetDirectionCollapseToSubmatrix();
		originExtractFilter->SetInput( originReader->GetOutput() );
		
		//���¹��������������쳣
		try{
			originExtractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//�趨ͼ����������������ڶ�д���ص�ֵ
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType origin( originExtractFilter->GetOutput(), originExtractFilter->GetOutput()->GetRequestedRegion() );

		//�趨�۲���������������ÿ���۲�����������ά�ȷֱ��Ӧ��ֵ�˲�ͼ���ԭʼͼ������������Ĵ���۲�����
		typedef itk::Vector< double , 1 > MeasurementVectorType;
		typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
		SampleType::Pointer sample = SampleType::New();
		sample->SetMeasurementVectorSize( 1 );
		MeasurementVectorType mv;
		for ( origin.GoToBegin(); !origin.IsAtEnd(); origin++ ){
			mv[0] = ( double ) origin.Get();
			sample->PushBack( mv );
		}
		
		//���趨������������kd����������cell�Ĵ�С
		typedef itk::Statistics::WeightedCentroidKdTreeGenerator< SampleType > TreeGeneratorType;
		TreeGeneratorType::Pointer treeGenerator = TreeGeneratorType::New();
		treeGenerator->SetSample( sample );
		treeGenerator->SetBucketSize( 16 );
		treeGenerator->Update();

		//��kd������kmeans��ֵ�����趨����ʼ�����ĵľ�ֵ���Լ��㷨ֹͣ����ֵ�趨
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

		//��estimator��ȡ���յĸ������ĵľ�ֵ�����
		EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();
		for ( unsigned int i = 0; i < 2; ++i ){
			std::cout << "cluster[" << i << "] " << std::endl;
			std::cout << "	estimated mean : " << estimatedMeans[i] << std::endl;
		}

		//���Ա�жϹ��������趨������Сֵ�жϹ�����˭����˭
		typedef itk::Statistics::MinimumDecisionRule DecisionRuleType;
		DecisionRuleType::Pointer decisionRule = DecisionRuleType::New();

		//�趨����������������Ա�ж�����
		typedef itk::Statistics::SampleClassifierFilter< SampleType > ClassifierType;
		ClassifierType::Pointer classifier = ClassifierType::New();
		classifier->SetDecisionRule( decisionRule );
		classifier->SetInput( sample );
		classifier->SetNumberOfClasses( 2 );

		//���ǩ���趨
		typedef ClassifierType::ClassLabelVectorObjectType ClassLabelVectorObjectType;
		typedef ClassifierType::ClassLabelVectorType ClassLabelVectorType;
		typedef ClassifierType::ClassLabelType ClassLabelType;
		ClassLabelVectorObjectType::Pointer classLabelsObject = ClassLabelVectorObjectType::New();
		ClassLabelVectorType &classLabelsVector = classLabelsObject->Get();
		classLabelsVector.push_back( 0 );
		classLabelsVector.push_back( 1 );

		//Ϊ�������趨���ǩ
		classifier->SetClassLabels( classLabelsObject );

		//����ֵ���õ������յĸ������ĵľ�ֵ���������Ⱥ���
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

		//Ϊ�������趨�����Ⱥ��������·��������������쳣
		classifier->SetMembershipFunctions( membershipFunctionVectorObject );
		try{
			classifier->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject classifier->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		//����������������������е����ǩ��ȡ��������ʹ��ͼ�����������д��ԭʼͼ����Ӧλ��
		const ClassifierType::MembershipSampleType *membershipSample = classifier->GetOutput();
		ClassifierType::MembershipSampleType::ConstIterator membershipSampleIterator = membershipSample->Begin();
		for ( origin.GoToBegin(); !origin.IsAtEnd(); origin++ ){
			origin.Set( membershipSampleIterator.GetClassLabel() );
			membershipSampleIterator++;
		}
			
		//�趨ͼƬ�����������
		joinSeries->PushBackInput( originExtractFilter->GetOutput() );
	}

	//����ͼƬ��������������쳣
	try{
		joinSeries->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject joinSeries->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	//�趨д��������
	typedef itk::ImageFileWriter< ImageType3D > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput( joinSeries->GetOutput() );
	writer->SetFileName( argv[2] );	
	
	//����д�������������쳣
	try{
		writer->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject writer->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
