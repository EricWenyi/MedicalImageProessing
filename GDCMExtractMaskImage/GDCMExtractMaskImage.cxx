#include "itkGDCMImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

#include "itkOpenCVImageBridge.h"

int main( int argc, char* argv[] ){
	if( argc < 3 ){
		std::cerr << "Usage: " << argv[0] << "InputImageFile OutputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	typedef signed short PixelType3D;
	typedef itk::Image< PixelType3D, 3 > ImageType3D;

	typedef unsigned char PixelType2D;
	typedef itk::Image< PixelType2D, 2 > ImageType2D;

	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

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
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType inIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	inIterator.SetFirstDirection( 0 );
	inIterator.SetSecondDirection( 1 );

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( originImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	int tempInCounter = 1;
	for( inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		printf( "Slice Index --- %d ---", tempInCounter++ );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );

		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New();
		inExtractor->SetExtractionRegion( sliceRegion );
		inExtractor->InPlaceOn();
		inExtractor->SetDirectionCollapseToSubmatrix();
		inExtractor->SetInput( originReader->GetOutput() );

		try{
			inExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		using namespace cv;
		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( inExtractor->GetOutput() );
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		RNG rng( 12345 );
		findContours( img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0) );

		float *distance = (float *)calloc( contours.size(), sizeof(float) );
		for (int i = 0; i < contours.size(); i++){
			distance[i] = 0.0f;
		}

		Scalar color = Scalar( rng.uniform( 0, 255 ) );
		int temp = 0;

		for(int l = img.cols/3; l < img.cols/2; l++){
			if(img.at<uchar>( img.rows/3, l ) == 0){
				continue;
			} else {
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						if(contours[i][j].x == l && contours[i][j].y == img.rows/3){
							for(int k = 0; k < contours[i].size() - 1; k++){
								if((contours[i][k+1].x != contours[i][k].x) && (contours[i][k+1].y != contours[i][k].y)){ 
									distance[i] += 1.4142135f;
								} else {
									distance[i] += 1.0f;
								}
							}
							temp = i;
						}
					}
				}
			}
			if( distance[temp] > 600.0f ){
				break;
			}
		}
		/*
		for (int i = 0; i < contours.size(); i++){
			for(int j = 0; j < contours[i].size() - 1; j++){
				if((contours[i][j+1].x != contours[i][j].x) && (contours[i][j+1].y != contours[i][j].y)){ 
					distance[i] += 1.4142135f;
				} else {
					distance[i] += 1.0f;
				}
			}
		}		

		float minimum = INT_MAX;
		int temp = 0;

		for(int i = 0; i < contours.size(); i++){
			printf("%f\n", distance[i]);
			if( distance[i] > 650.0f && distance[i] < 700.0f ){
				if( distance[i] < minimum ){
					minimum = distance[i];
					temp = i;
				}
			} else {
				continue;
			}
		}
		*/
		printf( "drawing\n" );
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );

		/*
		for( int i = 0; i < contours.size(); i++ ){
			Scalar color = Scalar( rng.uniform( 0, 255 ) );
			drawContours( drawing, contours, i, color, 1, 8, hierarchy, 0, Point(0, 0) );
		}
		*/
		
		drawContours( drawing, contours, temp, color, 1, 8, hierarchy, 0, Point(0, 0) );

		/*
		for( int i = 0; i < contours.size(); i++ ){
			for( int j = 0; j < contours[i].size(); j++ ){
				drawing.at<uchar>( contours[i][j].y, contours[i][j].x ) = 255;
			}
		}
		
		for( int j = 0; j < contours[temp].size(); j++ ){
			drawing.at<uchar>( contours[temp][j].y, contours[temp][j].x ) = 255;
		}
		*/

		for(int l = 2*img.cols/3; l < img.cols; l++){
			if(img.at<uchar>( img.rows/3, l ) == 0){
				continue;
			} else {
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						if(contours[i][j].x == l && contours[i][j].y == img.rows/3){
							for(int k = 0; k < contours[i].size() - 1; k++){
								if((contours[i][k+1].x != contours[i][k].x) && (contours[i][k+1].y != contours[i][k].y)){ 
									distance[i] += 1.4142135f;
								} else {
									distance[i] += 1.0f;
								}
							}
							temp = i;
						}
					}
				}
			}
			if( distance[temp] > 600.0f ){
				break;
			}
		}

		drawContours( drawing, contours, temp, color, 1, 8, hierarchy, 0, Point(0, 0) );

		ImageType2D::Pointer itkDrawing;
		try{
			itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( drawing );
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVMatToITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		printf( "%d ITK Drawing is OK\n", tempInCounter - 1 );
		joinSeries->PushBackInput( itkDrawing );
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
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}