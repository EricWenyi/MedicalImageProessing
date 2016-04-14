#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

#include "itkThresholdImageFilter.h"
#include "itkCurvatureFlowImageFilter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

int main( int argc, char* argv[] ){
	if( argc < 4 ){
		std::cerr << "Usage: " << argv[0] << " dicomDirectory lowerThreshold originFile thresholdFile" << std::endl;
		return EXIT_FAILURE;
    }

	typedef itk::Image< signed short, 3 > InputImageType;
	typedef itk::ImageSeriesReader< InputImageType > ReaderType;
	typedef itk::Image< float, 2 > OutputImageType;

	typedef itk::GDCMImageIO ImageIOType;
	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();
	namesGenerator->SetInputDirectory( argv[1] );

	const ReaderType::FileNamesContainer & filenames = namesGenerator->GetInputFileNames();
	std::vector< std::string > tmp_filenames = filenames;
	const unsigned int numberOfFileNames = filenames.size();

    int j = numberOfFileNames;
    for(int i = 0; i < filenames.size(); i++){
		tmp_filenames[i] = filenames[j-1];
		j--;
    }

	for(unsigned int fni = 0; fni < numberOfFileNames; fni++){
		std::cout << "filename # " << fni + 1 << " = ";
		std::cout << tmp_filenames[fni] << std::endl;
    }

	ReaderType::Pointer reader = ReaderType::New();
	reader->SetImageIO( gdcmIO );
	reader->SetFileNames( tmp_filenames );

	try{
		reader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	InputImageType::ConstPointer InputImage3D = reader->GetOutput();
	typedef itk::ImageSliceConstIteratorWithIndex < InputImageType > SliceIteratorType;

	SliceIteratorType inIterator( InputImage3D, InputImage3D->GetLargestPossibleRegion() );
	inIterator.SetFirstDirection(0);
	inIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< InputImageType, OutputImageType > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< OutputImageType, InputImageType > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries1 = JoinSeriesFilterType::New();
	JoinSeriesFilterType::Pointer joinSeries2 = JoinSeriesFilterType::New();
	joinSeries1->SetOrigin(InputImage3D->GetOrigin()[2]);
	joinSeries1->SetSpacing(InputImage3D->GetSpacing()[2]);
	joinSeries2->SetOrigin(InputImage3D->GetOrigin()[2]);
	joinSeries2->SetSpacing(InputImage3D->GetSpacing()[2]);

	for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice()){
		InputImageType::IndexType sliceIndex = inIterator.GetIndex();

		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New();
		inExtractor->SetExtractionRegion( sliceRegion );
		inExtractor->InPlaceOn();
		inExtractor->SetDirectionCollapseToSubmatrix();
		inExtractor->SetInput( reader->GetOutput() );

		try{
			inExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		joinSeries1->PushBackInput( inExtractor->GetOutput() );

		typedef itk::CurvatureFlowImageFilter< OutputImageType, OutputImageType > CurvatureFlowImageFilterType;
		CurvatureFlowImageFilterType::Pointer smoothing = CurvatureFlowImageFilterType::New();
		smoothing->SetInput( inExtractor->GetOutput() );

		typedef itk::ThresholdImageFilter< OutputImageType > FilterType;
		FilterType::Pointer thresholdFilter = FilterType::New();

		const signed short lowerThreshold = atof( argv[2] );
		
		thresholdFilter->SetOutsideValue( -2048 );
		thresholdFilter->ThresholdBelow( lowerThreshold );
		thresholdFilter->SetInput( smoothing->GetOutput() );
		
		try{
			thresholdFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: threshold->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		joinSeries2->PushBackInput( thresholdFilter->GetOutput() );
	}

	try{
		joinSeries1->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries1->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	try{
		joinSeries2->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries1->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::ImageFileWriter<InputImageType> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( argv[3] );
	writer->SetInput( joinSeries1->GetOutput() );

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: writer->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	writer->SetFileName( argv[4] );
	writer->SetInput( joinSeries2->GetOutput() );

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: writer->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
