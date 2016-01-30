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
		findContours( img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, Point(0, 0) );
		
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		Scalar color = Scalar( rng.uniform( 0, 255 ) );
		
		vector<RotatedRect> fixedEllipse( contours.size() );//����װfitEllipse֮��Ľ��
		vector<RotatedRect> ellipseBox( contours.size() );//��Ҫ���ڿ����Բ
		vector<Rect> boundingBox( contours.size() );//����minAreaRect���ҳ������С����ת���Σ�RotatedRect��
		Point2f vertices[4];//���������ȡ����ת���ζ˵�

		for(int i = 0; i < contours.size(); i++){
			//֮ǰ��boudingBox
			//boundingBox[i] = boundingRect( contours[i] );
			//rectangle(drawing, boundingBox[i].tl(), boundingBox[i].br(), color, 1, 8, 0);
			if(contours[i].size() >= 5){//������֪�����������ߣ���������ĵ�����5���ᱨ��
				fixedEllipse[i] = fitEllipse(contours[i]);
				ellipse(drawing, fixedEllipse[i], color, 1, 8);//������Բ�����һ��ͼ��һ��Mat����Ȼ����findContours����������minAreaRect
				//��������������contours����ת����
				//ellipseBox[i] = minAreaRect(contours[i]);
				//��ȡ�ĵ㱣����vertices
				//ellipseBox[i].points(vertices);
				//for(int i=0;i<4;i++){
                      //line( drawing, vertices[i], vertices[(i+1)%4], color);//�ĸ��ǵ������ߣ������γ���ת�ľ��Ρ�    
				//}
				drawContours( drawing, contours, i, color, 1, 8, hierarchy, 0, Point(0, 0) );//��ԭ����
			}
			//ԭboundingBox
			/*
			if( boundingBox[i].area() / contourArea(contours[i]) > 2.0f ){
				drawContours( drawing, contours, i, color, 1, 8, hierarchy, 0, Point(0, 0) );
			}
			*/
			
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