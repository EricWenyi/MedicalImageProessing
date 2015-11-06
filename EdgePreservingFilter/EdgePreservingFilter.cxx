#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"


#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"
#include "itkFastMarchingImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

#include "itkRescaleIntensityImageFilter.h"


int main( int argc, char* argv[] ){
	if( argc < 7 ){
		std::cerr << "Usage: " << argv[0] << " InputImageFile OutputImageFile Sigma SigmoidAlpha SigmoidBeta TimeThreshold" << std::endl;
		return EXIT_FAILURE;
    }
	//��άͼ������
	typedef signed short PixelType3D;
	const unsigned int Dimension3D = 3;
	typedef itk::Image< PixelType3D, Dimension3D > ImageType3D;
	
	//��άͼ������
	const unsigned int Dimension2D = 2;
	typedef float PixelType2D;//�˴�ԭΪsigned short��Ӧ����Ϊfloat���ͣ�����CurvatureFlowImageFilter��Ҫ�󣬷��򽫻ᵯ��������ʧȥ��Ӧ
	typedef itk::Image< PixelType2D, Dimension2D > ImageType2D;


	//�趨����ͼ�������
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//�趨��ȡ�����ͼ��趨��ȡ��ͼ�����ͺ�ͼ���ַ
	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer originReader = ReaderType::New();
	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );


	try{
		originReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType inIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	inIterator.SetFirstDirection(0);
	inIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(originImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(originImage3D->GetSpacing()[2]);

	for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice()){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();

		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New();
		inExtractor->SetExtractionRegion( sliceRegion );
		inExtractor->InPlaceOn();
		inExtractor->SetDirectionCollapseToSubmatrix();//�˴�ԭΪ�ޣ������ʵ�ִ˷������˷��������趨����̮����ģʽ����Դ�ļ���ͷ�����ݣ���ûʲô�ã�������ǿ����Ҫ���ã�������������µ�����ʱ���Ჶ���쳣
		inExtractor->SetInput( originReader->GetOutput() );

		try{
			inExtractor->Update();//ԭ�˴���6010�������try��catch���֣��������޸����������ǽ�����Update()����������try��catch
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		//��ֵ�˲���
		typedef itk::BinaryThresholdImageFilter< ImageType2D,
                        ImageType2D    >    ThresholdingFilterType;
		ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
		const PixelType2D  timeThreshold = atof( argv[6] );

		//��ΪSigmoid���������0��1֮���ֵ����ȡ��ֵ0.5
		thresholder->SetLowerThreshold(0.5);
		thresholder->SetUpperThreshold( timeThreshold  );//timeThreshold����Ϊ1����

		thresholder->SetOutsideValue(  0  );
		thresholder->SetInsideValue(  255 );
		//���¹��̵�����ͼ�μ�ITK II P386ҳ���õ�Edge Image
		typedef itk::RescaleIntensityImageFilter<
                               ImageType2D,
                               ImageType2D >   CastFilterType;
		typedef   itk::CurvatureAnisotropicDiffusionImageFilter<
                               ImageType2D,
                               ImageType2D >  SmoothingFilterType;
		SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();
		typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter<
                               ImageType2D,
                               ImageType2D >  GradientFilterType;
		typedef   itk::SigmoidImageFilter<
                               ImageType2D,
                               ImageType2D >  SigmoidFilterType;
		GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
		SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
		sigmoid->SetOutputMinimum(  0.0  );
		sigmoid->SetOutputMaximum(  1  );
		typedef  itk::FastMarchingImageFilter< ImageType2D,
                              ImageType2D >    FastMarchingFilterType;
		FastMarchingFilterType::Pointer  fastMarching
                                              = FastMarchingFilterType::New();
		smoothing->SetTimeStep( 0.1 );
		smoothing->SetNumberOfIterations(  5 );
		smoothing->SetConductanceParameter( 9.0 );
		const double sigma = atof( argv[3] );
		gradientMagnitude->SetSigma(  sigma  );
		const double alpha =  atof( argv[4] );
		const double beta  =  atof( argv[5] );

		sigmoid->SetAlpha( alpha );
		sigmoid->SetBeta(  beta  );


		smoothing->SetInput( inExtractor->GetOutput() );
		gradientMagnitude->SetInput( smoothing->GetOutput() );
		sigmoid->SetInput( gradientMagnitude->GetOutput() );
		thresholder->SetInput( sigmoid->GetOutput() );
		try{
			thresholder->Update();
		}catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: Thresholder->Update(); caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
  }
  joinSeries->PushBackInput( thresholder->GetOutput() );
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
