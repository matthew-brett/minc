#include <iostream>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageIOFactory.h>
#include <itkImageIOBase.h>
#include <itkFlipImageFilter.h>
#include <itkMetaDataObject.h>
#include <itkMetaDataDictionary.h>

#include "itkMincImageIOFactory.h"
#include "itkMincImageIO.h"

#include <time_stamp.h>    // for creating minc style history entry
#include <getopt.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>

#include "minc_helpers.h"

typedef std::vector<double> double_vector;


void show_usage (const char *name)
{
  std::cerr  
      << "Usage: "<<name<<" <input> <output> " << std::endl
      << "--clobber clobber output files"<<std::endl
      << "--verbose be verbose"<<std::endl
      << "--inv-x invert X axis"<<std::endl
      << "--inv-y invert Y axis"<<std::endl
      << "--inv-z invert Z axis"<<std::endl
      << "--center set origin to the center of the image"<<std::endl
      << "--show-meta show meta information (if present)"<<std::endl
      << "--dwi - assume that we are dealing with DWI scan"<<std::endl
      << "--use-b-matrix - convert b-matrix (if present) into DWI gradient directions and b-value, implies DWI"<<std::endl
      << "--char   - save as signed char"<<std::endl
      << "--byte   - save as unsigned char"<<std::endl
      << "--short  - save as signed short"<<std::endl
      << "--ushort - save as unsigned short"<<std::endl
      << "--int    - save as signed int"<<std::endl
      << "--uint   - save as unsigned int"<<std::endl
      << "--float  - save as float"<<std::endl
      << "--double - save as double"<<std::endl
;
}

typedef itk::ImageIOBase IOBase;
typedef itk::SmartPointer<IOBase> IOBasePointer;

class ImageConverterBase
{
protected:
  bool assume_dti;
  bool use_b_matrix;
  std::string history;
  bool verbose;
  bool inv_x;
  bool inv_y;
  bool inv_z;
  bool center;
  bool show_meta;
  
public:
  void setup(bool _verbose=false,
              bool _assume_dti=false,
              bool _use_b_matrix=false,
              bool _inv_x=false,bool _inv_y=false,bool _inv_z=false, 
              bool _center=false, 
              bool _show_meta=false,
              const std::string _history="")
  {
    assume_dti=_assume_dti;
    use_b_matrix=_use_b_matrix;
    history=_history;
    verbose=_verbose;
    inv_x=_inv_x;
    inv_y=_inv_y;
    inv_z=_inv_z;
    center=_center;
    show_meta=_show_meta;
  }
  
  virtual void load_and_save_image(IOBase* base,const char *fname)=0;

  virtual ~ImageConverterBase()
  {}
};

