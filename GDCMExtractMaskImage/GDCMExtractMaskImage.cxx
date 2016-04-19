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
	double perimeter = 100.0f;
	double area = 1000.0f;

	int startX1 = 200;
	int startX2 = 300;
	int startY1 = 300;
	int startY2 = 300; 

	for( inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		printf( "Slice Index --- %d ---", tempInCounter++ );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;
		
		if( tempInCounter > (int)(1/3.0f * inIterator.GetRegion().GetSize()[2]) && tempInCounter < (int)(2/3.0f * inIterator.GetRegion().GetSize()[2]) ){
			startX1 = 100;
			startX2 = 400;
			startY1 = 250;
			startY2 = 250;
			perimeter = 600.0f;
			area = 10000.0f;
		}

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
		findContours( img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0) );

		int index = 0;

		//开始算法，这是适用于测试图的代码，对于原图，系数等还需调整，对于测试图，这里y轴固定，x轴起点分别为1/3 cols，2/3 cols
		for(int x = startX1; x < img.cols/2; x++){
			//如果像素值为0（即背景点），则跳过
			if(img.at<uchar>( startY1, x ) == 0){
				continue;
			} else {
				//遍历contours，寻找该非0像素点所在的contours的序号
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						//如果坐标相同，则计算所在的contours的周长，并记录序号index
						if(contours[i][j].x == x && contours[i][j].y == startY1){
							index = i;
						}
					}
				}
			}
			//若该contours周长大于给定的值并且面积大于给定的值（适用于本测试图，原图的话需要分区设定不同的值），则退出大循环，开始画轮廓
			if( arcLength(contours[index], true) > perimeter && fabs(contourArea(contours[index])) > area ){
				break;
			}
		}

		printf( "drawing\n" );
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		
		//根据上面找到的序号画出左侧轮廓（此处也可用at方法画，at比较灵活，可以精确到像素点，想画几个画几个，像素值也可随意设定（不同点设置不同像素），而drawContours，只能送进去一个vector把整个vector用同一颜色画出来。。）
		drawContours( drawing, contours, index, Scalar(255), CV_FILLED, 8, hierarchy, 0, Point(0, 0) );

		//下文为画右边轮廓，原理同上
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
		
		drawContours( drawing, contours, index, Scalar(255), CV_FILLED, 8, hierarchy, 0, Point(0, 0) );

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