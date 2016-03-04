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

// Software Guide : BeginLatex
//
// This example illustrates how to manually construct an \doxygen{Image}
// class. The following is the minimal code needed to instantiate, declare
// and create the \code{Image} class.
//
// \index{itk::Image!Instantiation}
// \index{itk::Image!Header}
//
// First, the header file of the \code{Image} class must be included.
//
// Software Guide : EndLatex

// Software Guide : BeginCodeSnippet
#include "itkImage.h"
#include "itkImageFileWriter.h"

typedef itk::Image< unsigned short, 3 > ImageType;

int SetPixelInterface(ImageType::Pointer& image,int x,int y,int z, int ppixelValue){
	ImageType::IndexType pixelIndex={{x,y,z}};
	ImageType::PixelType pixelValue = image->GetPixel(pixelIndex);
	image->SetPixel(pixelIndex,ppixelValue);
	return 0;
}


int main(int argc, char* argv[]){

  
 
  ImageType::Pointer image = ImageType::New();
  
  ImageType::IndexType start;

  start[0] =   0;  // first index on X
  start[1] =   0;  // first index on Y
  start[2] =   0;  // first index on Z
  
  ImageType::SizeType  size;

  size[0]  = 200;  // size along X
  size[1]  = 200;  // size along Y
  size[2]  = 3;  // size along Z
 
  ImageType::RegionType region;

  region.SetSize( size );
  region.SetIndex( start );
  
  image->SetRegions( region );
  image->Allocate(true);
  //Drawing
  //Slice1
  for(int i=10;i<20;i++)
	  SetPixelInterface(image,10,i,0,1);
  
  for(int i=10;i<20;i++)
	  SetPixelInterface(image,i,10,0,1);

  for(int i=10;i<20;i++)
	  SetPixelInterface(image,i,20,0,1);

  for(int i=10;i<20;i++)
	  SetPixelInterface(image,20,i,0,1);


  //Slice2
  for(int i=21;i<30;i++)
	  SetPixelInterface(image,10,i,1,1);
  
  for(int i=10;i<20;i++)
	  SetPixelInterface(image,i,21,1,1);

  for(int i=10;i<20;i++)
	  SetPixelInterface(image,i,30,1,1);

  for(int i=21;i<30;i++)
	  SetPixelInterface(image,20,i,1,1);

  //Slice3
  for(int i=10;i<20;i++)
	   SetPixelInterface(image,i,20,2,3);


  	typedef unsigned short PixelType3D;
	typedef itk::Image< PixelType3D, 3 > ImageType3D;

  	typedef itk::ImageFileWriter<ImageType3D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( argv[1] );
	writer->SetInput(image);

	try{
		writer->Update();
	} catch (itk::ExceptionObject &excp){
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << excp << std::endl;
		return EXIT_FAILURE;
	}

  return EXIT_SUCCESS;
}
