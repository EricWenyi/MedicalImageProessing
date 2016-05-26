#include "itkGDCMImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

#include "itkOpenCVImageBridge.h"

#include <algorithm>

#define THRESHOLD 1.5
#define PERCENTOFPOINTS_F 0.50
#define PERCENTOFPOINTS_B 0.25
struct APoint{
	int contour;
	int n;
	int x;
	int y;
	bool status;
};

cv::vector<cv::vector<cv::Point>> contours;
cv::Vector<APoint> repairedByStatus;

//寻找两点之间的欧式距离
float Euc_distance(int x1, int y1, int x2, int y2);

//判断新push进来的点是左肺还是右肺
int left_or_right(cv::Vector<APoint> &repairedByStatus);

int repairContour(cv::Vector<APoint> &repairedByStatus,int i);

int findStatusBegin(cv::Vector<APoint> &repairedByStatus,int i);

int findStatusEnd(cv::Vector<APoint> &repairedByStatus,int i);

int findNewContourBegin(cv::Vector<APoint> &repairedByStatus,int i);

void delAndDrawLine(cv::Vector<APoint> &repairedByStatus,int i,int statusBegin,int statusEnd);

void AddNewNode(int possition,int i,cv::Point &newNode);

void NodeDelete(int possition,int i);

