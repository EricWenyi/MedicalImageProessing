#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

#include "itkThresholdImageFilter.h"
#include "itkImage.h"
#include "itkCurvatureFlowImageFilter.h"

#include <vector>
#include "itksys/SystemTools.hxx"
#include "itkCastImageFilter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

int main( int argc, char* argv[] ){
	if( argc < 4 ){
		std::cerr << "Usage: " << argv[0] << " DicomDirectory OutputDicomPath&Name lowerThreshold" << std::endl;
		return EXIT_FAILURE;
    }

	typedef signed short PixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image< PixelType, Dimension > ImageType;
	typedef itk::ImageSeriesReader< ImageType > ReaderType;

	const unsigned int OutputDimension = 2;

	typedef float OutputPixelType;//此处原为signed short，应设置为float类型，下文CurvatureFlowImageFilter所要求，否则将会弹窗报错至失去响应
	typedef itk::Image< OutputPixelType, OutputDimension > OutputImageType;

	typedef itk::GDCMImageIO ImageIOType;
	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();
	namesGenerator->SetInputDirectory( argv[1] );

	ReaderType::FileNamesContainer & filenames = const_cast<ReaderType::FileNamesContainer &> (namesGenerator->GetInputFileNames());
	const ReaderType::FileNamesContainer & tmp_filenames = filenames;
	const unsigned int numberOfFileNames = filenames.size();

    //reverse order?? bug???

                int j=numberOfFileNames;
                for(int i=0; i< tmp_filenames.size(); i++)
                {
                        filenames[i]=tmp_filenames[j-1];
                        j--;
                }
                //end reverse

	for(unsigned int fni = 0; fni < numberOfFileNames; ++fni){
		std::cout << "filename # " << fni + 1 << " = ";
		std::cout << filenames[fni] << std::endl;
    }

	ReaderType::Pointer reader = ReaderType::New();
	reader->SetImageIO( gdcmIO );
	reader->SetFileNames( filenames );

	try{
		reader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType::ConstPointer InputImage3D = reader->GetOutput();
	typedef itk::ImageSliceConstIteratorWithIndex <ImageType> SliceIteratorType;

	SliceIteratorType inIterator( InputImage3D, InputImage3D->GetLargestPossibleRegion() );
	inIterator.SetFirstDirection(0);
	inIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType, OutputImageType > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< OutputImageType, ImageType > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(InputImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(InputImage3D->GetSpacing()[2]);

	for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice()){
		ImageType::IndexType sliceIndex = inIterator.GetIndex();

		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New();
		inExtractor->SetExtractionRegion( sliceRegion );
		inExtractor->InPlaceOn();
		inExtractor->SetDirectionCollapseToSubmatrix();//此处原为无，如果不实现此方法（此方法用于设定方向坍塌的模式，看源文件里头的内容，虽没什么用，但还是强制需要设置），否则下面更新迭代器时将会捕获到异常
		inExtractor->SetInput( reader->GetOutput() );

		try{
			inExtractor->Update();//原此处报6010错误，添加try，catch后发现，上文有修复方法，于是将所有Update()方法都加了try，catch
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		typedef itk::CurvatureFlowImageFilter< OutputImageType, OutputImageType > CurvatureFlowImageFilterType;
  
		CurvatureFlowImageFilterType::Pointer smoothing = CurvatureFlowImageFilterType::New();
		smoothing->SetInput(inExtractor->GetOutput());

		typedef itk::ThresholdImageFilter< OutputImageType > FilterType;

		FilterType::Pointer ThresholdFilter = FilterType::New();

		
		const PixelType lowerThreshold = atof( argv[3] );


		ThresholdFilter->SetOutsideValue( -2048 );
		ThresholdFilter->ThresholdBelow( lowerThreshold );
  


		ThresholdFilter->SetInput(smoothing->GetOutput());
		
		try{
			ThresholdFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: connectedThreshold->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		joinSeries->PushBackInput( inExtractor->GetOutput() );
	}

	try{
		joinSeries->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries->Update(); caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::ImageFileWriter<ImageType> WriterType;
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
