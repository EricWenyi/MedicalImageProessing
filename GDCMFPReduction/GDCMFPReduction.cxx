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
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	using namespace cv;

	struct APoint{
		int c;
		int x;
		int y;
		int label;
	};
	APoint apoint;
	vector<APoint> temp1;
	vector<APoint> temp2;
	vector<vector<APoint>> points;
	vector<int> labelToRemain;

	struct AContour{
		int n;//contour的序号，后为contour标记label时会用到
		int label;
		double area;
		double perimeter;
		double boundingArea;//minAreaRect的面积
		double a;//fitEllipse的长
		double b;//fitEllipse的宽
		int x;//boundingRect的长
		int y;//boundingRect的宽
		vector<Point> point;//将属于同一contour的点都推进去
	};
	AContour acontour;
	vector<AContour> temp;
	vector<vector<AContour>> contour;

	int labelCounter = 0;
	int zeroCounter = 0;
	int nowC = -1;
	int nowL = -1;

	typedef itk::ConstNeighborhoodIterator< ImageType3D > NeighborhoodIteratorType;
	NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(1);
	NeighborhoodIteratorType it( radius, originImage3D, originImage3D->GetLargestPossibleRegion() );
	ImageType3D::IndexType location;

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
		findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0) );
		
		Mat drawing = Mat::zeros( img.size(), CV_8UC1 );

		if(sliceIndex[2] == 0){
			for(int i = 0; i < contours.size(); i++){
				acontour.n = i;
				acontour.label = labelCounter;
				acontour.area = contourArea(contours[i]);
				acontour.perimeter = arcLength(contours[i], true);
				//contour内的点不足5个时fitEllipse无法使用
				if(contours[i].size() < 5){
					acontour.a = -2.9f;
					acontour.b = -1.0f;
				} else {
					acontour.a = fitEllipse(contours[i]).size.height;
					acontour.b = fitEllipse(contours[i]).size.width;
				}

				acontour.boundingArea = minAreaRect(contours[i]).size.area();
				acontour.x = boundingRect(contours[i]).height;
				acontour.y = boundingRect(contours[i]).width;

				for(int j = 0; j < contours[i].size(); j++){
					apoint.c = i;
					apoint.x = contours[i][j].x;
					apoint.y = contours[i][j].y;
					apoint.label = labelCounter;
					temp1.push_back(apoint);
					acontour.point.push_back(contours[i][j]);//将当前contour内的全部点保存于结构体中
				}

				labelCounter++;
				temp.push_back(acontour);
			}
		} else {
			for(int i = 0; i < contours.size(); i++){
				acontour.n = i;
				acontour.label = -1;
				acontour.area = contourArea(contours[i]);
				acontour.perimeter = arcLength(contours[i], true);
				
				if(contours[i].size() < 5){
					acontour.a = -2.9f;
					acontour.b = -1.0f;
				} else {
					acontour.a = fitEllipse(contours[i]).size.height;
					acontour.b = fitEllipse(contours[i]).size.width;
				}
				
				acontour.boundingArea = minAreaRect(contours[i]).size.area();
				acontour.x = boundingRect(contours[i]).height;
				acontour.y = boundingRect(contours[i]).width;

				for(int j = 0; j < contours[i].size(); j++){
					apoint.c = i;
					apoint.x = contours[i][j].x;
					apoint.y = contours[i][j].y;
					apoint.label = -1;
					temp2.push_back(apoint);
					acontour.point.push_back(contours[i][j]);
				}

				temp.push_back(acontour);
			}
	
			for(int i = 0; i < temp2.size(); i++){
				location[0] = temp2[i].x;
				location[1] = temp2[i].y;
				it.SetLocation(location);

				int counter = -1;

				for(int j = 0; j < 9; j++){
					if(it.GetPixel(j) != 0){
						for(int k = 0; k < temp1.size(); k++){
							if(temp1[k].x == it.GetIndex(j)[0] && temp1[k].y == it.GetIndex(j)[1]){
								if(counter != 0){
									temp2[i].label = temp1[k].label;
									counter++;
								} else if(temp2[i].label != temp1[k].label) {
									for(int l = 0; l < temp1.size(); l++){
										if(temp1[l].label == temp1[k].label){
											temp1[l].label = temp2[i].label;
										}
									}
								}
							}
						}
					} else {
						zeroCounter++;
					}
				}

				counter = -1;

				if(zeroCounter == 9){
					temp2[i].label = labelCounter;
					labelCounter++;
				}

				zeroCounter = 0;

				if(nowC != temp2[i].c){
					nowC = temp2[i].c;
					nowL = temp2[i].label;
					for(int j = 0; j < temp.size(); j++){
						if(temp[j].n == nowC){
							temp[j].label = nowL;
						}
					}
				} else if(nowL != temp2[i].label) {
					temp2[i].label = nowL;
				}
			}

			nowC = -1;
			nowL = -1;

			points.push_back(temp1);
			contour.push_back(temp);
			
			if(sliceIndex[2] == inIterator.GetRegion().GetSize()[2] - 1){
				points.push_back(temp2);
				contour.push_back(temp);
			} else {
				temp1.clear();
				temp1.resize(temp2.size());
				memcpy(&temp1[0], &temp2[0], temp2.size() * sizeof(APoint));
				temp2.clear();
				temp.clear();
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
		joinSeries->PushBackInput( itkDrawing );
	}

	struct AnObject{
		int label;
		int z;//3D物体Z轴大小
		int mC;//3D物体的最大圆度
		vector<AContour> contour;//将属于同一物体的contour推进去
	};
	AnObject anobject;
	vector<AnObject> objects;
	int tempCounter = 0;

	for(int i = 0; i < contour.size(); i++){
		if(i == 0){
			for(int j = 0; j < contour[i].size(); j++){
				anobject.label = contour[i][j].label;
				anobject.z = 1;
				anobject.mC = -1;
				anobject.contour.push_back(contour[i][j]);
				objects.push_back(anobject);
			}
		} else {
			for(int j = 0; j < contour[i].size(); j++){
				for(int k = 0; k < objects.size(); k++){
					if(contour[i][j].label == objects[k].label){
						objects[k].z++;
						anobject.contour.push_back(contour[i][j]);
						if(4 * 3.1415926f * contour[i][j].area / contour[i][j].perimeter / contour[i][j].perimeter > objects[k].mC){
							objects[k].mC = 4 * 3.1415926f * contour[i][j].area / contour[i][j].perimeter / contour[i][j].perimeter;
						}
					} else {
						tempCounter++;
					}
				}

				if(tempCounter == objects.size()){
					anobject.label = contour[i][j].label;
					anobject.z = 1;
					anobject.mC = -1;
					anobject.contour.push_back(contour[i][j]);
					objects.push_back(anobject);
				}

				tempCounter = 0;
			}
		}
	}

	std::cout<<objects.size()<<std::endl;

	//4个remain分别为4次reduction的结果，层层递进的，下一个remain一定是前面的子集
	vector<int> remain1, remain2, remain3, remain4;

	for(int i = 0; i < objects.size(); i++){
		if(objects[i].z >= 3){
			for(int j = 0; j < objects[i].contour.size(); j++){
				if(objects[i].contour[j].x >= 3 && objects[i].contour[j].y >= 3){
					if(nowL != objects[i].label){
						remain1.push_back(objects[i].label);
						nowL = objects[i].label;
					}
				}
			}
		}
	}

	nowL = -1;

	for(int i = 0; i < objects.size(); i++){
		if(objects[i].mC > 0.2f){
			for(int j = 0; j < remain1.size(); j++){
				if(objects[i].label == remain1[j]){
					if(nowL != objects[i].label){
						remain2.push_back(objects[i].label);
						nowL = objects[i].label;
					}
				}
			}
		}
	}

	nowL = -1;

	for(int i = 0; i < objects.size(); i++){
		for(int j = 0; j < objects[i].contour.size(); j++){
			if(objects[i].contour[j].boundingArea / objects[i].contour[j].area <= 2.0f){
				tempCounter++;
			}
		}
		
		if(tempCounter == objects[i].contour.size()){
			for(int j = 0; j < remain2.size(); j++){
				if(objects[i].label == remain2[j]){
					remain3.push_back(objects[i].label);
				}
			}
		}
		
		tempCounter = 0;
	}
	
	for(int i = 0; i < objects.size(); i++){
		for(int j = 0; j < objects[i].contour.size(); j++){
			if(objects[i].contour[j].a / objects[i].contour[j].b <= 3.0f){
				tempCounter++;
			}
		}
		
		if(tempCounter == objects[i].contour.size()){
			for(int j = 0; j < remain3.size(); j++){
				if(objects[i].label == remain3[j]){
					remain4.push_back(objects[i].label);
				}
			}
		}
		
		tempCounter = 0;
	}

	std::cout<<remain1.size()<<std::endl;
	std::cout<<remain2.size()<<std::endl;
	std::cout<<remain3.size()<<std::endl;
	std::cout<<remain4.size()<<std::endl;

	//2D特征提取，以下皆为各变量的最大值
	vector<double> area, perimeter, circularity, a, b, eccentricity;
	double max = -1.0f;

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(objects[remain4[i]].contour[j].area > max){
				max = objects[remain4[i]].contour[j].area;
			}
		}
		area.push_back(max);
	}

	max = -1;

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(objects[remain4[i]].contour[j].perimeter > max){
				max = objects[remain4[i]].contour[j].perimeter;
			}
		}
		perimeter.push_back(max);
	}

	for(int i = 0; i < remain4.size(); i++){
		circularity.push_back(objects[remain4[i]].mC);
	}

	max = -1;

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(objects[remain4[i]].contour[j].a > max){
				max = objects[remain4[i]].contour[j].a;
			}
		}
		a.push_back(max);
	}

	max = -1;

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(objects[remain4[i]].contour[j].b > max){
				max = objects[remain4[i]].contour[j].b;
			}
		}
		b.push_back(max);
	}

	max = -1;

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(sqrt(1 - (objects[remain4[i]].contour[j].b * objects[remain4[i]].contour[j].b / objects[remain4[i]].contour[j].a / objects[remain4[i]].contour[j].a)) > max){
				max = sqrt(1 - (objects[remain4[i]].contour[j].b * objects[remain4[i]].contour[j].b / objects[remain4[i]].contour[j].a / objects[remain4[i]].contour[j].a));
			}
		}
		eccentricity.push_back(max);
	}

	max = -1;

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
