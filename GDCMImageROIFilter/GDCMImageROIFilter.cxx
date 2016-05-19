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

//��Ȥ����ȡ��ͷ�ļ�
#include "itkRegionOfInterestImageFilter.h"

int main( int argc, char * argv[] ){

	//�жϲ���������������������ʾ��Ҫ�Ĳ���
	if( argc < 3 ){
		std::cerr << "Usage: " << std::endl;
		std::cerr << argv[0] << "inputImageFile outputImageFile" << std::endl;
		return EXIT_FAILURE;
	}

	//ָ������������ص�����
	typedef signed short PixelType;

	//ָ�����������ͼ������
	typedef itk::Image< PixelType, 3 > ImageType3D;
	typedef itk::Image< PixelType, 2 > ImageType2D;

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

	//�趨���������С
	ImageType3D::ConstPointer image3D = reader->GetOutput();
	ImageType3D::RegionType region3D = image3D->GetLargestPossibleRegion();
	
	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType iterator( image3D, region3D );
	iterator.SetFirstDirection( 0 );
	iterator.SetSecondDirection( 1 );

	//����ͼƬ�����
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( image3D->GetOrigin()[2] );

	for( iterator.GoToBegin(); !iterator.IsAtEnd(); iterator.NextSlice() ){
		//��ȡ��Ƭ���
		ImageType3D::IndexType sliceIndex = iterator.GetIndex();

		if ( sliceIndex[2] >= atoi(argv[3]) - 1 && sliceIndex[2] < atoi(argv[4]) ){
			//��ȡÿ����Ƭ�Ĵ�С��������ÿ����Ƭ��Z��Ϊ0
			typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
			ExtractFilterType::InputImageRegionType sliceRegion = iterator.GetRegion();
			ExtractFilterType::InputImageRegionType::SizeType sliceSize = sliceRegion.GetSize();
			sliceSize[2] = 0;

			//�趨ÿ����Ƭ�Ĵ�С�����
			sliceRegion.SetSize( sliceSize );
			sliceRegion.SetIndex( sliceIndex );
		
			//ָ������������
			ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
			extractFilter->SetExtractionRegion( sliceRegion );
		
			//��λ���������������޸ķ���̮������Ϊ�Ӿ���
			extractFilter->InPlaceOn();
			extractFilter->SetDirectionCollapseToSubmatrix();
			extractFilter->SetInput( reader->GetOutput() );

			//���¹��������������쳣
			try{
				extractFilter->Update();
			} catch (itk::ExceptionObject &excp){
				std::cerr << "ExceptionObject: extractFilter->Update() caught !" << std::endl;
				std::cerr << excp << std::endl;
				return EXIT_FAILURE;
			}
			
			//�趨������ʼ��
			ImageType2D::IndexType index;
			index[0] = 0;
			index[1] = 0;

			//�趨�����С
			ImageType2D::SizeType size;
			size[0] = 512;
			size[1] = 512;

			//�趨ROI�������
			ImageType2D::RegionType region;
			region.SetIndex( index );
			region.SetSize( size );

			//�趨ROI������
			typedef itk::RegionOfInterestImageFilter< ImageType2D, ImageType2D > ROIImageFilterType;
			ROIImageFilterType::Pointer ROIImageFilter = ROIImageFilterType::New();
			ROIImageFilter->SetRegionOfInterest( region );
			ROIImageFilter->SetInput( extractFilter->GetOutput());
			
			//����ROI���������������쳣
			try{
				ROIImageFilter->Update();
			} catch ( itk::ExceptionObject & err ){
				std::cerr << "ExceptionObject ROIImageFilter->Update() caught !" << std::endl;
				std::cerr << err << std::endl;
				return EXIT_FAILURE;
			}

			//�趨ͼƬ�����������
			joinSeries->PushBackInput( ROIImageFilter->GetOutput() );
		}
	}

	//����ͼƬ��������������쳣
	try{
		joinSeries->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject joinSeries->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	//�趨д��������
	typedef itk::ImageFileWriter< ImageType3D > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput( joinSeries->GetOutput() );
	writer->SetFileName( argv[2] );	
	
	//����д�������������쳣
	try{
		writer->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject writer->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
