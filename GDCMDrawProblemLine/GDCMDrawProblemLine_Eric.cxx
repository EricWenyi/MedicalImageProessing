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

#include "itkOpenCVImageBridge.h"

#include "cv.h"
#include <math.h>

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

		//引入Opencv
		using namespace cv;
		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( inExtractor->GetOutput() );
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		RNG rng( 12345 );
		findContours( img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0) );//注意参数，之前吃大亏了
		
		struct APoint{
			int contour;
			int n;
			int x;
			int y;
			bool status;
		};
		Vector<APoint> repairedByStatus;
		for (int i = 0; i< contours.size(); i++){
			APoint apoint;
			for(int q = 0; q < contours[i].size(); q++ ){
				apoint.contour = i;
				apoint.n = q;
				apoint.x = contours[i][q].x;
				apoint.y = contours[i][q].y;
				apoint.status = false;
				repairedByStatus.push_back( apoint );
			}
			printf( "Enter %d/%d of contours\n", i + 1, contours.size() );
			float *disClockWise;
			float **disPixel;
			disClockWise = (float *)malloc( sizeof(float) * contours[i].size() * contours[i].size() );
			disPixel = (float **)malloc( sizeof(float) * contours[i].size() );
			for( int a = 0; a < contours[i].size(); a++ ){
				disPixel[a] = &disClockWise[a * contours[i].size()];
			}
			float *disCounterClockWise;
			float **disCounterPixel;
			disCounterClockWise = (float *)malloc( sizeof(float) * contours[i].size() * contours[i].size() );
			disCounterPixel = (float **)malloc( sizeof(float) * contours[i].size() );
			for( int a = 0; a < contours[i].size(); a++ ){
				disCounterPixel[a] = &disCounterClockWise[a * contours[i].size()];
			}
			for( int j = 0; j < contours[i].size(); j++ ){
				for( int k = 0; k < contours[i].size(); k++ ){
					disPixel[j][k] = 0;
					disCounterPixel[j][k] = 0;
				}
			}
			for( int j = 0; j < contours[i].size(); j++ ){
				for( int k = 0; k < contours[i].size(); k++ ){
					if( j < k ){
						for( int m = j; m < k ; m++ ){
							if( (contours[i][m+1].x != contours[i][m].x)&&(contours[i][m+1].y != contours[i][m].y) ){ 
								disPixel[j][k] += 1.4142135f; 
							}else{
								disPixel[j][k] += 1.0f;
							}
						}
						for( int m = 0; m < j; m++ ){
							if( (contours[i][m+1].x != contours[i][m].x)&&(contours[i][m+1].y != contours[i][m].y) ){
								disCounterPixel[j][k] += 1.4142135f;
							}else{
								disCounterPixel[j][k] += 1.0f;
							}
						}
						for( int m = k; m < contours[i].size() - 1; m++ ){
							if( (contours[i][m+1].x != contours[i][m].x)&&(contours[i][m+1].y != contours[i][m].y) ){
								disCounterPixel[j][k] += 1.4142135f; 
							}else{
								disCounterPixel[j][k] += 1.0f;
							}
						}
						if( (contours[i][contours[i].size()-1].x != contours[i][0].x)&&(contours[i][contours[i].size()-1].y != contours[i][0].y) ){
							disCounterPixel[j][k] += 1.4142135f;
						}else{
							disCounterPixel[j][k] += 1.0f;
						}
					}else if( j > k ){
						for( int m = j; m < contours[i].size(); m++ ){
							disPixel[j][k] = disCounterPixel[k][j]; 
							disCounterPixel[j][k] = disPixel[k][j];
						}
					}else{
						disPixel[j][k] = 0;
						disCounterPixel[j][k] = 0;
					}
				}
			}
			printf( "i:%d distance calc OK\n", i + 1 );

			int Points_Ahead = 0;
			for(int round_ahead = 0; round_ahead<i; round_ahead++){
				Points_Ahead += contours[round_ahead].size();
			}


			for( int j = 0; j < contours[i].size(); j++ ){
				for( int k = 0; k < contours[i].size(); k++ ){ 
					if( disPixel[j][k] < disCounterPixel[j][k] ){
						float disOfjk = (float)sqrt( (double)((contours[i][j].x - contours[i][k].x) * (contours[i][j].x - contours[i][k].x) + (contours[i][j].y - contours[i][k].y) * (contours[i][j].y - contours[i][k].y)) );
						if( disPixel[j][k] / disOfjk >8 ){	

							//std::cout<<disPixel[j][k]<<"    "<<disOfjk<<std::endl;
							if( k > j ){
								for( int p = j; p <= k; p++ ){

									repairedByStatus[p + Points_Ahead].status = true;
								}
							}else{
								for( int p = j; p <= k + contours[i].size(); p++ ){
									repairedByStatus[(p % contours[i].size()) + Points_Ahead].status = true;
								}
							}
						}
					}
				}
			}
			printf( "%d Detecting is OK, enter repair...\n", tempInCounter - 1 );
		}

		//TODO replace

		// Draw contours
		printf( "drawing\n" );
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		/*
		for( int i = 0; i < contours.size(); i++ ){
			Scalar color = Scalar( rng.uniform( 0, 255 ) );
			drawContours( drawing, contours, i, color, 1, 8, hierarchy, 0, Point(0, 0) );//注意参数，之前吃大亏了
		}
		*/
		for( int i = 0; i < repairedByStatus.size(); i++ ){
			if( repairedByStatus[i].status ){
				drawing.at<uchar>( repairedByStatus[i].y, repairedByStatus[i].x ) = 128;
			} else {
				drawing.at<uchar>( repairedByStatus[i].y, repairedByStatus[i].x ) = 255;
			}
		}
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
