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
	double size = 100.0f;
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

		if( tempInCounter > 46 && tempInCounter < 106 ){
			//������Ƭ����λ���趨��ͬ�Ĳ���
			//�ҳ���Чslice����
			//ȷ��ÿ����������
			//��Ч��1-12 106-150
			//12-46 47-97 98-105
			startX1 = 100;
			startX2 = 400;
			startY1 = 256;
			startY2 = 256;
			size = 600.0f;
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

		//��ʼ��distance
		float *distance = (float *)calloc( contours.size(), sizeof(float) );
		for (int i = 0; i < contours.size(); i++){
			distance[i] = 0.0f;
		}

		int index = 0;

		//��ʼ�㷨�����������ڲ���ͼ�Ĵ��룬����ԭͼ��ϵ���Ȼ�����������ڲ���ͼ������y��̶���x�����ֱ�Ϊ1/3 cols��2/3 cols
		for(int x = startX1; x < img.cols/2; x++){
			//�������ֵΪ0���������㣩��������
			if(img.at<uchar>( startY1, x ) == 0){
				continue;
			} else {
				//����contours��Ѱ�Ҹ÷�0���ص����ڵ�contours�����
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						//���������ͬ����������ڵ�contours���ܳ�������¼���index
						if(contours[i][j].x == x && contours[i][j].y == startY1){
							for(int k = 0; k < contours[i].size() - 1; k++){
								if((contours[i][k+1].x != contours[i][k].x) && (contours[i][k+1].y != contours[i][k].y)){ 
									distance[i] += 1.4142135f;
								} else {
									distance[i] += 1.0f;
								}
							}
							index = i;
						}
					}
				}
			}
			//����contours�ܳ����ڸ�����ֵ����������ڸ�����ֵ�������ڱ�����ͼ��ԭͼ�Ļ���Ҫ�����趨��ͬ��ֵ�������˳���ѭ������ʼ������
			if( distance[index] > size && fabs(contourArea(contours[index])) > area ){
				break;
			}
		}

		printf( "drawing\n" );
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		
		//���������ҵ�����Ż�������������˴�Ҳ����at��������at�Ƚ������Ծ�ȷ�����ص㣬�뻭����������������ֵҲ�������趨����ͬ�����ò�ͬ���أ�����drawContours��ֻ���ͽ�ȥһ��vector������vector��ͬһ��ɫ������������
		drawContours( drawing, contours, index, Scalar(255), CV_FILLED, 8, hierarchy, 0, Point(0, 0) );

		//����Ϊ���ұ�������ԭ��ͬ��
		for(int x = startX2; x > img.cols/2; x--){
			if(img.at<uchar>( startY2, x ) == 0){
				continue;
			} else {
				for (int i = 0; i < contours.size(); i++){
					for(int j = 0; j < contours[i].size() - 1; j++){
						if(contours[i][j].x == x && contours[i][j].y == startY2){
							for(int k = 0; k < contours[i].size() - 1; k++){
								if((contours[i][k+1].x != contours[i][k].x) && (contours[i][k+1].y != contours[i][k].y)){ 
									distance[i] += 1.4142135f;
								} else {
									distance[i] += 1.0f;
								}
							}
							index = i;
						}
					}
				}
			}
			if( distance[index] > size && fabs(contourArea(contours[index])) > area ){
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