int repairInsideContour(int i);

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

	//从这里开始遍历每一张图片
	for( inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		printf( "Slice Index --- %d ---\n", tempInCounter++ );
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

		cv::vector<cv::vector<cv::Point>> tempContours;
		contours=tempContours;

		cv::Vector<APoint> tempRepairedByStatus;
		repairedByStatus=tempRepairedByStatus;

		cv::Vector<APoint> tempRBS;

		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( inExtractor->GetOutput() );

		vector<Vec4i> hierarchy;
		findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0) );
		printf("End of findcontours\n");
		
		for (int i = 0; i< contours.size(); i++){//一般情况下contours.size() == 2
			APoint apoint;
			for(int q = 0; q < contours[i].size(); q++ ){
				apoint.contour = i;
				apoint.n = q;
				apoint.x = contours[i][q].x;
				apoint.y = contours[i][q].y;
				apoint.status = false;
				tempRBS.push_back( apoint );
			}

			//接下来是Eric的修改

			//通过测试发现，随着repairedByStatus[i]的增加，像素点按照逆时针方向旋转
			//又通过测试发现，findContour函数每次都是先找到了最顶点的点，也就是说repairedByStatus[0 + n]为顶点,
			//n是之前contour推进去的点数

			//之后分为左右两肺进行判断(左右是指图片的左右)

			//先是左肺
			if(left_or_right(tempRBS) == 0){//如果left_or_right为0代表是左肺
				for(int i = 0; i < (int)(PERCENTOFPOINTS_F * tempRBS.size()) ; i++){
					//计算在5~15个像素点范围内的点的欧式距离和轮廓距离
					for(int j = 5; j < 10; j++){
						float Eu_dis = 0.0;
						float Con_dis = 0.0;
						//计算欧式距离
						Eu_dis = Euc_distance(tempRBS[i].x,tempRBS[i].y,tempRBS[i+j].x,tempRBS[i+j].y);

						//计算contour距离
						for(int q = 0; q < j ; q++){
							int p = i;
							Con_dis += Euc_distance(tempRBS[p].x,tempRBS[p].y,tempRBS[p+1].x,tempRBS[p+1].y);
							p++;
						}
						//如果到达临界值
						if(Con_dis / Eu_dis > THRESHOLD){
							for(int p = i; p <= i + j; p++){
								tempRBS[p].status = true;
							}
						}

					}
				}
			}//接下来是右肺，注意repairedByStatus中已经有左肺的点
			else{
				//因为随着i增加，像素点按照逆时针方向旋转，对于右肺就不适用了，我们从后向前遍历
				for(int i = tempRBS.size() - 1; i > tempRBS.size() * (1 - PERCENTOFPOINTS_F); i--){
					//计算在5~15个像素点范围内的点的欧式距离和轮廓距离
					for(int j = 5; j < 10; j++){
						float Eu_dis = 0.0;
						float Con_dis = 0.0;
						//计算欧式距离
						Eu_dis = Euc_distance(tempRBS[i].x,tempRBS[i].y,tempRBS[i-j].x,tempRBS[i-j].y);
						//计算轮廓距离
						for(int q = 0; q < j ; q++){
							int p = i;
							Con_dis += Euc_distance(tempRBS[p].x,tempRBS[p].y,tempRBS[p-1].x,tempRBS[p-1].y);
							p--;
						}

						//如果到达临界值
						if(Con_dis / Eu_dis > THRESHOLD){
							for(int p = i; p >= i - j; p--){
								tempRBS[p].status = true;
							}
						}
					}
				}
			}

			

			//std::cout<<tempRBS.size()<<std::endl;
			for(int k = 0; k < tempRBS.size();k++){
				repairedByStatus.push_back(tempRBS[k]);
			}
			tempRBS.clear();

			//std::cout<<repairedByStatus.size()<<std::endl;

			printf( "%d Detecting is OK, enter repair...\n", i +1 );
		}

		system("pause");

		printf("End of detecting\n");

		repairContour(repairedByStatus, 0);

		printf("End of repairing\n");

		printf( "Drawing\n" );
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );

		for( int i = 0; i < contours.size(); i++ ){
			drawContours( drawing, contours, i, Scalar( 255 ), 1, 8, hierarchy, 0, Point(0, 0) );
		}
		
		ImageType2D::Pointer itkDrawing;

		try{
			itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( drawing );
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

float Euc_distance(int x1, int y1, int x2, int y2){
	return sqrt((float)(x1 - x2)*(x1 - x2) + (float)(y1 - y2)*(y1 - y2));
}

int left_or_right(cv::Vector<APoint> &repairedByStatus){
	int totalx = 0;
	for(int i = 0; i < repairedByStatus.size(); i++){
		totalx += repairedByStatus[i].x;
	}
	float position_x = totalx / repairedByStatus.size();

	if(position_x <= 255)
		return 0;
	else
		return 1;
}

int repairContour(cv::Vector<APoint> &repairedByStatus,int i){
	int insideReturn = repairInsideContour(i);
	if(insideReturn == 0){
		return 0;
	}
	int newContourBeginI = findNewContourBegin(repairedByStatus,i);
	if(newContourBeginI == -1){
		return 0;
	} else {
		return repairContour(repairedByStatus,newContourBeginI);
	}
}

int repairInsideContour(int i){
	int statusBegin = 0;
	printf("Reparing %d/%d repairedByStatus",i,repairedByStatus.size());
	statusBegin = findStatusBegin(repairedByStatus,i);
	if(statusBegin == -1){
		int newContourBeginI = findNewContourBegin(repairedByStatus,i);
		if(newContourBeginI == -1){
			return 0;
		} else {
			return 1;
		}
	} else if (statusBegin == -2){
		return 0;
	}
	int statusEnd = 0;
	statusEnd = findStatusEnd(repairedByStatus,statusBegin);
	if(statusEnd == -1){
		return 0;
	}
	delAndDrawLine(repairedByStatus,repairedByStatus[i].contour,statusBegin,statusEnd);
	if(statusEnd < repairedByStatus.size() && repairedByStatus[statusEnd].contour == repairedByStatus[statusEnd + 1].contour){
		repairInsideContour(statusEnd + 1);
	}
	return 1;
}


int findStatusBegin(cv::Vector<APoint> &repairedByStatus,int i){
	int currentContour = repairedByStatus[i].contour;
	while(repairedByStatus[i].contour == currentContour && (!repairedByStatus[i].status) && i < repairedByStatus.size() - 1){
		i++;
	}
	if(repairedByStatus[i].contour != currentContour){
		return -1;
	} else if (i == repairedByStatus.size() - 1){
		return -2;
	} else {
		return i;
	}
}

int findStatusEnd(cv::Vector<APoint> &repairedByStatus,int i){
	int currentContour = repairedByStatus[i].contour;
	while(i < repairedByStatus.size() - 1 && repairedByStatus[i].contour == currentContour && (repairedByStatus[i].status)){
		i++;
	}

	if(i == repairedByStatus.size() - 1){
		return -1;
	} else {
		return i - 1;
	}
}

int findNewContourBegin(cv::Vector<APoint> &repairedByStatus,int i){
	int currentContour = repairedByStatus[i].contour;
	while(i < repairedByStatus.size() && repairedByStatus[i].contour == currentContour){
		i++;
	}
	if(i < repairedByStatus.size()){
		return i;
	} else {
		return -1;
	}
}

void delAndDrawLine(cv::Vector<APoint> &repairedByStatus,int i,int statusBegin,int statusEnd){
	int currentNode = 0;
	while((contours[i][currentNode].x != repairedByStatus[statusBegin].x) || (contours[i][currentNode].y != repairedByStatus[statusBegin].y)){
		currentNode++;
	}
	while((contours[i][currentNode].x != repairedByStatus[statusEnd].x) || (contours[i][currentNode].y != repairedByStatus[statusEnd].y)){
		NodeDelete(currentNode,i);
	}
	int Rx = repairedByStatus[statusEnd].x - repairedByStatus[statusBegin].x;
	int Ry = repairedByStatus[statusEnd].y - repairedByStatus[statusBegin].y;
	printf("=============%d,%d===(%d,%d)(%d,%d)=======",Rx,Ry,repairedByStatus[statusBegin].x,repairedByStatus[statusBegin].y,repairedByStatus[statusEnd].x,repairedByStatus[statusEnd].y);
	//assert the node of statusEnd is known, then we have four possible related possition cases.
	if(Rx > 0 && Ry > 0){
		for(int x = 1; x <= Rx; x++){
			//first one, normal one.
			int y = ((double)Ry) / ((double)Rx) * x + repairedByStatus[statusBegin].y;
			cv::Point aPoint;
			aPoint.x = x + repairedByStatus[statusBegin].x;
			aPoint.y = y;
			AddNewNode(currentNode,i,aPoint);
		}
	}

	if(Rx < 0 && Ry > 0){
		for(int x = -1; x >= Rx; x--){
			//first one, normal one.
			int y = ((double)Ry) / ((double)Rx) * x + repairedByStatus[statusBegin].y;
			cv::Point aPoint;
			aPoint.x = x + repairedByStatus[statusBegin].x;
			aPoint.y = y;
			AddNewNode(currentNode,i,aPoint);
		}
	}
	
	if(Rx < 0 && Ry < 0){
		for(int x = -1; x >= Rx; x--){
			//first one, normal one.
			int y = ((double)Ry) / ((double)Rx) * x + repairedByStatus[statusBegin].y;
			cv::Point aPoint;
			aPoint.x = x + repairedByStatus[statusBegin].x;
			aPoint.y = y;
			AddNewNode(currentNode,i,aPoint);
		}
	}
		
	if(Rx > 0 && Ry < 0){
		for(int x=1;x<=Rx;x++){
			//first one, normal one.
			int y = ((double)Ry) / ((double)Rx) * x + repairedByStatus[statusBegin].y;
			cv::Point aPoint;
			aPoint.x = x + repairedByStatus[statusBegin].x;
			aPoint.y = y;
			AddNewNode(currentNode,i,aPoint);
		}
	}
}

void AddNewNode(int possition, int i, cv::Point &newNode){
	contours[i].insert(contours[i].begin() + possition, newNode);
}

void NodeDelete(int possition, int i){
	contours[i].erase(contours[i].begin() + possition);
}