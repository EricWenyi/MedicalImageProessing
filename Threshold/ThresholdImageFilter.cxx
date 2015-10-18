/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

/*=========================================================================
 *
 *  Copyright Medical Image Processing Group@SunDataGroup, RTlab, Uestc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/*=========================================================================
 *
 *  Copyright l3mmings
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  
 *  
 *  http://l3mmings.blogspot.com/2010/08/slices-stacks-and-itk.html
 *
 *=========================================================================*/
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"

#include "itkConnectedThresholdImageFilter.h"
#include "itkImage.h"
#include "itkCurvatureFlowImageFilter.h"

#include <vector>
#include "itksys/SystemTools.hxx"
#include "itkCastImageFilter.h"

#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkExtractImageFilter.h"
#include "itkJoinseriesImageFilter.h"

int main( int argc, char* argv[] )
{
  if( argc < 7 )
    {
    std::cerr << "Usage: " << argv[0] <<
      " DicomDirectory  OutputDicomPath&Name seedX seedY lowerThreshold upperThreshold" << std::endl;
    return EXIT_FAILURE;
    }
  ///Part one: Reading
  ///Pixel type used for series inputs.
  typedef  signed short   PixelType;
  const unsigned int      Dimension = 3;

  typedef itk::Image< PixelType, Dimension >      ImageType;
  typedef itk::ImageSeriesReader< ImageType >     ReaderType;

  ///Pixel type used for single slice
  const unsigned int      OutputDimension = 2;
  typedef signed  short        OutputPixelType;
  typedef itk::Image< OutputPixelType, OutputDimension >      OutputImageType;


  ///def image format:Dicom format
  typedef itk::GDCMImageIO                        ImageIOType;
  typedef itk::GDCMSeriesFileNames                NamesGeneratorType;
  ImageIOType::Pointer gdcmIO = ImageIOType::New();

  ///Prepare the file names to be read in
  NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();
  namesGenerator->SetInputDirectory( argv[1] );

  const ReaderType::FileNamesContainer & filenames =
                            namesGenerator->GetInputFileNames();
  const unsigned int numberOfFileNames =  filenames.size();
  std::cout << numberOfFileNames << std::endl;
  for(unsigned int fni = 0; fni < numberOfFileNames; ++fni)
    {
    std::cout << "filename # " << fni << " = ";
    std::cout << filenames[fni] << std::endl;
    }

  ReaderType::Pointer reader = ReaderType::New();


  reader->SetImageIO( gdcmIO );
  reader->SetFileNames( filenames );
  ///Reading...
  try
    {
    reader->Update();

    }
  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while writing the image" << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }

  /// Part two: Extract each slice from the whole 3D volume, and operate on it.
  ImageType::ConstPointer InputImage3D=reader->GetOutput();
//  ImageType::RegionType tempRegion = InputImage3D->GetLargestPossibleRegion();
//  tempRegion.Print(std::cout);
  typedef itk::ImageSliceConstIteratorWithIndex<ImageType> SliceIteratorType;
  
  SliceIteratorType inIterator(InputImage3D,InputImage3D->GetLargestPossibleRegion());
  inIterator.SetFirstDirection(0);
  inIterator.SetSecondDirection(1);

 


  typedef itk::ExtractImageFilter<ImageType,OutputImageType> ExtractFilterType;
  typedef itk::JoinSeriesImageFilter< OutputImageType, ImageType > JoinSeriesFilterType;

   JoinSeriesFilterType::Pointer joinSeries = JoinSeriesFilterType::New();
   joinSeries->SetOrigin(InputImage3D->GetOrigin()[2]);
   joinSeries->SetSpacing(InputImage3D->GetSpacing()[2]);

 for(inIterator.GoToBegin(); !inIterator.IsAtEnd(); inIterator.NextSlice())
 {
   
    ImageType::IndexType sliceIndex = inIterator.GetIndex();
    ExtractFilterType::InputImageRegionType::SizeType sliceSize = inIterator.GetRegion().GetSize();
        sliceSize[2] = 0;
    ExtractFilterType::InputImageRegionType sliceRegion = inIterator.GetRegion();
        sliceRegion.SetSize( sliceSize );
        sliceRegion.SetIndex( sliceIndex );

    ///Pull out slice
    ExtractFilterType::Pointer inExtractor = ExtractFilterType::New(); ///Must be within loop so that smart pointer is unique
        inExtractor->SetInput( InputImage3D );
        inExtractor->SetExtractionRegion( sliceRegion );
        inExtractor->Update();

    ///Operate on Slice

		///Smoothing filter
      typedef itk::CurvatureFlowImageFilter< OutputImageType, OutputImageType > CurvatureFlowImageFilterType;
  
      CurvatureFlowImageFilterType::Pointer smoothing = CurvatureFlowImageFilterType::New();
	  smoothing->SetInput(inExtractor->GetOutput());

      typedef itk::ConnectedThresholdImageFilter< OutputImageType,
                                   OutputImageType > ConnectedFilterType;

      ConnectedFilterType::Pointer connectedThreshold = ConnectedFilterType::New();

      smoothing->SetNumberOfIterations( 5 );
      smoothing->SetTimeStep( 0.125 );
  
	  ///ConnectedThreshold Filter
      const PixelType lowerThreshold = atof( argv[5] );
      const PixelType upperThreshold = atof( argv[6] );


      connectedThreshold->SetLower(  lowerThreshold  );
      connectedThreshold->SetUpper(  upperThreshold  );
  
      connectedThreshold->SetReplaceValue( 255 );
	
      OutputImageType::IndexType  index;

      index[0] = atoi( argv[3] );
      index[1] = atoi( argv[4] );

      connectedThreshold->SetSeed( index );
    ///Save Slice
      connectedThreshold->SetInput(smoothing->GetOutput());
      connectedThreshold->Update();
      joinSeries->PushBackInput( connectedThreshold->GetOutput() );
 }

 try
    {
    joinSeries->Update();
    }
  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while join the image" << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }
  printf("Image Join is OK!\n");

//  ImageType::ConstPointer tempImage3D=joinSeries->GetOutput();
//  ImageType::RegionType tempRegion = tempImage3D->GetLargestPossibleRegion();
//  tempRegion.Print(std::cout);
  
  ///Part three: Write out
  const char * outputDirectory = argv[2];

  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputDirectory );
  writer->SetInput( joinSeries->GetOutput() );

try
{
    writer->Update();
}
catch( itk::ExceptionObject & err )
{
    std::cerr << "Write Output Exception caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
}
 
  return EXIT_SUCCESS;
}
