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

//��ֵ�˲�����ͷ�ļ�
#include "itkMedianImageFilter.h"

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
	ReaderType::Pointer originReader = ReaderType::New();
	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );

	//���¶�ȡ�����������쳣
	try{
		originReader->Update();
	} catch ( itk::ExceptionObject & err ){
		std::cerr << "ExceptionObject originReader->Update() caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	//�趨���������С
	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::RegionType originRegion3D = originImage3D->GetLargestPossibleRegion();
	
	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;
	SliceIteratorType originIterator( originImage3D, originRegion3D );
	originIterator.SetFirstDirection(0);
	originIterator.SetSecondDirection(1);

	//����ͼƬ�����
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;
	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin(originImage3D->GetOrigin()[2]);
	joinSeries->SetSpacing(originImage3D->GetSpacing()[2]);
	
	for( originIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice() ){
		//��ȡ��Ƭ���
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();

		//��ȡÿ����Ƭ�Ĵ�С��������ÿ����Ƭ��Z��Ϊ0
		typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originSliceRegion.GetSize();
		originSliceSize[2] = 0;

		//�趨ÿ����Ƭ�Ĵ�С�����
		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );
		
		//ָ������������
		ExtractFilterType::Pointer originExtractFilter = ExtractFilterType::New();
		originExtractFilter->SetExtractionRegion( originSliceRegion );
		
		//��λ���������������޸ķ���̮������Ϊ�Ӿ���
		originExtractFilter->InPlaceOn();
		originExtractFilter->SetDirectionCollapseToSubmatrix();
		originExtractFilter->SetInput( originReader->GetOutput() );
		
		//���¹��������������쳣
		try{
			originExtractFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		//�趨ԭͼ����ֵ�˲������������ͼ������
		typedef itk::MedianImageFilter< ImageType2D, ImageType2D >  MedianFilterType;
		MedianFilterType::Pointer medianFilter = MedianFilterType::New();

		//�趨ˮƽ�뾶����ֱ�뾶
		ImageType2D::SizeType indexRadius;
		indexRadius.Fill(2);
		medianFilter->SetRadius( indexRadius );

		//�趨��ֵ�˲�������
		medianFilter->SetInput( originExtractFilter->GetOutput() );
		
		//������ֵ�˲������������쳣
		try{
			medianFilter->Update();
		} catch ( itk::ExceptionObject & err ){
			std::cerr << "ExceptionObject medianFilter->Update() caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}
		
		joinSeries->PushBackInput( medianFilter->GetOutput() );
	}
	
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