template<class TInputImage,class TOutputImage=TInputImage> class ImageConverter: public ImageConverterBase
{
protected:
public:
  
  
  ~ImageConverter()
  {}
  
  ImageConverter()
  {
    setup();
  }
    
 
  void convert_meta_minc_to_nrrd(itk::MetaDataDictionary& dict)
  {
    // let's try converting DTI-related meta-information
    double_vector bvalues,direction_x,direction_y,direction_z;
    
    if( itk::ExposeMetaData<double_vector>( dict , "acquisition:bvalues",bvalues) &&
        itk::ExposeMetaData<double_vector>( dict , "acquisition:direction_x",direction_x) &&
        itk::ExposeMetaData<double_vector>( dict , "acquisition:direction_y",direction_y) &&
        itk::ExposeMetaData<double_vector>( dict , "acquisition:direction_z",direction_z))
    {
      //We have got MINC-style DTI metadata
      if(bvalues.size()!=direction_x.size() || 
        bvalues.size()!=direction_y.size() ||
        bvalues.size()!=direction_z.size() )
      {
        std::cerr<<"WARNING: Different number of components of DTI directions"<<std::endl
                <<" Skipping DTI metadata conversion!"<<std::endl;
      } else {
        double max_b_value=*std::max_element(bvalues.begin(),bvalues.end());
        if(verbose)
        {
          std::cout<<"Found maximum B-value:"<<max_b_value<<std::endl;
        }
        
        itk::EncapsulateMetaData<std::string>( dict,
          "modality",
          "DWMRI");      
        
        //TODO: find out if we can get this info from minc 
        // using identity for now
        std::vector< std::vector< double > > measurement_frame(3,std::vector< double >(3));
        measurement_frame[0][0]=1.0;
        measurement_frame[1][1]=1.0;
        measurement_frame[2][2]=1.0;
        
        
        itk::EncapsulateMetaData< std::vector< std::vector< double > > >( dict,
          "NRRD_measurement frame",
          measurement_frame);      

        
        //TODO: deal with NRRD_thicknesses ?
        
        std::ostringstream ossKey;
        ossKey<<std::setw(9) << std::setiosflags(std::ios::fixed)
          << std::setprecision(6) << std::setiosflags(std::ios::right) << max_b_value;
        
          
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_centerings[0]","cell");
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_centerings[1]","cell");
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_centerings[2]","cell");
        
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_kinds[0]","space");
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_kinds[1]","space");
        itk::EncapsulateMetaData<std::string>( dict,
          "NRRD_kinds[2]","space");

        
        itk::EncapsulateMetaData<std::string>( dict,
          "DWMRI_b-value",
          ossKey.str());
        
        
        size_t zero_b_value_cnt=0;
        for(size_t i=0;i<bvalues.size();i++)
        {
          double direction[3]={direction_x[i],direction_y[i],direction_z[i]};
          double bval=bvalues[i];
          double b_scale=1.0;
          
          if(fabs(bval)<1e-3) 
            zero_b_value_cnt++;
          else
          {
            b_scale=::sqrt(bval/max_b_value);
            
            direction[0]*=b_scale;
            direction[1]*=b_scale;
            direction[2]*=b_scale;
          }
          
          std::ostringstream ossKey;
          ossKey << "DWMRI_gradient_" << std::setw(4) << std::setfill('0') << i;

          std::ostringstream ossMetaString;
          ossMetaString << std::setw(9) << std::setiosflags(std::ios::fixed)
            << std::setprecision(6) << std::setiosflags(std::ios::right)
            << direction[0]
          << "    "
            << std::setw(9) << std::setiosflags(std::ios::fixed)
            << std::setprecision(6) << std::setiosflags(std::ios::right)
            << direction[1]
          << "    "
            << std::setw(9) << std::setiosflags(std::ios::fixed)
            << std::setprecision(6) << std::setiosflags(std::ios::right)
            << direction[2];

          // std::cout<<ossKey.str()<<ossMetaString.str()<<std::endl;
          itk::EncapsulateMetaData<std::string>( dict,
            ossKey.str(), ossMetaString.str() );
        }
        if(verbose)
          std::cout<<"Found "<<zero_b_value_cnt<<" Zero b-value components"<<std::endl;
      }
    }
  }

  void convert_meta_nrrd_to_minc(itk::MetaDataDictionary& dict)
  {
    //1. get b-value
    
    std::string b_value_,gradient_value_;
    double bval=0.0,vect3d[3],vmag=0.0;
    double_vector bvalues,direction_x,direction_y,direction_z;
    
    if(itk::ExposeMetaData<std::string>( dict,"DWMRI_b-value", b_value_))
    {
      int i=0;
      while(true)
      {
          bval=atof(b_value_.c_str());
          
          std::ostringstream ossKey;
          ossKey << "DWMRI_gradient_" << std::setw(4) << std::setfill('0') << i++;
          if(itk::ExposeMetaData<std::string>( dict,ossKey.str(), gradient_value_))
          {
            std::istringstream iss(gradient_value_);
            iss >> vect3d[0] >> vect3d[1] >> vect3d[2];
            
            direction_x.push_back(vect3d[0]);
            direction_y.push_back(vect3d[1]);
            direction_z.push_back(vect3d[2]);
            
            //? normalize to unit vector and modulate bvalue instead?
            vmag=::sqrt(vect3d[0]*vect3d[0]+vect3d[1]*vect3d[1]+vect3d[2]*vect3d[2]);
            if(vmag<0.1) //Zero
              bvalues.push_back(0.0); 
            else
              bvalues.push_back(bval); 
          } else {
            break;
          }
      }
      if(!bvalues.empty())
      {
        itk::EncapsulateMetaData<double_vector>( dict , "acquisition:bvalues",bvalues);
        itk::EncapsulateMetaData<double_vector>( dict , "acquisition:direction_x",direction_x);
        itk::EncapsulateMetaData<double_vector>( dict , "acquisition:direction_y",direction_y);
        itk::EncapsulateMetaData<double_vector>( dict , "acquisition:direction_z",direction_z);
        itk::EncapsulateMetaData<double>( dict ,        "acquisition:b_value", bval);
      }
    }
  }



  double decompose_b_matrix(const double_vector& b_matrix, 
                          double_vector & bvalues,
                          double_vector & direction_x, 
                          double_vector & direction_y,
                          double_vector & direction_z)
  {
    size_t elements=b_matrix.size();
    if(elements%6) //should be divisible by 6 
      std::cerr<<"Number of elements in b_matrix is not divisible by 6 , probably an error!"<<std::endl;
    elements/=6;
    
    bvalues.resize(elements);
    direction_x.resize(elements);
    direction_y.resize(elements);
    direction_z.resize(elements);

    vnl_matrix_fixed<double, 3, 3> bMatrix;
    double max_bval=0;
    
    for(size_t i=0;i<elements;i++)
    {
      //Code from DicomToNrrdConverter
      // UNC comments: The principal eigenvector of the bmatrix is to be extracted as
      // it's the gradient direction and trace of the matrix is the b-value

      // UNC comments: Fill out the 3x3 bmatrix with the 6 components read from the 
      // DICOM header.
      bMatrix[0][0] = b_matrix[i*6+0];
      bMatrix[0][1] = b_matrix[i*6+1];
      bMatrix[0][2] = b_matrix[i*6+2];
      bMatrix[1][1] = b_matrix[i*6+3];
      bMatrix[1][2] = b_matrix[i*6+4];
      bMatrix[2][2] = b_matrix[i*6+5];
      bMatrix[1][0] = bMatrix[0][1];
      bMatrix[2][0] = bMatrix[0][2];
      bMatrix[2][1] = bMatrix[1][2];

      double bvalue = bMatrix[0][0] + bMatrix[1][1] + bMatrix[2][2];
      
      if(bvalue>max_bval) max_bval=bvalue;
      
      // UNC comments: The b-value si the trace of the bmatrix
      // UNC comments: Even if the bmatrix is null, the svd decomposition set the 1st eigenvector
      // to (1,0,0). So we force the gradient direction to 0 if the bvalue is null
      if(bvalue == 0.0 ) //TODO: threshold?
      {
        direction_x[i] = 0.0;
        direction_y[i] = 0.0;
        direction_z[i] = 0.0;
        bvalues[i] = 0.0;
      } else {
        // UNC comments: Computing the decomposition
        vnl_svd<double> svd(bMatrix);

        // UNC comments: Extracting the principal eigenvector i.e. the gradient direction
        
        direction_x[i] = svd.U(0,0);
        direction_y[i] = svd.U(1,0);
        direction_z[i] = svd.U(2,0);
        bvalues[i] = bvalue;
      }
    }
    return max_bval;
  }
  
  virtual void load_and_save_image(IOBase* base,const char *fname)
  {
    typename itk::ImageFileReader<TInputImage >::Pointer reader = itk::ImageFileReader<TInputImage >::New();
    typename itk::FlipImageFilter<TInputImage >::Pointer flip=itk::FlipImageFilter<TInputImage >::New();
    
    reader->SetImageIO(base);
    reader->SetFileName(base->GetFileName());
    reader->Update();
    
    typename TInputImage::Pointer img=reader->GetOutput();
    itk::MetaDataDictionary &thisDic=img->GetMetaDataDictionary();

    if(show_meta)
    {
      
      //let's write some meta information if there is any 
      for(itk::MetaDataDictionary::ConstIterator it=thisDic.Begin();it!=thisDic.End();++it)
      {
        itk::MetaDataObjectBase *bs=(*it).second;
        itk::MetaDataObject<std::string> * str=dynamic_cast<itk::MetaDataObject<std::string> *>(bs);
        if(str)
          std::cout<<(*it).first.c_str()<<" = "<< str->GetMetaDataObjectValue().c_str()<<std::endl;
        else
          std::cout<<(*it).first.c_str()<<" type: "<< typeid(*bs).name()<<std::endl;
      }
    }
    
    if( thisDic.HasKey( "acquisition:b_matrix" ) && use_b_matrix )
    { 
      double_vector bmatrix;
      
      if( itk::ExposeMetaData<double_vector>( thisDic , "acquisition:b_matrix",bmatrix))
      {
        double_vector bvalues,direction_x,direction_y,direction_z;
        double bval=decompose_b_matrix(bmatrix,bvalues,direction_x,direction_y,direction_z);
        
        if(!bvalues.empty())
        {
          itk::EncapsulateMetaData<double_vector>( thisDic , "acquisition:bvalues"    ,bvalues);
          itk::EncapsulateMetaData<double_vector>( thisDic , "acquisition:direction_x",direction_x);
          itk::EncapsulateMetaData<double_vector>( thisDic , "acquisition:direction_y",direction_y);
          itk::EncapsulateMetaData<double_vector>( thisDic , "acquisition:direction_z",direction_z);
          itk::EncapsulateMetaData<double>(        thisDic , "acquisition:b_value"    ,bval);
        }
      }
    }
    
    //making sure that all vcrtors contain the same number of parameters (just in case)
    if( thisDic.HasKey( "acquisition:bvalues") &&
        thisDic.HasKey( "acquisition:direction_x") &&
        thisDic.HasKey( "acquisition:direction_y") &&
        thisDic.HasKey( "acquisition:direction_z"))
    {
      convert_meta_minc_to_nrrd(thisDic);
    } else if ( thisDic.HasKey("DWMRI_b-value")  ) {
      //We have got NRRD-style DTI metadata
      convert_meta_nrrd_to_minc(thisDic);
    }
    
    

    if(verbose)
      std::cout<<"Writing "<<fname<<"..."<<std::endl;
    
    if(inv_x||inv_y||inv_z)
    {
      typename itk::FlipImageFilter<TInputImage >::FlipAxesArrayType arr;
      arr[0]=inv_x;
      arr[1]=inv_y;
      arr[2]=inv_z;
      flip->SetFlipAxes(arr);
      flip->SetInput(img);
      flip->Update();
      img=flip->GetOutput();
    }
    
    if(center)//move origin to the center of the image
    {
      typename TInputImage::RegionType r=img->GetLargestPossibleRegion();
      //std::vector<double> corner[3];
      
      typename TInputImage::IndexType idx;
      typename TInputImage::PointType c;
      
      idx[0]=r.GetIndex()[0]+r.GetSize()[0]/2.0;
      idx[1]=r.GetIndex()[1]+r.GetSize()[1]/2.0;
      idx[2]=r.GetIndex()[2]+r.GetSize()[2]/2.0;
      
      img->TransformIndexToPhysicalPoint(idx,c);
      
      typename TInputImage::PointType org=img->GetOrigin();
      
      org[0]-=c[0];
      org[1]-=c[1];
      org[2]-=c[2];
      
      img->SetOrigin(org);
    }  
    if(!history.empty())
      minc::append_history(img,history);

    typename itk::ImageFileWriter< TInputImage >::Pointer writer = itk::ImageFileWriter<TInputImage >::New();
    writer->SetFileName(fname);
    writer->SetInput( img );
    writer->Update();
    
  }

};




