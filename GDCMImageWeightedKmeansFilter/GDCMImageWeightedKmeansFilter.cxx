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

//��������ͷ�ļ�
#include "itkListSample.h"

//kd����������ͷ�ļ�
#include "itkWeightedCentroidKdTreeGenerator.h"

//kd������ֵ��������ͷ�ļ�
#include "KdTreeBasedKmeansEstimator.h"

int main( int argc, char * argv[] ){

	//�жϲ���������������������ʾ��Ҫ�Ĳ���
	if( argc < 6 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "medianImageFile originImageFile outputImageFile indexBetweenUpperAndMiddle indexBetweenMiddleAndLower" << std::endl;
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
	ReaderType::Pointer medianReader = ReaderType::New();
	ReaderType::Pointer originReader = ReaderType::New();

	medianReader->SetImageIO( gdcmIO );
	medianReader->SetFileName( argv[1] );

	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[2] );

	//���¶�ȡ�����������쳣
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

	//�趨���������С
	ImageType3D::ConstPointer medianImage3D = medianReader->GetOutput();
	ImageType3D::RegionType medianRegion3D = medianImage3D->GetLargestPossibleRegion();

	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::RegionType originRegion3D = originImage3D->GetLargestPossibleRegion();
	
	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType medianIterator( medianImage3D, medianRegion3D );
	medianIterator.SetFirstDirection( 0 );
	medianIterator.SetSecondDirection( 1 );

	SliceIteratorType originIterator( originImage3D, originRegion3D );
	originIterator.SetFirstDirection( 0 );
	originIterator.SetSecondDirection( 1 );
	
	//����ͼƬ�����
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( originImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	//��ȡ�������������������������ٽ���Ƭ���
	double R;
 
	for( medianIterator.GoToBegin(), originIterator.GoToBegin(); !medianIterator.IsAtEnd(); medianIterator.NextSlice(), originIterator.NextSlice() ){
		//��ȡ��Ƭ���
		ImageType3D::IndexType medianSliceIndex = medianIterator.GetIndex();
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();

		//���λ�����������趨R=4.0������Ϊ8.0
		if ( medianSliceIndex[2] >= atoi(argv[4]) - 1 && medianSliceIndex[2] < atoi(argv[5])){
			R = 4.0f;
		} else {
			R = 8.0f;
		}

		//��ȡÿ����Ƭ�Ĵ�С��������ÿ����Ƭ��Z��Ϊ0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType medianSliceRegion = medianIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType medianSliceSize = medianSliceRegion.GetSize();
		medianSliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;

		//�趨ÿ����Ƭ�Ĵ�С�����
		medianSliceRegion.SetSize( medianSliceSize );
		medianSliceRegion.SetIndex( medianSliceIndex );

		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );
		
		//ָ������������
		ExtractFilterType::Pointer medianExtractFilter = ExtractFilterType::New();
		medianExtractFilter->SetExtractionRegion( medianSliceRegion );

		ExtractFilterType::Pointer originExtractFilter = ExtractFilterType::New();
		originExtractFilter->SetExtractionRegion( originSliceRegion );
		
		//��λ���������������޸ķ���̮������Ϊ�Ӿ���
		medianExtractFilter->InPlaceOn();
		medianExtractFilter->SetDirectionCollapseToSubmatrix();
		medianExtractFilter->SetInput( medianReader->GetOutput() );
		
		originExtractFilter->InPlaceOn();
		originExtractFilter->SetDirectionCollapseToSubmatrix();
		originExtractFilter->SetInput( originReader->GetOutput() );
		
		//���¹��������������쳣
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

		//�趨ͼ����������������ڶ�д���ص�ֵ
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType median( medianExtractFilter->GetOutput(), medianExtractFilter->GetOutput()->GetRequestedRegion() );
		IteratorType origin( originExtractFilter->GetOutput(), originExtractFilter->GetOutput()->GetRequestedRegion() );

		//�趨�۲���������������ÿ���۲�����������ά�ȷֱ��Ӧ��ֵ�˲�ͼ���ԭʼͼ������������Ĵ���۲�����
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
		EstimatorType::ParametersType initialMeans( 4 );
		initialMeans[0] = 0.0;
		initialMeans[1] = 0.0;
		initialMeans[2] = 0.0;
		initialMeans[3] = 0.0;
		estimator->SetParameters( initialMeans );
		estimator->SetKdTree( treeGenerator->GetOutput() );
		estimator->SetCentroidPositionChangesThreshold( 0.0 );
		estimator->StartOptimization( R );

		//��estimator��ȡ���յĸ������ĵľ�ֵ�����
		EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();

		std::cout<<"slice No."<<medianSliceIndex[2]+1<<std::endl;

		for ( unsigned int i = 0; i < 4; ++i ){
			std::cout << "cluster[" << i << "] " << std::endl;
			std::cout << "	estimated mean : " << estimatedMeans[i] << std::endl;
		}

		//�趨ŷ�Ͼ�������Լ����յı���������
		typedef itk::Array< double > ArrayType;
		typedef itk::Statistics::EuclideanDistanceMetric< ArrayType > DistanceMetricType;
		DistanceMetricType::Pointer distanceMetric = DistanceMetricType::New();
		ArrayType means0( 2 );
		means0[0] = estimatedMeans[0];
		means0[1] = estimatedMeans[1];
		ArrayType means1( 2 );
		means1[0] = estimatedMeans[2];
		means1[1] = estimatedMeans[3];

		//�������ص���background��nodule
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
	writer->SetFileName( argv[3] );	
	
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
