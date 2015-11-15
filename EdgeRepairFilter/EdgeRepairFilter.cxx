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

// includes from OpenCV
#include "cv.h"
#include <math.h>

int main( int argc, char* argv[] ){
	if( argc < 3 ){
		std::cerr << "Usage: " << argv[0] << " InputImageFile OutputImageFile" << std::endl;
		return EXIT_FAILURE;
    }
	//三维图像类型
	typedef signed short PixelType3D;
	const unsigned int Dimension3D = 3;
	typedef itk::Image< PixelType3D, Dimension3D > ImageType3D;
	
	//二维图像类型
	const unsigned int Dimension2D = 2;
	typedef unsigned char PixelType2D;//此处原为signed short，应设置为float类型，下文CurvatureFlowImageFilter所要求，否则将会弹窗报错至失去响应
	typedef itk::Image< PixelType2D, Dimension2D > ImageType2D;


	//设定输入图像的类型
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//设定读取器类型及设定读取的图像类型和图像地址
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
	//设定切片迭代器
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType inIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	inIterator.SetFirstDirection(0);
	inIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(originImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(originImage3D->GetSpacing()[2]);

	for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice()){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();

		ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
		sliceSize[2] = 0;

		ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
		sliceRegion.SetSize( sliceSize );
		sliceRegion.SetIndex( sliceIndex );
		
		ExtractFilterType::Pointer inExtractor = ExtractFilterType::New();
		inExtractor->SetExtractionRegion( sliceRegion );
		inExtractor->InPlaceOn();
		inExtractor->SetDirectionCollapseToSubmatrix();//此处原为无，如果不实现此方法（此方法用于设定方向坍塌的模式，看源文件里头的内容，虽没什么用，但还是强制需要设置），否则下面更新迭代器时将会捕获到异常
		inExtractor->SetInput( originReader->GetOutput() );

		try{
			inExtractor->Update();//原此处报6010错误，添加try，catch后发现，上文有修复方法，于是将所有Update()方法都加了try，catch
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: inExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		
		using namespace cv;
		Mat img = itk::OpenCVImageBridge::ITKImageToCVMat< ImageType2D >( inExtractor->GetOutput() );
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		RNG rng(12345);
  /// Find contours
		findContours( img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );



  /// Draw contours
		Mat drawing = Mat::zeros( img.size(), CV_8UC3 );
		for( int i = 0; i< contours.size(); i++ )
		{
			 Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
			drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
		}

		//TODO
		//1.Check if it is a closed circle,by Eu distance of each neighbor pixels <=sqrt(2)
		//2.Sum of the on circle distance in two directions
		//3.Replace troublesome archs by lines
		struct ALine{
			int BeginX;
			int BeginY;
			int EndX;
			int EndY;
		};
		Vector<ALine> repairedByALine;

		for (int i = 0; i< contours.size(); i++)
		{
		    int closed = 1;
			for(int j = 0; j<(contours[i].size()-1);j++){
				//1. Check whether circle is closed
				if(abs(contours[i][j+1].x-contours[i][j].x)+abs(contours[i][j+1].y-contours[i][j].y)>2){
					closed = 0;
					break;
				}
			}
			if(closed == 0){
				break;
			}else{
				//2.Sum of the on circle distance in two directions
				
				    float *disClockWise;
					float **disPixel;



					disClockWise = (float *)malloc(sizeof(float)*contours[i].size()*contours[i].size());
					disPixel =  (float **)malloc(sizeof(float)*contours[i].size());
					for(i =0; i< contours[i].size();i++)
					{
						disPixel[i] = &disClockWise[i * contours[i].size()];
					}

					float *disCounterClockWise;
					float **disCounterPixel;



					disCounterClockWise = (float *)malloc(sizeof(float)*contours[i].size()*contours[i].size());
					disCounterPixel =  (float **)malloc(sizeof(float)*contours[i].size());
					for(i =0; i< contours[i].size();i++)
					{
						disCounterPixel[i] = &disCounterClockWise[i * contours[i].size()];
					}

					

					//...
					free(subpower);
					free(power);



				for(int j = 0; j<contours[i].size();j++){
					


					for(int k=0; k<contours[i].size();k++){
						if(j < k){
							for(int m = j; m<k-1 ;m++){
								if((contours[i][m+1].x != contours[i][m].x)||(contours[i][m+1].y!=contours[i][m].y)){
									disPixel[j][m]+=1.4142135f;
								}else{
									disPixel[j][m]++;
								}
							}
							for(int m=0; m<j-1; m++){
								if((contours[i][m+1].x != contours[i][m].x)||(contours[i][m+1].y!=contours[i][m].y)){
									disCounterPixel[j][m]+=1.4142135f;
								}else{
									disCounterPixel[j][m]++;
								}
							}
							for(int m=k;m<contours[i].size()-1;m++){
								if((contours[i][m+1].x != contours[i][m].x)||(contours[i][m+1].y!=contours[i][m].y)){
									disCounterPixel[j][m]+=1.4142135f;
								}else{
									disCounterPixel[j][m]++;
								}
							}
						}else if(j > k){
							for(int m = j;m<contours[i].size();m++){
								disPixel[j][m] = disPixel[m][j];
							}
						}else{
							disPixel[j][k] = 0;
						}
					}
				}

				//3.Replace troublesome archs by lines
				for(int j = 0; j<contours[i].size();j++){
					for(int k=0; k<contours[i].size();k++){
						if(disPixel[j][k]<disCounterPixel[j][k]){
							float disOfjk = (float)sqrt((double)((contours[i][j].x - contours[i][k].x)*(contours[i][j].x - contours[i][k].x)+(contours[i][j].y - contours[i][k].y)*(contours[i][j].y - contours[i][k].y)));

							if(disCounterPixel[j][k]/disOfjk>1.5){
								//Re
								ALine aline;
								aline.BeginX=contours[i][j].x;
								aline.BeginY=contours[i][j].y;
								aline.EndX=contours[i][k].x;
								aline.EndY=contours[i][k].y;
								repairedByALine.push_back(aline);// aline is copied into vector, different from java's storage of pointer.
							}
						}/*
						else if(disPixel[j][k]>disCounterPixel[j][k]){
							;
						}else{
							;
						}*/ //have nonsense due to symmetry
			}
		}
		
		ImageType2D::Pointer itkDrawing;
		try{
		itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage<ImageType2D>(drawing);
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVtoITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		joinSeries->PushBackInput(itkDrawing );
}

  

	try{
		joinSeries->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: joinSeries->Update(); caught !" << std::endl;
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
