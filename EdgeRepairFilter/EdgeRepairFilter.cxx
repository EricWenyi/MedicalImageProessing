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

	int tempInCounter = 0;
	for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice()){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		printf(" Slice Index --- %d ---",tempInCounter++);
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





		//TODO
		//1.Check if it is a closed circle,by Eu distance of each neighbor pixels <=sqrt(2)
		//2.Sum of the on circle distance in two directions
		//3.Replace troublesome archs by lines
		struct ALine{
			int contour;
			int BeginX;
			int BeginY;
			int EndX;
			int EndY;
		};
		Vector<ALine> repairedByALine;
	try{
		for (int i = 0; i< contours.size(); i++)
		{
			int isContinuing = 1;
			for(int j = 0; j<(contours[i].size()-1);j++){
				//1. Check whether circle is closed
				if(abs(contours[i][j+1].x-contours[i][j].x)+abs(contours[i][j+1].y-contours[i][j].y)>2){
					printf("contours[%d] not continuing\n",i);
					isContinuing = 0;
					break;
				}
			}
			if(isContinuing == 0){
				printf("Skip contours[%d] \n",i);
				continue;
			}else{
			
				//2.Sum of the on circle distance in two directions
				    
				printf("Enter %d/%d of contours\n",i,contours.size());
				    float *disClockWise;
					float **disPixel;



					disClockWise = (float *)malloc(sizeof(float)*contours[i].size()*contours[i].size());
					disPixel =  (float **)malloc(sizeof(float)*contours[i].size());
					for(int a =0; a< contours[i].size();a++)
					{
						disPixel[a] = &disClockWise[a * contours[i].size()];
					}

					float *disCounterClockWise;
					float **disCounterPixel;



					disCounterClockWise = (float *)malloc(sizeof(float)*contours[i].size()*contours[i].size());
					disCounterPixel =  (float **)malloc(sizeof(float)*contours[i].size());
					for(int a =0; a< contours[i].size();a++)
					{
						disCounterPixel[a] = &disCounterClockWise[a* contours[i].size()];
					}

					
					for(int j =0;j<contours[i].size();j++){
						for(int  k = 0; k<contours[i].size();k++){
							disPixel[j][k]=0;
							disCounterPixel[j][k]=0;
						}
					}

					//...
	//				free(subpower);
	//				free(power);


				try{
				for(int j = 0; j<contours[i].size();j++){
					for(int k=0; k<contours[i].size();k++){
						if(j < k){
							for(int m = j; m<k ;m++){
								try{
								if((contours[i][m+1].x != contours[i][m].x)&&(contours[i][m+1].y!=contours[i][m].y)){ ////Wrong &&
									disPixel[j][k]+=1.4142135f; //Wrong k.
								}else{
									disPixel[j][k]+=1.0f;
								}
								}catch(...){
									printf("Error comes from for1\n");
									return EXIT_FAILURE;
								}
							}
							for(int m=0; m<j; m++){
								try{
								if((contours[i][m+1].x != contours[i][m].x)||(contours[i][m+1].y!=contours[i][m].y)){
									disCounterPixel[j][k]+=1.4142135f; //Wrong k
								}else{
									disCounterPixel[j][k]+=1.0f;
								}
								}catch(...){
									printf("Error comes from for2\n");
									return EXIT_FAILURE;
								}
							}
							for(int m=k;m<contours[i].size()-1;m++){
								try{
								if((contours[i][m+1].x != contours[i][m].x)||(contours[i][m+1].y!=contours[i][m].y)){
									disCounterPixel[j][k]+=1.4142135f; //Wrong k
								}else{
									disCounterPixel[j][k]+=1.0f;
								}
								}catch(...){
									printf("Error comes from for3\n");
									return EXIT_FAILURE;
								}
							}
							{
								if((contours[i][contours[i].size()-1].x != contours[i][0].x)||(contours[i][contours[i].size()-1].y!=contours[i][0].y)){
									disCounterPixel[j][contours[i].size()-1]+=1.4142135f;
								}else{
									disCounterPixel[j][contours[i].size()-1]+=1.0f;
								}
							}
						}else if(j > k){
							try{
							for(int m = j;m<contours[i].size();m++){
								disPixel[j][k] = disCounterPixel[k][j]; //Wrong CounterPixel
								disCounterPixel[j][k] = disPixel[k][j];
							}
							}catch(...){
								printf("Error comes from {else if}, contours[%d].size() = %d;j: %d k: %d\n ; disPixel[%d][%d] = %f; disCounterPixel[%d][%d] = %f",i,contours[i].size(),j,k,k,j,disPixel[k][j],k,j,disCounterPixel[k][j]);
								for(int x=0;x<contours[i].size();x++){
									printf("disPixel[0][%d]= %f;\n",x,disPixel[0][x]);
									printf("disCounterPixel[0][%d]=%f\n",x,disCounterPixel[0][x]);

								}
								
								
								return EXIT_FAILURE;
							}
						}else{
							try{
							disPixel[j][k] = 0;
							disCounterPixel[j][k] = 0;
							}catch(...){
								printf("Error comes from else\n");
								return EXIT_FAILURE;
							}
						}
					}
				}
				}catch(...){
					printf("distanceCalc Error\n");
					return EXIT_FAILURE;
				}

				printf( "i:%d distance calc OK\n",i);
				//3.Replace troublesome archs by lines
				for(int j = 0; j<contours[i].size();j++){
					for(int k=0; k<contours[i].size();k++){ //wrong -> add if j< k 
						//printf("disReplace: disPixel[%d][%d]= %f;disCounterPixel[%d][%d]=%f\n",j,k,disPixel[j][k],j,k,disCounterPixel[j][k]);
						if(j<k){
						if(disPixel[j][k]<disCounterPixel[j][k]){
							float disOfjk = (float)sqrt((double)((contours[i][j].x - contours[i][k].x)*(contours[i][j].x - contours[i][k].x)+(contours[i][j].y - contours[i][k].y)*(contours[i][j].y - contours[i][k].y)));
							
							if(disPixel[j][k]/disOfjk>1.5){ //Wrong CounterPixel -> disPixel
								//Re
								ALine aline;
								aline.contour=i;
								aline.BeginX=contours[i][j].x;
								aline.BeginY=contours[i][j].y;
								aline.EndX=contours[i][k].x;
								aline.EndY=contours[i][k].y;
								repairedByALine.push_back(aline);// aline is copied into vector, different from java's storage of pointer.
								//printf("[%d,%d](%d,%d),(%d,%d) is connected by a line\n",j,k,aline.BeginX,aline.BeginY,aline.EndX,aline.EndY);
							}
						}else{
							float disOfjk = (float)sqrt((double)((contours[i][j].x - contours[i][k].x)*(contours[i][j].x - contours[i][k].x)+(contours[i][j].y - contours[i][k].y)*(contours[i][j].y - contours[i][k].y)));
							if(disCounterPixel[j][k]/disOfjk>1.5){ 
								//Re
								ALine aline;
								aline.contour=i;
								aline.BeginX=contours[i][j].x;
								aline.BeginY=contours[i][j].y;
								aline.EndX=contours[i][k].x;
								aline.EndY=contours[i][k].y;
								repairedByALine.push_back(aline);// aline is copied into vector, different from java's storage of pointer.
								//printf("(%d,%d),(%d,%d) is connected by a line\n",aline.BeginX,aline.BeginY,aline.EndX,aline.EndY);
							}
						}
						}
			}
		}
				printf( "i:%d distance rep calc OK\n",i);
//				free(disPixel);
//				free(disClockWise);
//				free(disCounterPixel);
//				free(disCounterClockWise);
				
	}
	

}
}catch(...){
	std::cerr << "Exception: Caught" << std::endl;
	return EXIT_FAILURE;
	}
		printf("%d Detecting is OK\n",tempInCounter-1);
		for(int it=0;it<repairedByALine.size();it){
			int& i = repairedByALine[it].contour;
/*			int beginNum;
			int endNum;
			for(int j;j<contours[i].size();j++){
				if((contours[i][j].x==repairedByALine[it].BeginX)&&(contours[i][j].y==repairedByALine[it].BeginY)){
					beginNum = j;
				}
				if((contours[i][j].x==repairedByALine[it].EndX)&&(contours[i][j].y==repairedByALine[it].EndY)){
					endNum = j;
				}
			}

			if(beginNum<endNum){
				contours[i].erase(beginNum+1,endNum-1);

			}
*/
			int deletePixelOnContour = -1;
			for(vector<cv::Point>::iterator iter = contours[i].begin();iter != contours[i].end();iter++){
				if(((*iter).x == repairedByALine[it].BeginX)&&((*iter).y == repairedByALine[it].BeginX)){ //Wrong BeginY
					deletePixelOnContour = 1;
				}
				if(((*iter).x == repairedByALine[it].EndX)&&((*iter).y == repairedByALine[it].EndY)){
					deletePixelOnContour = 0;
				}
				if(deletePixelOnContour = 1){
					 iter = contours[i].erase(iter);
				}
				if(iter == contours[i].end()){
					break;
				}
			}
			//add contour pixels
			for(vector<cv::Point>::iterator iter = contours[i].begin();iter != contours[i].end();iter++){
				if(((*iter).x == repairedByALine[it].BeginX)&&((*iter).y == repairedByALine[it].BeginX)){
					iter = iter++;
					for(int p = 0; p<(repairedByALine[it].EndX-repairedByALine[it].BeginX);p++){
						cv::Point linePoint;
						linePoint.x = repairedByALine[it].BeginX+p; 
						linePoint.y = repairedByALine[it].BeginY+(repairedByALine[it].EndY-repairedByALine[it].BeginY)/(repairedByALine[it].EndX-repairedByALine[it].BeginX)*p;
						iter = contours[i].insert(iter,linePoint);
					}
				}
			}
		}

		  /// Draw contours
		Mat drawing = Mat::zeros( img.size(), CV_8UC3 );
		for( int i = 0; i< contours.size(); i++ )
		{
			 Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
			drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
		}

		ImageType2D::Pointer itkDrawing;
		try{
		itkDrawing=itk::OpenCVImageBridge::CVMatToITKImage<ImageType2D>(drawing);
		} catch (itk::ExceptionObject &excp){
			std::cerr << "Exception: CVtoITKImage failure !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}
		printf(" %d ITK Drawing is OK\n",inIterator.GetIndex());
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