int main(int argc,char **argv)
{
  int verbose=0;
  int clobber=0;
  int show_meta=0;
  int inv_x=0,inv_y=0,inv_z=0,center=0;
  int assume_dti=0;
  char *_history = time_stamp(argc, argv);
  std::string history=_history;
  int use_b_matrix=0;
  
  int store_char=0,store_uchar=0,store_short=0,store_ushort=0,store_float=0,store_int=0,store_uint=0,store_double=0;
  
  free(_history);

  static struct option long_options[] = { 
    {"verbose", no_argument, &verbose, 1},
    {"quiet", no_argument, &verbose, 0},
    {"clobber", no_argument, &clobber, 1},
    {"inv-x", no_argument,  &inv_x, 1},
    {"inv-y", no_argument,  &inv_y, 1},
    {"inv-z", no_argument,  &inv_z, 1},
    {"center", no_argument, &center, 1},
    {"show-meta", no_argument, &show_meta, 1},
    {"dti", no_argument, &assume_dti, 1},
    {"dwi", no_argument, &assume_dti, 1},
    {"use-b-matrix",no_argument, &use_b_matrix, 1},
    
    {"float", no_argument, &store_float, 1},
    {"double", no_argument, &store_double, 1},
    {"byte", no_argument, &store_uchar, 1},
    {"char", no_argument, &store_char, 1},
    {"short", no_argument, &store_short, 1},
    {"ushort", no_argument, &store_ushort, 1},
    {"int", no_argument, &store_int, 1},
    {"uint", no_argument, &store_uint, 1},
    
    {0, 0, 0, 0}
  };
    
  int c;
  for (;;)
  {
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
    {
      case 0:
        break;
      case '?':
        /* getopt_long already printed an error message. */
      default:
        show_usage (argv[0]);
        return 1;
    }
  }
  
  if((argc - optind)<2)
  {
    show_usage(argv[0]);
    return 1;
  }
  std::string input=argv[optind];
  std::string output=argv[optind+1];
  
  if (!clobber && !access (output.c_str(), F_OK))
  {
    std::cerr << output.c_str () << " Exists!" << std::endl;
    return 1;
  }
 
  try
  {
    itk::RegisterMincIO();
    
    if(use_b_matrix) assume_dti=1;
    
    /* READING */
    if( verbose ) 
      std::cout<<"Reading "<<input.c_str()<<"..."<<std::endl;
    
    //try to figure out what we have got
    IOBasePointer io = itk::ImageIOFactory::CreateImageIO(input.c_str(), itk::ImageIOFactory::ReadMode );
    
    if(!io)
      throw itk::ExceptionObject("Unsupported image file type");
    
    io->SetFileName(input.c_str());
    io->ReadImageInformation();

    size_t nd = io->GetNumberOfDimensions();
    size_t nc = io->GetNumberOfComponents();
    itk::ImageIOBase::IOComponentType  ct = io->GetComponentType();
    std::string ct_s = io->GetComponentTypeAsString(ct);
    
    if(verbose)
    {
      std::cout<<"dimensions:"<< nd << std::endl
               <<"components:"<< nc << std::endl
               <<"type:"<<ct_s.c_str()<<std::endl;
    }
    
    if(store_char)
      ct=itk::ImageIOBase::CHAR;
    else if(store_uchar)
      ct=itk::ImageIOBase::UCHAR;
    else if(store_short)
      ct=itk::ImageIOBase::SHORT;
    else if(store_ushort)
      ct=itk::ImageIOBase::USHORT;
    else if(store_int)
      ct=itk::ImageIOBase::INT;
    else if(store_uint)
      ct=itk::ImageIOBase::UINT;
    else if(store_float)
      ct=itk::ImageIOBase::FLOAT;
    else if(store_double)
      ct=itk::ImageIOBase::DOUBLE;
    
    ImageConverterBase *converter=NULL;
    
    if(nd==3 && nc==1)
    {
      if(verbose) std::cout<<"Writing 3D image..."<<std::endl;
      switch(ct)
      { 
        case itk::ImageIOBase::UCHAR :
          converter=new ImageConverter<itk::Image<unsigned char, 3> >();
          break;
        case itk::ImageIOBase::CHAR :
          converter=new ImageConverter<itk::Image<char, 3> >();
          break;
        case itk::ImageIOBase::USHORT :
          converter=new ImageConverter<itk::Image<unsigned short, 3> >();
          break;
        case itk::ImageIOBase::SHORT :
          converter=new ImageConverter<itk::Image<short, 3> >();
          break;
        case itk::ImageIOBase::INT :
          converter=new ImageConverter<itk::Image<int, 3> >();
          break; 
        case itk::ImageIOBase::UINT:
          converter=new ImageConverter<itk::Image<unsigned int, 3> >();
           break; 
        case itk::ImageIOBase::FLOAT :
          converter=new ImageConverter<itk::Image<float, 3> >();
          break; 
        case itk::ImageIOBase::DOUBLE:
          converter=new ImageConverter<itk::Image<double, 3> >();
          break; 
        default:
          itk::ExceptionObject("Unsupported component type");
      }
    } else if((nd==4 && nc==1 && !assume_dti)) {
      if(verbose) std::cout<<"Writing 4D image..."<<std::endl;
        switch(ct)
        {
          case itk::ImageIOBase::UCHAR:
            converter=new ImageConverter<itk::Image<unsigned char, 4> >();
            break;
          case itk::ImageIOBase::CHAR:
            converter=new ImageConverter<itk::Image<char, 4> >();
            break;
          case itk::ImageIOBase::USHORT:
            converter=new ImageConverter<itk::Image<unsigned short, 4> >();
            break;
          case itk::ImageIOBase::SHORT:
            converter=new ImageConverter<itk::Image<short, 4> >();
            break;
          case itk::ImageIOBase::INT:
            converter=new ImageConverter<itk::Image<int, 4> >();
            break; 
          case itk::ImageIOBase::UINT:
            converter=new ImageConverter<itk::Image<unsigned int, 4> >();
            break; 
          case itk::ImageIOBase::FLOAT:
            converter=new ImageConverter<itk::Image<float, 4> >();
            break; 
          case itk::ImageIOBase::DOUBLE:
            converter=new ImageConverter<itk::Image<double, 4> >();
            break; 
          default:
            itk::ExceptionObject("Unsupported component type");
        }
    } else if(((nd==3 && nc>1) || assume_dti)) {
        if(verbose) std::cout<<"Writing multicomponent 3D image..."<<std::endl;

        switch(ct)
        {
          case itk::ImageIOBase::UCHAR:
            converter=new ImageConverter<itk::VectorImage<unsigned char, 3> >();
            break;
          case itk::ImageIOBase::CHAR:
            converter=new ImageConverter<itk::VectorImage<char, 3> >();
            break;
          case itk::ImageIOBase::USHORT:
            converter=new ImageConverter<itk::VectorImage<unsigned short, 3> >();
            break;
          case itk::ImageIOBase::SHORT:
            converter=new ImageConverter<itk::VectorImage<short, 3> >();
            break;
          case itk::ImageIOBase::INT:
            converter=new ImageConverter<itk::VectorImage<int, 3> >();
            break; 
          case itk::ImageIOBase::UINT:
            converter=new ImageConverter<itk::VectorImage<unsigned int, 3> >();
            break; 
          case itk::ImageIOBase::FLOAT:
            converter=new ImageConverter<itk::VectorImage<float, 3> >();
            break; 
          case itk::ImageIOBase::DOUBLE:
            converter=new ImageConverter<itk::VectorImage<double, 3> >();
            break; 
          default:
            itk::ExceptionObject("Unsupported component type");
        }
    } else {
        throw itk::ExceptionObject("Unsupported number of dimensions");
    }
    
    if(converter)
    {
      converter->setup(verbose,assume_dti,use_b_matrix,inv_x,inv_y,inv_z,center,show_meta,history);
      converter->load_and_save_image(io,output.c_str());
      delete converter;
    }
  }
  
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return 2;
  }
  return 0;
}
