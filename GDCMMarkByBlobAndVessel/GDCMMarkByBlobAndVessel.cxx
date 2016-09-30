#include "itkGDCMImageIO.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

#include "itkNeighborhoodIterator.h"

#include "mat.h"

int main( int argc, char* argv[] ){
	if( argc < 3 ){
		std::cerr << "Usage: " << argv[0] << "inputFile matFile outputFile" << std::endl;
		return EXIT_FAILURE;
    }

	typedef itk::Image< signed short, 3 > ImageType3D;
	typedef itk::Image< signed short, 2 > ImageType2D;

	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetImageIO( gdcmIO );
	reader->SetFileName( argv[1] );

	try{
		reader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: reader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::NeighborhoodIterator< ImageType3D > NeighborhoodIteratorType;
	NeighborhoodIteratorType::RadiusType radius;
	radius.Fill(0);
	NeighborhoodIteratorType it( radius, reader->GetOutput(), reader->GetOutput()->GetLargestPossibleRegion() );
	ImageType3D::IndexType location;
	
	MATFile *pmatFile = matOpen(argv[2], "r");
	mxArray *pMxArrayBlob = matGetVariable(pmatFile, "FBlob");
	mxArray *pMxArrayVessel = matGetVariable(pmatFile, "FVessel");
	
	int *dimensions = (int *)mxGetDimensions(pMxArrayBlob);
	int num = mxGetNumberOfElements(pMxArrayBlob);

	double *FBlob = (double *)mxGetData(pMxArrayBlob);
	double *FVessel = (double *)mxGetData(pMxArrayVessel);
	
	for(int i = 0; i < dimensions[0]; i++){
		for(int j = 0; j < dimensions[1]; j++){
			for(int k = 0; k < dimensions[2]; k++){
				if(FBlob[dimensions[0] * dimensions[1] * (k - 1) + dimensions[0] * j + i] > 0.5 && FVessel[dimensions[0] * dimensions[1] * (k - 1) + dimensions[0] * j + i] > 0 && FVessel[dimensions[0] * dimensions[1] * (k - 1) + dimensions[0] * j + i] < 0.7){
					location[0] = i;
					location[1] = j;
					location[2] = k;
					it.SetLocation(location);
					it.SetPixel(0, 2047);
				}
			}
		}
	}

	mxFree(FBlob);
	mxFree(FVessel);
	matClose(pmatFile);
	
	typedef itk::ImageFileWriter<ImageType3D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( argv[3] );
	writer->SetInput( reader->GetOutput() );

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: writer->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}