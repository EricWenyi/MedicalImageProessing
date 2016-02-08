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
		int c;
		int x;
		int y;
		int label;
	};
	APoint apoint;
	vector<APoint> temp1;
	vector<APoint> temp2;
	vector<vector<APoint>> points;
	
	int labelCounter = 0;
	int zeroCounter = 0;
	int tempC = -1;
	int tempL = -1;

	//邻域迭代器
	typedef itk::ConstNeighborhoodIterator< ImageType3D > NeighborhoodIteratorType;
	NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(1);//设定领域半径，所有方向半径都为1，及3*3*3
	NeighborhoodIteratorType it( radius, originImage3D, originImage3D->GetLargestPossibleRegion() );
	ImageType3D::IndexType location;//location用于跳转邻域迭代器至所需要的点的位置

	for( inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice() ){
		ImageType3D::IndexType sliceIndex = inIterator.GetIndex();
		location[2] = sliceIndex[2];//设定location的Z坐标为当前切片序号
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
		
		if(sliceIndex[2] == 0){
			for(int i = 0; i < contours.size(); i++){
				for(int j = 0; j < contours[i].size(); j++){
					apoint.c = i;
					apoint.x = contours[i][j].x;
					apoint.y = contours[i][j].y;
					apoint.label = labelCounter;
					temp1.push_back(apoint);
				}
				labelCounter++;
			}
			points.push_back(temp1);
		} else {
			for(int i = 0; i < contours.size(); i++){
				for(int j = 0; j < contours[i].size(); j++){
					apoint.c = i;
					apoint.x = contours[i][j].x;
					apoint.y = contours[i][j].y;
					apoint.label = -1;
					temp2.push_back(apoint);
				}
			}
	
			for(int i = 0; i < temp2.size(); i++){
				location[0] = temp2[i].x;
				location[1] = temp2[i].y;
				it.SetLocation(location);
				for(int j = 0; j < 9; j++){
					if(it.GetPixel(j) != 0){
						for(int k = 0; k < temp1.size(); k++){
							if(temp1[k].x == it.GetIndex(j)[0] && temp1[k].y == it.GetIndex(j)[1]){
								temp2[i].label = temp1[k].label;
								//std::cout<<temp2[i].label<<std::endl;
							}
						}
					} else {
						zeroCounter++;
					}
				}
				if(zeroCounter == 9){
					if(tempC != temp2[i].c){
						temp2[i].label = labelCounter;
						labelCounter++;
						tempL = temp2[i].label;
						tempC = temp2[i].c;
					} else {
						temp2[i].label = tempL;
					}
					//std::cout<<temp2[i].label<<std::endl;
					zeroCounter = 0;
				}
			}

			points.push_back(temp2);

			temp1.clear();
			temp1.resize(temp2.size());
			memcpy(&temp1[0], &temp2[0], temp2.size() * sizeof(APoint));
			temp2.clear();
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