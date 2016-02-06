#include "itkGDCMImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
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
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );\

	using namespace cv;

	struct APoint{
		int c;//所在轮廓序号，用于定位
		int n;//点在轮廓中的序号，用于定位
		int x;//X坐标
		int y;//Y坐标
		bool isUpConnected;//是否跟别的contour的点相连（26领域相连），默认为false
		bool isDownConnected;//是否跟别的contour的点相连（26领域相连），默认为false
	};
	vector<APoint> points;
	vector<int> eraseIndex;

	APoint apoint;
	int tempIndex = -1;
	int eraseCount = 0;

	typedef itk::ConstNeighborhoodIterator< ImageType3D > NeighborhoodIteratorType;
	NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(1);
	NeighborhoodIteratorType it( radius, originImage3D, originImage3D->GetLargestPossibleRegion() );
	ImageType3D::IndexType location;
	/*
	for(it.GoToBegin(); !it.IsAtEnd(); it++){
		//std::cout<<it.GetElement(26)<<std::endl;//get the buffer address of neighbor voxels
		ImageType3D::IndexType center = it.GetIndex();
		//std::cout << center[0] << " " << center[1] << " " << center[2] << std::endl;
		//std::cout<<it.GetCenterPixel()<<std::endl;
		//NeighborhoodIteratorType::RegionType region = it.GetBoundingBoxAsImageRegion();
		for(int i = 0; i < 27; i++){
			ImageType3D::IndexType index = it.GetIndex(i);	
			//std::cout << index[0] << " " << index[1] << " " << index[2] << std::endl;
			//std::cout<<it.GetPixel(i)<<std::endl;
		}
	}

	画出每个contours的boundingRect，长或宽小于3的所在contours删去
	struct APoint{
		int c;//所在轮廓序号，用于定位
		int n;//点在轮廓中的序号，用于定位
		int x;//X坐标
		int y;//Y坐标
		bool isUpConnected;//是否跟别的contour的点相连（26领域相连），默认为false
		bool isDownConnected;//是否跟别的contour的点相连（26领域相连），默认为false
	};
	vector<APoint> points;
	
	对于每个切片分别遍历contours，按contours顺序推到上述结构体中(APoint apoint，然后赋值，然后push_back进去）
	然后遍历points，对于points中每个元素判断领域编号0-8，18-26的点的像素值
	只要不全为0，isUpConnected=true（0-8）或isDownConnected=true（18-26）
	然后把两个都为0的点所在的contours删去
	检查每个contours里头的点的true和false是否一致，不一致的contours删去

	先前也有想过简化，把isUpConnected和isDownConnected合并成一个isConnected，然后只要每一层向下一层找非0点
	但是找到非0点就要把自己和找到的点的isConnected都标记为true，但是怎么快速定位那个找到的点（不知道它属于哪个conours）
	一种就是在contours里头根据它的坐标来找，效率明显低，另一种就是把points的范围从contours的点扩展到所有点
	这样可以points[x][y]直接定位到要找的点然后修改标记，但是所有点占用很大的空间后面遍历也很慢
	最重要的是无法解决3个像素点的问题
	*/
	for( inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		location[2] = sliceIndex[2];
		printf( "Slice Index --- %d ---\n", sliceIndex[2] );
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

		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( inExtractor->GetOutput() );
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours( img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, Point(0, 0) );
		
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );
		vector<Rect> boundingBox( contours.size() );
		
		for(int i = 0; i < contours.size(); i++){
			boundingBox[i] = boundingRect(contours[i]);
			if( boundingBox[i].height < 3 || boundingBox[i].width < 3 ){
				contours.erase(contours.begin() + i);
				i--;
			}
		}
		
		for(int i = 0; i < contours.size(); i++){
			for(int j = 0; j < contours[i].size(); j++){
				apoint.c = i;
				apoint.n = j;
				apoint.x = contours[i][j].x;
				apoint.y = contours[i][j].y;
				apoint.isUpConnected = false;
				apoint.isDownConnected = false;
				points.push_back(apoint);
			}
		}

		//std::cout<<points.size()<<std::endl;

		for(int i = 0; i < points.size(); i++){
			location[0] = points[i].x;
			location[1] = points[i].y;
			it.SetLocation(location);
			for(int j = 0; j < 9; j++){
				if(it.GetPixel(j) != 0){
					points[i].isUpConnected = true;
					break;
				}
			}

			for(int k = 18; k < 27; k++){
				if(it.GetPixel(k) != 0){
					points[i].isDownConnected = true;
					break;
				}
			}

			//std::cout<<points[i].c<<std::endl;

			if(points[i].isUpConnected == false && points[i].isDownConnected == false){
				if(points[i].c != tempIndex){
					eraseIndex.push_back(points[i].c);
					tempIndex = points[i].c;
				}
			}
		}
		
		for(int i = 0; i < eraseIndex.size(); i++){
			std::cout<<eraseIndex[i]<<std::endl;
			drawContours( drawing, contours, eraseIndex[i], Scalar( 255 ), 1, 8, hierarchy, 0, Point(0, 0) );
		}

		Mat draw = Mat::zeros( img.size(), CV_8UC1 );
		for(int i = 0; i < contours.size(); i++){
			drawContours( draw, contours, i, Scalar( 255 ), 1, 8, hierarchy, 0, Point(0, 0) );
		}

		for(int i = 0; i < draw.cols; i++){
			for(int j = 0; j < draw.rows; j++){
				if(drawing.at<uchar>(j, i) != 0){
					draw.at<uchar>(j, i) = 0;
				}
			}
		}

		eraseIndex.clear();
		points.clear();

		ImageType2D::Pointer itkDrawing;
		try{
			itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( draw );
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVMatToITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
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