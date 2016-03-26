#include "itkGDCMImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkOpenCVImageBridge.h"

int main( int argc, char* argv[] ){
	if( argc < 4 ){
		std::cerr << "Usage: " << argv[0] << "noduleImageFile originImageFile outputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::Image< signed short, 3 > ImageType3D;
	typedef itk::Image< signed short, 2 > ImageType2D;

	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer noduleReader = ReaderType::New();
	noduleReader->SetImageIO( gdcmIO );
	noduleReader->SetFileName( argv[1] );

	ReaderType::Pointer originReader = ReaderType::New();
	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[2] );

	try{
		noduleReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: noduleReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	try{
		originReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: originReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer noduleImage3D = noduleReader->GetOutput();
	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();

	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType noduleIterator( noduleImage3D, noduleImage3D->GetLargestPossibleRegion() );
	noduleIterator.SetFirstDirection( 0 );
	noduleIterator.SetSecondDirection( 1 );

	SliceIteratorType originIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	originIterator.SetFirstDirection( 0 );
	originIterator.SetSecondDirection( 1 );

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( noduleImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( noduleImage3D->GetSpacing()[2] );

	using namespace cv;

	struct APoint{
		int c;
		int x;
		int y;
		int label;
	};
	APoint aPoint;
	vector<APoint> temp1;
	vector<APoint> temp2;
	vector<vector<APoint>> points;

	struct AContour{
		int label;
		int slice;
		double area;
		double perimeter;
		double boundingArea;
		double a;
		double b;
		int x;
		int y;
		vector<Point> point;
	};
	AContour aContour;
	vector<AContour> temp3;
	vector<vector<AContour>> contours;

	int labelCounter = 0;
	int zeroCounter = 0;
	int nowC = -1;
	int nowL = -1;

	typedef itk::ConstNeighborhoodIterator< ImageType3D > NeighborhoodIteratorType;
	NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(1);
	NeighborhoodIteratorType it( radius, noduleImage3D, noduleImage3D->GetLargestPossibleRegion() );
	ImageType3D::IndexType location;

	for( noduleIterator.GoToBegin(); !noduleIterator.IsAtEnd(); noduleIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = noduleIterator.GetIndex();
		location[2] = sliceIndex[2];
		printf( "Slice Index --- %d ---\n", sliceIndex[2] );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = noduleIterator.GetRegion().GetSize();
		sliceSize[2] = 0;
		
		ExtractFilterType::InputImageRegionType sliceRegion = noduleIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );

		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( noduleReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( extractor->GetOutput() );
		img.convertTo(img, CV_8UC1);

		vector<vector<Point>> contour;
		vector<Vec4i> hierarchy;
		findContours( img, contour, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0) );

		/*
		vector<vector<Point>> set(contour.size());
		for(int i = 0; i < img.cols; i++){
			for(int j = 0; j < img.rows; j++){
				if(img.at<uchar>(j, i) != 0){
					for(int k = 0; k < contour.size(); k++){
						if(pointPolygonTest(contour[k], Point(j, i), false) != -1){
							set[k].push_back(Point(j, i));
						}
					}
				}
			}
		}
		*/

		if(sliceIndex[2] == 0){
			for(int i = 0; i < contour.size(); i++){
				aContour.label = labelCounter;
				aContour.slice = sliceIndex[2];
				aContour.area = contourArea(contour[i]);
				aContour.perimeter = arcLength(contour[i], true);
				
				if(contour[i].size() < 5){
					aContour.a = -3.0f;
					aContour.b = -1.0f;
				} else {
					aContour.a = fitEllipse(contour[i]).size.height;
					aContour.b = fitEllipse(contour[i]).size.width;
				}

				aContour.boundingArea = minAreaRect(contour[i]).size.area();
				aContour.x = boundingRect(contour[i]).height;
				aContour.y = boundingRect(contour[i]).width;

				for(int j = 0; j < contour[i].size(); j++){
					aPoint.c = i;
					aPoint.x = contour[i][j].x;
					aPoint.y = contour[i][j].y;
					aPoint.label = labelCounter;
					temp1.push_back(aPoint);
					aContour.point.push_back(contour[i][j]);
				}

				labelCounter++;
				temp3.push_back(aContour);
				aContour.point.clear();
			}

			contours.push_back(temp3);
			temp3.clear();
		} else {
			for(int i = 0; i < contour.size(); i++){
				aContour.label = -1;
				aContour.slice = sliceIndex[2];
				aContour.area = contourArea(contour[i]);
				aContour.perimeter = arcLength(contour[i], true);
				
				if(contour[i].size() < 5){
					aContour.a = -3.0f;
					aContour.b = -1.0f;
				} else {
					aContour.a = fitEllipse(contour[i]).size.height;
					aContour.b = fitEllipse(contour[i]).size.width;
				}
				
				aContour.boundingArea = minAreaRect(contour[i]).size.area();
				aContour.x = boundingRect(contour[i]).height;
				aContour.y = boundingRect(contour[i]).width;

				for(int j = 0; j < contour[i].size(); j++){
					aPoint.c = i;
					aPoint.x = contour[i][j].x;
					aPoint.y = contour[i][j].y;
					aPoint.label = -1;
					temp2.push_back(aPoint);
					aContour.point.push_back(contour[i][j]);
				}

				temp3.push_back(aContour);
				aContour.point.clear();
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
								} else if (temp2[i].label != temp1[k].label){
									for(int l = 0; l < temp1.size(); l++){
										if(temp1[l].label == temp1[k].label){
											temp1[l].label = temp2[i].label;
										}
									}
								}
							}
						}
					}
				}

				counter = -1;

				if(nowC != temp2[i].c){
					nowC = temp2[i].c;
					zeroCounter = 0;
					if(temp2[i].label == -1){
						zeroCounter++;
					} else {
						temp3[nowC].label = temp2[i].label;
					}
				} else if (temp2[i].label == -1){
					zeroCounter++;
					if(zeroCounter == temp3[nowC].point.size()){
						temp3[nowC].label = labelCounter;
						labelCounter++;
					}
				} else if (temp3[nowC].label == -1){
					temp3[nowC].label = temp2[i].label;
				}
			}

			nowC = -1;

			points.push_back(temp1);
			contours.push_back(temp3);
			
			if(sliceIndex[2] == noduleIterator.GetRegion().GetSize()[2] - 1){
				points.push_back(temp2);
			} else {
				temp1.clear();
				temp1.resize(temp2.size());
				memcpy(&temp1[0], &temp2[0], temp2.size() * sizeof(APoint));
				temp2.clear();
				temp3.clear();
			}
		}
	}

	struct AnObject{
		int label;
		int z;
		int count;
		double mC;
		double volume;
		double surfaceArea;
		double agv;
		double sd;
		vector<AContour> contour;
	};
	AnObject anObject;
	vector<AnObject> objects;
	int tempCounter = 0;
	int counter = 0;
	
	for(int i = 0; i < contours.size(); i++){
		if(i == 0){
			for(int j = 0; j < contours[i].size(); j++){
				anObject.label = contours[i][j].label;
				anObject.z = 1;
				anObject.count = 0;
				anObject.mC = -1;
				anObject.volume = 0.0f;
				anObject.surfaceArea = 0.0f;
				anObject.agv = 0.0f;
				anObject.sd = 0.0f;
				anObject.contour.push_back(contours[i][j]);
				objects.push_back(anObject);
				anObject.contour.clear();
			}
		} else {
			for(int j = 0; j < contours[i].size(); j++){
				for(int k = 0; k < objects.size(); k++){
					if(contours[i][j].label == objects[k].label){
						if(counter == 0){
							objects[k].z++;
							counter++;
						} else {
							objects[k].z = 1;
						}
						
						objects[k].contour.push_back(contours[i][j]);
						if(4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter > objects[k].mC){
							objects[k].mC = 4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter;
						}
					} else {
						tempCounter++;
					}
				}

				counter = 0;

				if(tempCounter == objects.size()){
					anObject.label = contours[i][j].label;
					anObject.z = 1;
					anObject.count = 0;
					anObject.mC = -1;
					anObject.volume = 0.0f;
					anObject.surfaceArea = 0.0f;
					anObject.agv = 0.0f;
					anObject.sd = 0.0f;
					anObject.contour.push_back(contours[i][j]);
					objects.push_back(anObject);
					anObject.contour.clear();
				}

				tempCounter = 0;
			}
		}
	}

	std::cout<<objects.size()<<std::endl;

	vector<int> remain1, remain2, remain3, remain4;

	for(int i = 0; i < objects.size(); i++){
		if(objects[i].z >= 3){
			for(int j = 0; j < objects[i].contour.size(); j++){
				if(objects[i].contour[j].x >= 3 && objects[i].contour[j].y >= 3){
					if(nowL != i){
						remain1.push_back(i);
						nowL = i;
					}
				}
			}
		}
	}
	
	nowL = -1;

	for(int i = 0; i < objects.size(); i++){
		if(objects[i].mC > 0.2f){
			for(int j = 0; j < remain1.size(); j++){
				if(i == remain1[j]){
					if(nowL != i){
						remain2.push_back(i);
						nowL = i;
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
				if(i == remain2[j]){
					remain3.push_back(i);
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
				if(i == remain3[j]){
					remain4.push_back(i);
				}
			}
		}
		
		tempCounter = 0;
	}
	
	std::cout<<remain1.size()<<std::endl;
	std::cout<<remain2.size()<<std::endl;
	std::cout<<remain3.size()<<std::endl;
	std::cout<<remain4.size()<<std::endl;

	for( noduleIterator.GoToBegin(), originIterator.GoToBegin(); !noduleIterator.IsAtEnd(); noduleIterator.NextSlice(), originIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = noduleIterator.GetIndex();
		printf( "Slice Index --- %d ---\n", sliceIndex[2] );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = noduleIterator.GetRegion().GetSize();
		sliceSize[2] = 0;
		
		ExtractFilterType::InputImageRegionType sliceRegion = noduleIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );

		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( noduleReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( extractor->GetOutput() );
		Mat drawing = Mat::zeros( img.size(), CV_16UC1 );

		extractor->SetInput( originReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		Mat origin = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( extractor->GetOutput() );

		vector<vector<Point>> contourToDraw;

		for(int i = 0; i < remain4.size(); i++){
			for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
				if(objects[remain4[i]].contour[j].slice == sliceIndex[2]){
					contourToDraw.push_back(objects[remain4[i]].contour[j].point);
				}
			}
		}

		for(int i = 0; i < remain4.size(); i++){
			drawContours(drawing, contourToDraw, i, Scalar(i + 1), CV_FILLED, 8, noArray(), 0, Point(0, 0));
		}

		contourToDraw.clear();

		for(int i = 0; i < drawing.cols; i++){
			for(int j = 0; j < drawing.rows; j++){
				if(drawing.at<unsigned short>(j, i) != 0){
					objects[remain4[drawing.at<unsigned short>(j, i) - 1]].agv += origin.at<signed short>(j, i);
					objects[remain4[drawing.at<unsigned short>(j, i) - 1]].count++;
				}
			}
		}
	}
	
	for(int i = 0; i < remain4.size(); i++){
		objects[remain4[i]].agv /= objects[remain4[i]].count;
	}

	for( noduleIterator.GoToBegin(), originIterator.GoToBegin(); !noduleIterator.IsAtEnd(); noduleIterator.NextSlice(), originIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = noduleIterator.GetIndex();
		printf( "Slice Index --- %d ---\n", sliceIndex[2] );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = noduleIterator.GetRegion().GetSize();
		sliceSize[2] = 0;
		
		ExtractFilterType::InputImageRegionType sliceRegion = noduleIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );

		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( noduleReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( extractor->GetOutput() );
		Mat drawing = Mat::zeros( img.size(), CV_16UC1 );

		extractor->SetInput( originReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		Mat origin = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( extractor->GetOutput() );

		vector<vector<Point>> contourToDraw;

		for(int i = 0; i < remain4.size(); i++){
			for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
				if(objects[remain4[i]].contour[j].slice == sliceIndex[2]){
					contourToDraw.push_back(objects[remain4[i]].contour[j].point);
				}
			}
		}

		for(int i = 0; i < remain4.size(); i++){
			drawContours(drawing, contourToDraw, i, Scalar(i + 1), CV_FILLED, 8, noArray(), 0, Point(0, 0));
		}

		contourToDraw.clear();

		for(int i = 0; i < drawing.cols; i++){
			for(int j = 0; j < drawing.rows; j++){
				if(drawing.at<unsigned short>(j, i) != 0){
					objects[remain4[drawing.at<unsigned short>(j, i) - 1]].sd += (origin.at<signed short>(j, i) - objects[remain4[drawing.at<unsigned short>(j, i) - 1]].agv) * (origin.at<signed short>(j, i) - objects[remain4[drawing.at<unsigned short>(j, i) - 1]].agv);
					objects[remain4[drawing.at<unsigned short>(j, i) - 1]].count++;
				}
			}
		}
		/*
		ImageType2D::Pointer itkDrawing;
		try{
			itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage< ImageType2D >( drawing );
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVMatToITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		joinSeries->PushBackInput( itkDrawing );
		*/
	}

	for(int i = 0; i < remain4.size(); i++){
		objects[remain4[i]].sd = sqrt(objects[remain4[i]].sd / objects[remain4[i]].count);
	}

	vector<double> area, perimeter, circularity, a, b, eccentricity, volume, surfaceArea, agv, sd;
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

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			objects[remain4[i]].volume += 0.801f * objects[remain4[i]].contour[j].area;
		}
	}

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if (j == 0 || j == objects[remain4[i]].contour.size() - 1){
				objects[remain4[i]].surfaceArea += objects[remain4[i]].contour[j].area;
			} else {
				objects[remain4[i]].surfaceArea += 0.801f * objects[remain4[i]].contour[j].perimeter;
			}
		}
	}

	for(int i = 0; i < remain4.size(); i++){
		volume.push_back(objects[remain4[i]].volume);
		surfaceArea.push_back(objects[remain4[i]].surfaceArea);
		agv.push_back(objects[remain4[i]].agv);
		sd.push_back(objects[remain4[i]].sd);
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
	writer->SetFileName( argv[3] );
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
