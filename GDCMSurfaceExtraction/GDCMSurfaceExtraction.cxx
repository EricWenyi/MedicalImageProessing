//�ļ���д��ͷ�ļ�
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//GDCM�����ļ���д��ͷ�ļ�
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"

//��Ƭ��������ͷ�ļ�
#include "itkImageSliceConstIteratorWithIndex.h"

//��ȡ��Ƭ��ͷ�ļ�
#include "itkExtractImageFilter.h"

//��Ƭ�ļ����ϳ�3Dͼ���ͷ�ļ�
#include "itkJoinseriesImageFilter.h"

//mesh��ͷ�ļ�
#include "itkBinaryMask3DMeshSource.h"

int main( int argc, char * argv[] ){

	//�жϲ���������������������ʾ��Ҫ�Ĳ���
	if( argc < 2 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "inputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//ָ�����������ͼ������
	typedef itk::Image< signed short, 3 > ImageType3D;

	//�趨����ͼ�������
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//�趨��ȡ�����ͼ��趨��ȡ��ͼ�����ͺ�ͼ���ַ
	typedef itk::ImageFileReader< ImageType3D > ReaderType;
	ReaderType::Pointer reader = ReaderType::New();

	reader->SetImageIO( gdcmIO );
	reader->SetFileName( argv[1] );

	//���¶�ȡ�����������쳣
	try{
		reader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject reader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	typedef itk::Mesh< double > MeshType;
	typedef itk::BinaryMask3DMeshSource< ImageType3D, MeshType > MeshSourceType;
	MeshSourceType::Pointer meshSource = MeshSourceType::New();
	meshSource->SetObjectValue(255);
	meshSource->SetInput( reader->GetOutput() );

	try{
		meshSource->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject meshSource->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	std::cout << "Nodes = " << meshSource->GetNumberOfNodes() << std::endl;
	std::cout << "Cells = " << meshSource->GetNumberOfCells() << std::endl;

	return EXIT_SUCCESS;
}