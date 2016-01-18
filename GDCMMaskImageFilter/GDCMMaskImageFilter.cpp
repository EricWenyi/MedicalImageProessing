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

//��ģͼ���������ͷ�ļ�
#include "itkMaskImageFilter.h"

int main( int argc, char* argv[] ){
	if( argc < 4 ){
		std::cerr << "Usage: " << argv[0] << " InputImageFile OutputImageFile" << std::endl;
		return EXIT_FAILURE;
    }

	//�趨ͼ������
	typedef itk::Image< signed short, 3 > ImageType3D;
	typedef itk::Image< signed short, 2 > ImageType2D;

	//�趨����ͼ�������
	typedef itk::GDCMImageIO ImageIOType;
	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	//�趨��ȡ�����ͼ��趨��ȡ��ͼ�����ͺ�ͼ���ַ
	typedef itk::ImageFileReader< ImageType3D > ReaderType;

	ReaderType::Pointer originReader = ReaderType::New();
	originReader->SetImageIO( gdcmIO );
	originReader->SetFileName( argv[1] );

	ReaderType::Pointer maskReader = ReaderType::New();
	maskReader->SetImageIO( gdcmIO );
	maskReader->SetFileName( argv[2] );

	try{
		originReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: originReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	try{
		maskReader->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject: maskReader->Update() caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

	ImageType3D::ConstPointer originImage3D = originReader->GetOutput();
	ImageType3D::ConstPointer maskImage3D = maskReader->GetOutput();

	//�趨��Ƭ������
	typedef itk::ImageSliceConstIteratorWithIndex < ImageType3D > SliceIteratorType;

	SliceIteratorType originIterator( originImage3D, originImage3D->GetLargestPossibleRegion() );
	originIterator.SetFirstDirection(0);
	originIterator.SetSecondDirection(1);

	SliceIteratorType maskIterator( maskImage3D, maskImage3D->GetLargestPossibleRegion() );
	maskIterator.SetFirstDirection(0);
	maskIterator.SetSecondDirection(1);

	typedef itk::ExtractImageFilter< ImageType3D, ImageType2D > ExtractFilterType;
	typedef itk::JoinSeriesImageFilter< ImageType2D, ImageType3D > JoinSeriesFilterType;

	JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
	joinSeries->SetOrigin( originImage3D->GetOrigin()[2] );
	joinSeries->SetSpacing( originImage3D->GetSpacing()[2] );

	for( originIterator.GoToBegin(); !originIterator.IsAtEnd(); originIterator.NextSlice() ){
		ImageType3D::IndexType originSliceIndex = originIterator.GetIndex();
		ExtractFilterType::InputImageRegionType::SizeType originSliceSize = originIterator.GetRegion().GetSize();
		originSliceSize[2] = 0;
		ExtractFilterType::InputImageRegionType originSliceRegion = originIterator.GetRegion();
		originSliceRegion.SetSize( originSliceSize );
		originSliceRegion.SetIndex( originSliceIndex );

		ImageType3D::IndexType maskSliceIndex = maskIterator.GetIndex();
		ExtractFilterType::InputImageRegionType::SizeType maskSliceSize = maskIterator.GetRegion().GetSize();
		maskSliceSize[2] = 0;
		ExtractFilterType::InputImageRegionType maskSliceRegion = maskIterator.GetRegion();
		maskSliceRegion.SetSize( maskSliceSize );
		maskSliceRegion.SetIndex( maskSliceIndex );
		
		ExtractFilterType::Pointer originExtractor = ExtractFilterType::New();
		originExtractor->SetExtractionRegion( originSliceRegion );
		originExtractor->InPlaceOn();
		originExtractor->SetDirectionCollapseToSubmatrix();
		originExtractor->SetInput( originReader->GetOutput() );

		ExtractFilterType::Pointer maskExtractor = ExtractFilterType::New();
		maskExtractor->SetExtractionRegion( maskSliceRegion );
		maskExtractor->InPlaceOn();
		maskExtractor->SetDirectionCollapseToSubmatrix();
		maskExtractor->SetInput( maskReader->GetOutput() );

		try{
			originExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: originExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		try{
			maskExtractor->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: maskExtractor->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		typedef itk::MaskImageFilter< ImageType2D, ImageType2D > MaskFilterType;
		MaskFilterType::Pointer maskImageFilter = MaskFilterType::New();
		
		maskImageFilter->SetInput( originExtractor->GetOutput() );
		maskImageFilter->SetMaskImage( maskExtractor->GetOutput() );

		try{
			maskImageFilter->Update();
		} catch (itk::ExceptionObject &excp){
			std::cerr << "ExceptionObject: maskImageFilter->Update() caught !" << std::endl;
			std::cerr << excp << std::endl;
			return EXIT_FAILURE;
		}

		joinSeries->PushBackInput( maskImageFilter->GetOutput() );
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