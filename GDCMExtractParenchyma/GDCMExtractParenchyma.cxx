#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

#include "itkThresholdImageFilter.h"
#include "itkMedianImageFilter.h"
#include "itkCurvatureFlowImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

#include "itkOpenCVImageBridge.h"

int main( int argc, char* argv[] ){
	if( argc < 2 ){
		std::cerr << "Usage: " << argv[0] << "dicomDirectory outputFile" << std::endl;
		return EXIT_FAILURE;
    }

	typedef signed short PixelType3D;
	typedef itk::Image< PixelType3D, 3 > ImageType3D;

	typedef signed short PixelType2D;
	typedef itk::Image< PixelType2D, 2 > ImageType2D;

	typedef itk::ImageSeriesReader< ImageType3D > ReaderType;

	typedef itk::GDCMImageIO ImageIOType;
	typedef itk::GDCMSeriesFileNames NamesGeneratorType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();
	namesGenerator->SetInputDirectory( argv[1] );

	const ReaderType::FileNamesContainer &filenames = namesGenerator->GetInputFileNames();
	std::vector< std::string > tempFilenames = filenames;
	int numberOfFileNames = filenames.size();

    int j = numberOfFileNames;
    for(int i = 0; i < filenames.size(); i++){
		tempFilenames[i] = filenames[j-1];
		j--;
    }

	for(int i = 0; i < numberOfFileNames; i++){
		std::cout << "filename # " << i + 1 << " = ";
		std::cout << tempFilenames[i] << std::endl;
    }

	ReaderType::Pointer reader = ReaderType::New();
	reader->SetImageIO( gdcmIO );
	reader->SetFileNames( tempFilenames );

	try{
		reader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer InputImage3D = reader->GetOutput();
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType iterator( InputImage3D, InputImage3D->GetLargestPossibleRegion() );
	iterator.SetFirstDirection(0);
	iterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();

	joinSeries->SetOrigin(InputImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(InputImage3D->GetSpacing()[2]);

	int tempInCounter = 1;

	double perimeter;
	double area;

	int startX1;
	int startX2;
	int startY1;
	int startY2;

	for(iterator.GoToBegin(); !iterator.IsAtEnd(); iterator.NextSlice()){
		ImageType3D::IndexType sliceIndex = iterator.GetIndex();
		printf( "Slice Index --- %d --- ", tempInCounter++ );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = iterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		if( tempInCounter > (int)(1/3.0f * iterator.GetRegion().GetSize()[2]) ){
			startX1 = 100;
			startX2 = 400;
			startY1 = 250;
			startY2 = 250;
			perimeter = 600.0f;
			area = 10000.0f;
		} else {
			startX1 = 200;
			startX2 = 350;
			startY1 = 350;
			startY2 = 350;
			perimeter = 100.0f;
			area = 1000.0f;
		}

		ExtractFilterType::InputImageRegionType sliceRegion = iterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( reader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		typedef itk::ImageRegionIterator< ImageType2D > IteratorType;
		IteratorType origin( extractor->GetOutput(), extractor->GetOutput()->GetRequestedRegion() );

		typedef itk::CurvatureFlowImageFilter< ImageType2D, ImageType2D > CurvatureFlowImageFilterType;
		CurvatureFlowImageFilterType::Pointer smoothing = CurvatureFlowImageFilterType::New();
		smoothing->SetInput( extractor->GetOutput() );

		typedef itk::ThresholdImageFilter< ImageType2D > FilterType;
		FilterType::Pointer thresholdFilter = FilterType::New();
		
		thresholdFilter->SetOutsideValue( -2048 );
		thresholdFilter->ThresholdBelow( -800 );
		thresholdFilter->SetInput( smoothing->GetOutput() );
		
		try{
			thresholdFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: threshold->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		typedef itk::MedianImageFilter< ImageType2D, ImageType2D >  MedianFilterType;
		MedianFilterType::Pointer medianFilter = MedianFilterType::New();

		ImageType2D::SizeType indexRadius;
		indexRadius.Fill(2);
		medianFilter->SetRadius( indexRadius );
		medianFilter->SetInput( thresholdFilter->GetOutput() );

		try{
			medianFilter->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject medianFilter->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}
		
		typedef itk::BinaryThresholdImageFilter< ImageType2D, ImageType2D > BinaryFilterType;
		BinaryFilterType::Pointer binaryFilter = BinaryFilterType::New();
		binaryFilter->SetInput(medianFilter->GetOutput());
		binaryFilter->SetOutsideValue(0);
		binaryFilter->SetInsideValue(1);
		binaryFilter->SetLowerThreshold(-200);
		binaryFilter->SetUpperThreshold(2047);

		try{
			binaryFilter->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject binaryFilter->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		using namespace cv;
		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( binaryFilter->GetOutput() );
		for(int i = 0; i < img.cols; i++){
			for(int j = 0; j < img.rows; j++){
				img.at<signed short>(j, i) = (unsigned char)img.at<signed short>(j, i);
			}
		}
		img.convertTo(img, CV_8UC1);
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours( img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0) );
		
		int index = 0;

		for(int x = startX1; x < img.cols/2; x++){
			if(img.at<uchar>( startY1, x ) == 0){
				continue;
			} else {
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						if(contours[i][j].x == x && contours[i][j].y == startY1){
							index = i;
						}
					}
				}
			}
			
			if( arcLength(contours[index], true) > perimeter && fabs(contourArea(contours[index])) > area ){
				break;
			}
		}

		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		
		drawContours( drawing, contours, index, Scalar(1), CV_FILLED, 8, hierarchy, 0, Point(0, 0) );

		for(int x = startX2; x > img.cols/2; x--){
			if(img.at<uchar>( startY2, x ) == 0){
				continue;
			} else {
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						if(contours[i][j].x == x && contours[i][j].y == startY2){
							index = i;
						}
					}
				}
			}

			if( arcLength(contours[index], true) > perimeter && fabs(contourArea(contours[index])) > area ){
				break;
			}
		}
		
		drawContours( drawing, contours, index, Scalar(1), CV_FILLED, 8, hierarchy, 0, Point(0, 0) );
		
		ImageType2D::Pointer itkDrawing;
		try{
			itkDrawing = itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( drawing );
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVMatToITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		IteratorType mask( itkDrawing, itkDrawing->GetRequestedRegion() );
		
		for ( origin.GoToBegin(), mask.GoToBegin(); !mask.IsAtEnd(); origin++, mask++ ){
			if( mask.Get() == 0 ){
				origin.Set(0);
			}
		}
		
		printf( "%d ITK Drawing is OK\n", tempInCounter - 1 );

		joinSeries->PushBackInput( extractor->GetOutput() );
	}

	try{
		joinSeries->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries->Update() caught !" << std::endl;
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
		std::cerr << "ExceptionObject: writer->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}