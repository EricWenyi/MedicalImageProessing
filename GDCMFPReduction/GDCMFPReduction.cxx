#include "itkGDCMImageIO.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkOpenCVImageBridge.h"

#include "minicsv.h"

int main( int argc, char* argv[] ){
	if( argc < 5 ){
		std::cerr << "Usage: " << argv[0] << "noduleImageFile originImageFile outputImageFile sliceThickness" << std::endl;
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
		Point2f centroid;
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
				aContour.centroid.x = 0.0f;
				aContour.centroid.y = 0.0f;

				for(int j = 0; j < contour[i].size(); j++){
					aPoint.c = i;
					aPoint.x = contour[i][j].x;
					aPoint.y = contour[i][j].y;
					aContour.centroid.x += aPoint.x;
					aContour.centroid.y += aPoint.y;
					aPoint.label = labelCounter;
					temp1.push_back(aPoint);
					aContour.point.push_back(contour[i][j]);
				}

				labelCounter++;
				aContour.centroid.x /= contour[i].size();
				aContour.centroid.y /= contour[i].size();
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
				aContour.centroid.x = 0.0f;
				aContour.centroid.y = 0.0f;

				for(int j = 0; j < contour[i].size(); j++){
					aPoint.c = i;
					aPoint.x = contour[i][j].x;
					aPoint.y = contour[i][j].y;
					aContour.centroid.x += aPoint.x;
					aContour.centroid.y += aPoint.y;
					aPoint.label = -1;
					temp2.push_back(aPoint);
					aContour.point.push_back(contour[i][j]);
				}

				aContour.centroid.x /= contour[i].size();
				aContour.centroid.y /= contour[i].size();
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
				
				vector<int> labels;

				if(nowC != temp2[i].c){
					if(nowC != -1){
						for(int j = 0; j < temp3[nowC].point.size(); j++){
							temp2[i - 1 - j].label = temp3[nowC].label;
						}
					}

					for(int j = 0; j < temp1.size(); j++){
						for(int k = 1; k < labels.size(); k++){
							if(temp1[j].label == labels[k]){
								temp1[j].label = labels[0];
							}
						}
					}

					nowC = temp2[i].c;
					zeroCounter = 0;

					if(temp2[i].label == -1){
						zeroCounter++;
					} else {
						temp3[nowC].label = temp2[i].label;
						labels.push_back(temp3[nowC].label);
					}
				} else if (temp2[i].label == -1){
					zeroCounter++;
					if(zeroCounter == temp3[nowC].point.size()){
						temp3[nowC].label = labelCounter;
						labelCounter++;
					}
				} else {
					if (temp3[nowC].label == -1){
						temp3[nowC].label = temp2[i].label;
						labels.push_back(temp3[nowC].label);
					}

					for(int j = 0; j < labels.size(); j++){
						if(temp2[i].label != labels[j]){
							labels.push_back(temp2[i].label);
						}
					}
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
		double mArea;
		double mP;
		double mC;
		double mA;
		double mB;
		double mE;
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
				anObject.mArea = contours[i][j].area;
				anObject.mP = contours[i][j].perimeter;
				anObject.mC = 4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter;
				anObject.mA = contours[i][j].a;
				anObject.mB = contours[i][j].b;
				anObject.mE = sqrt(1 - (contours[i][j].b * contours[i][j].b / contours[i][j].a / contours[i][j].a));
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

						if(contours[i][j].area > objects[k].mArea){
							objects[k].mArea = contours[i][j].area;
						}

						if(contours[i][j].perimeter > objects[k].mP){
							objects[k].mP = contours[i][j].perimeter;
						}

						if(4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter > objects[k].mC){
							objects[k].mC = 4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter;
						}

						if(contours[i][j].a > objects[k].mA){
							objects[k].mA = contours[i][j].a;
						}

						if(contours[i][j].b > objects[k].mB){
							objects[k].mB = contours[i][j].b;
						}

						if(sqrt(1 - (contours[i][j].b * contours[i][j].b / contours[i][j].a / contours[i][j].a)) > objects[k].mE){
							objects[k].mE = sqrt(1 - (contours[i][j].b * contours[i][j].b / contours[i][j].a / contours[i][j].a));
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
					anObject.mArea = contours[i][j].area;
					anObject.mP = contours[i][j].perimeter;
					anObject.mC = 4 * 3.1415926f * contours[i][j].area / contours[i][j].perimeter / contours[i][j].perimeter;
					anObject.mA = contours[i][j].a;
					anObject.mB = contours[i][j].b;
					anObject.mE = sqrt(1 - (contours[i][j].b * contours[i][j].b / contours[i][j].a / contours[i][j].a));
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

	vector<int> remain1, remain2, remain3, remain4, remain5;
	
	for(int i = 0; i < objects.size(); i++){
		if(objects[i].z >= 2){
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
	
	for(int i = 0; i < remain1.size(); i++){
		if(objects[remain1[i]].mC > 0.2f){
			remain2.push_back(remain1[i]);
		}
	}

	for(int i = 0; i < remain2.size(); i++){
		for(int j = 0; j < objects[remain2[i]].contour.size(); j++){
			if(objects[remain2[i]].contour[j].boundingArea / objects[remain2[i]].contour[j].area <= 2.0f){
				tempCounter++;
			}
		}
		
		if(tempCounter == objects[remain2[i]].contour.size()){
			remain3.push_back(remain2[i]);
		}
		
		tempCounter = 0;
	}
	
	for(int i = 0; i < remain3.size(); i++){
		for(int j = 0; j < objects[remain3[i]].contour.size(); j++){
			if(objects[remain3[i]].contour[j].a / objects[remain3[i]].contour[j].b <= 3.0f){
				tempCounter++;
			}
		}
		
		if(tempCounter == objects[remain3[i]].contour.size()){
			remain4.push_back(remain3[i]);
		}
		
		tempCounter = 0;
	}
	
	Point2f tempCentroid1(-1, -1);
	Point2f tempCentroid2(-1, -1);

	for(int i = 0; i < remain4.size(); i++){
		for(int j = 0; j < objects[remain4[i]].contour.size(); j++){
			if(j == 0){
				tempCentroid1.x = objects[remain4[i]].contour[j].centroid.x;
				tempCentroid1.y = objects[remain4[i]].contour[j].centroid.y;
			} else {
				tempCentroid2.x = objects[remain4[i]].contour[j].centroid.x;
				tempCentroid2.y = objects[remain4[i]].contour[j].centroid.y;

				if(sqrt((tempCentroid1.x - tempCentroid2.x) * (tempCentroid1.x - tempCentroid2.x) + (tempCentroid1.y - tempCentroid2.y) * (tempCentroid1.y - tempCentroid2.y)) < 3.0f){
					tempCounter++;
				}

				tempCentroid1.x = tempCentroid2.x;
				tempCentroid1.y = tempCentroid2.y;
				tempCentroid2.x = -1;
				tempCentroid2.y = -1;
			}
		}

		if(tempCounter == objects[remain4[i]].contour.size() - 1){
			remain5.push_back(remain4[i]);
		}

		tempCounter = 0;
	}

	std::cout<<remain1.size()<<std::endl;
	std::cout<<remain2.size()<<std::endl;
	std::cout<<remain3.size()<<std::endl;
	std::cout<<remain4.size()<<std::endl;
	std::cout<<remain5.size()<<std::endl;

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
		vector<int> index;

		for(int i = 0; i < remain5.size(); i++){
			for(int j = 0; j < objects[remain5[i]].contour.size(); j++){
				if(objects[remain5[i]].contour[j].slice == sliceIndex[2]){
					contourToDraw.push_back(objects[remain5[i]].contour[j].point);
					index.push_back(i);
				}
			}
		} 
		
		for(int i = 0; i < contourToDraw.size(); i++){
			drawContours(drawing, contourToDraw, i, Scalar(index[i] + 1), CV_FILLED, 8, noArray(), 0, Point(0, 0));
		}

		for(int i = 0; i < drawing.cols; i++){
			for(int j = 0; j < drawing.rows; j++){
				if(drawing.at<unsigned short>(j, i) != 0){
					objects[remain5[drawing.at<unsigned short>(j, i) - 1]].agv += origin.at<signed short>(j, i);
					objects[remain5[drawing.at<unsigned short>(j, i) - 1]].count++;
				}
			}
		}
	}
	
	for(int i = 0; i < remain5.size(); i++){
		objects[remain5[i]].agv /= objects[remain5[i]].count;
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
		vector<int> index;

		for(int i = 0; i < remain5.size(); i++){
			for(int j = 0; j < objects[remain5[i]].contour.size(); j++){
				if(objects[remain5[i]].contour[j].slice == sliceIndex[2]){
					contourToDraw.push_back(objects[remain5[i]].contour[j].point);
					index.push_back(i);
				}
			}
		}

		for(int i = 0; i < contourToDraw.size(); i++){
			drawContours(drawing, contourToDraw, i, Scalar(index[i] + 1), CV_FILLED, 8, noArray(), 0, Point(0, 0));
		}

		for(int i = 0; i < drawing.cols; i++){
			for(int j = 0; j < drawing.rows; j++){
				if(drawing.at<unsigned short>(j, i) != 0){
					objects[remain5[drawing.at<unsigned short>(j, i) - 1]].sd += (origin.at<signed short>(j, i) - objects[remain5[drawing.at<unsigned short>(j, i) - 1]].agv) * (origin.at<signed short>(j, i) - objects[remain5[drawing.at<unsigned short>(j, i) - 1]].agv);
				}
			}
		}
	}

	for(int i = 0; i < remain5.size(); i++){
		objects[remain5[i]].sd = sqrt(objects[remain5[i]].sd / objects[remain5[i]].count);
	}

	vector<double> volume, surfaceArea, agv, sd, area, perimeter, circularity, a, b, eccentricity;

	for(int i = 0; i < remain5.size(); i++){
		for(int j = 0; j < objects[remain5[i]].contour.size(); j++){
			objects[remain5[i]].volume += atof(argv[4]) * objects[remain5[i]].contour[j].area;
		}
	}

	for(int i = 0; i < remain5.size(); i++){
		for(int j = 0; j < objects[remain5[i]].contour.size(); j++){
			if (j == 0 || j == objects[remain5[i]].contour.size() - 1){
				objects[remain5[i]].surfaceArea += objects[remain5[i]].contour[j].area;
			} else {
				objects[remain5[i]].surfaceArea += atof(argv[4]) * objects[remain5[i]].contour[j].perimeter;
			}
		}
	}

	for(int i = 0; i < remain5.size(); i++){
		volume.push_back(objects[remain5[i]].volume);
		surfaceArea.push_back(objects[remain5[i]].surfaceArea);
		agv.push_back(objects[remain5[i]].agv);
		sd.push_back(objects[remain5[i]].sd);
		area.push_back(objects[remain5[i]].mArea);
		perimeter.push_back(objects[remain5[i]].mP);
		circularity.push_back(objects[remain5[i]].mC);
		a.push_back(objects[remain5[i]].mA);
		b.push_back(objects[remain5[i]].mB);
		eccentricity.push_back(objects[remain5[i]].mE);
	}

	vector<AnObject> remains;
	vector<int> labels(remain5.size());

	for(int i = 0; i < remain5.size(); i++){
		remains.push_back(objects[remain5[i]]);
		labels[i] = 0;
	}
	
	int slice = 0, X = 0, Y = 0;
	
	//input patients' nodule information

	for(int i = 0; i < remains.size(); i++){
		for(int j = 0; j < remains[i].contour.size(); j++){
			if(remains[i].contour[j].slice == slice){
				if(pointPolygonTest(remains[i].contour[j].point, Point(X, Y), false) != -1){
					labels[i] = 1;
				}
			}
		}
	}
	
	csv::ofstream os("C:\\downloads\\features.csv");
	os.set_delimiter(',', "EOF");
	for(int i = 0; i < remain5.size(); i++){
		os << volume[i] << surfaceArea[i] << agv[i] << sd[i] << area[i] << perimeter[i] << circularity[i] << a[i] << b[i] << eccentricity[i] << labels[i] << NEWLINE;
	}
	os.flush();

	//LDA

	//draw the result

	for( originIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = originIterator.GetIndex();
		printf( "Slice Index --- %d ---\n", sliceIndex[2] );
		ExtractFilterType::InputImageRegionType::SizeType sliceSize = originIterator.GetRegion().GetSize();
		sliceSize[2] = 0;
		
		ExtractFilterType::InputImageRegionType sliceRegion = originIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );

		ExtractFilterType::Pointer extractor = ExtractFilterType::New();
		extractor->SetExtractionRegion( sliceRegion );
		extractor->InPlaceOn();
		extractor->SetDirectionCollapseToSubmatrix();
		extractor->SetInput( originReader->GetOutput() );

		try{
			extractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: extractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
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


