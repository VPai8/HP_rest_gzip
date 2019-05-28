#include "multipart_parser.h"
/* #include <cpprest/filestream.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <future>
#include <cpprest/containerstream.h>
#include <boost/filesystem.hpp> */
namespace fs = boost::filesystem;

namespace web{
namespace http{

const std::string MultipartParser::boundary_prefix_("--CppRestSdkClient");
const std::string MultipartParser::rand_chars_("0123456789"
                                               "abcdefghijklmnopqrstuvwxyz"
                                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

std::string MultipartParser::GenBound(){
  int i = 0;
  int len = rand_chars_.size();
  std::string boundary = boundary_prefix_;
  while(i < 16){
    int idx = rand() % len;
    boundary.push_back(rand_chars_[idx]);
    ++i;
  }
  return boundary;
}


dirs MultipartParser::GetDirs(fs::directory_iterator iter){
  dirs d;
  d.dir = iter->path().parent_path().filename().string();
  fs::directory_iterator end;
  for(;iter!=end;iter++){
    if(fs::is_directory(iter->path())){
      if(!fs::is_empty(iter->path()))
          d.subdirs.push_back(GetDirs(fs::directory_iterator(iter->path())));
      else{
          dirs dtemp;
          dtemp.dir = iter->path().filename().string();
          d.subdirs.push_back(dtemp);
      }
    }
    else{
      std::ifstream ifile(iter->path().string(), std::ios::binary | std::ios::ate);
      std::streamsize size = ifile.tellg();
      ifile.seekg(0, std::ios::beg);
      std::vector<unsigned char> buff(size);
      ifile.read((char*)&buff[0], size);
      ifile.close();
      std::string fcontent = utility::conversions::to_base64(buff);
      d.file.push_back(std::make_pair(iter->path().filename().string(),fcontent.length()));
      d.fcontent.push_back(fcontent);
    }
  }
  return d;
}
void MultipartParser::MakeDirs(fs::path p,dirs d){
  fs::path dirpath(p/=fs::path(d.dir));
  fs::create_directory(dirpath);
  for(int i=0;i<d.file.size();++i){
      std::vector<unsigned char> data = utility::conversions::from_base64(d.fcontent[i]);
    	std::ofstream outfile(dirpath.string()+"/"+d.file[i].first, std::ios::out | std::ios::binary); 
    	outfile.write(reinterpret_cast<const char *>(data.data()), data.size()); 
  }
  for(int i=0;i<d.subdirs.size();++i){
    MakeDirs(dirpath,d.subdirs[i]);
  }
}
std::string MultipartParser::GetDirMeta(dirs d){
    std::string meta="";
    meta+="/";
    meta+=d.dir;
    meta+="\n";
    for(int i = 0; i < d.file.size(); ++i){
        meta+= d.file[i].first;
        meta+= " ";
        meta+= std::to_string(d.file[i].second);
        meta+= "\n";
    }
    for(int i=0;i<d.subdirs.size();++i)
        meta+= GetDirMeta(d.subdirs[i]);
    meta+= "\\\n";
    return meta;
}

std::string MultipartParser::GetDirData(dirs d,std::string bound){
  std::string data="";
  for(int i = 0; i < d.fcontent.size(); ++i){
    data += "\r\n";
    data += bound;
    data += "\r\nContent-Disposition: file"; 
    data += "\nContent-Type=\"application/octet-stream\""; 
    data += "\nContent-Transfer-Encoding: base64\n\n";
    data += d.fcontent[i];
  }
  for(int i=0;i<d.subdirs.size();++i)
    data+= GetDirData(d.subdirs[i],bound);
  return data;
}

dirs MultipartParser::ParseDirData(dirs d){
  unsigned long long found;
  
  for(int i = 0; i < d.file.size(); ++i){
    for(int j=0;j<5;++j){
      found = body_content_.find("\n",parse);
      parse=found+1;
    }
    d.fcontent.push_back(body_content_.substr(parse,d.file[i].second));
    parse = parse+d.file[i].second+2;
  }
  for(int i=0;i<d.subdirs.size();++i)
    d.subdirs[i]= ParseDirData(d.subdirs[i]);
  return d;
}

std::string MultipartParser::GenBodyContent(){
  
  body_content_.clear();
  dirs d;
  d.dir=".";
  std::vector<std::string> mfile;
  std::vector<int> mode;
  for(auto &file:files_){
    mfile.push_back(file.first.second);
    mode.push_back(file.second);
    if(file.first.first=="file"){
      std::future<std::vector<unsigned char>> content_futures = std::async(std::launch::async, [&file](){
        std::ifstream ifile(file.first.second, std::ios::binary | std::ios::ate);
        std::streamsize size = ifile.tellg();
        ifile.seekg(0, std::ios::beg);
        std::vector<unsigned char> buff(size);
        ifile.read((char*)&buff[0], size);
        ifile.close();
        return buff;
      });
      d.fcontent.push_back(utility::conversions::to_base64(content_futures.get()));
      d.file.push_back(std::make_pair(file.first.second,d.fcontent.back().length()));
    }
    else{
      if(!fs::is_empty(file.first.second))
        d.subdirs.push_back(GetDirs(fs::directory_iterator(file.first.second)));
      else{
        dirs dtemp;
        dtemp.dir = file.first.second;
        d.subdirs.push_back(dtemp);
      }
    }   
  }
  
  body_content_ += "\r\n";
  body_content_ += boundary_;
  body_content_ += "\r\nContent-Disposition: form-data; name=\"metadata\"\n";    
  body_content_ += "Content-Type=\"text/plain\""; 
  body_content_ += "\nContent-Transfer-Encoding: base64\n\n";
  std::string bodymeta;
  bodymeta+="/.\n";
  for(int i = 0; i < d.file.size(); ++i){
    bodymeta+= d.file[i].first;
    bodymeta+= " ";
    bodymeta+= std::to_string(d.file[i].second);
    bodymeta+= " ";
    bodymeta+= std::to_string(mode[std::distance(mfile.begin(),std::find(mfile.begin(),mfile.end(),d.file[i].first))]);
    bodymeta+= "\n";
  }
  for(int i = 0; i < d.subdirs.size(); ++i){
    bodymeta+= GetDirMeta(d.subdirs[i]);
    bodymeta+= std::to_string(mode[std::distance(mfile.begin(),std::find(mfile.begin(),mfile.end(),d.subdirs[i].dir))]);
    bodymeta+= "\n";
  }
  bodymeta+="\\\n";
  body_content_ += utility::conversions::to_base64(std::vector<unsigned char> (bodymeta.begin(),bodymeta.end()));
  body_content_ += "\r\n";
  body_content_ += boundary_;
  body_content_ += "\r\nContent-Disposition: form-data; name=\"dirs\"\n";    
  body_content_ += "Content-Type=\"multipart/mixed\"; "; 
  body_content_ += "boundary=";
  std::string sub_bound = GenBound();
  body_content_ += sub_bound;
  body_content_ += GetDirData(d,sub_bound);
  body_content_ += "\r\n";
  body_content_ += sub_bound+"--";
  body_content_ += "\r\n";
  body_content_ += boundary_+"--";
  body_content_ += "\r\n";
  return body_content_;
}



dirs MultipartParser::ParseDirMeta(std::string meta){
  dirs d;
  int found = meta.find("\n",parse);
  d.dir = meta.substr(parse+1,found-parse-1);
  parse = found+1;
  while(meta[parse]!='\\'){
    if(meta[parse]=='/'){
      d.subdirs.push_back(ParseDirMeta(meta));
    }
    else{
      std::string fname;
      unsigned long fsize;
      int mode;
      found = meta.find(" ",parse);
      fname = meta.substr(parse,found-parse);
      parse = found+1;
      found = meta.find("\n",parse);
      std::istringstream t1(meta.substr(parse,found-parse));
      t1>>fsize;
      parse = found+1;
      d.file.push_back(std::make_pair(fname,fsize));
    }
  }
  parse+=2;
  return d;
}

std::vector<std::pair<std::string,int>> MultipartParser::GetBodyContent(){
  std::string boundary = boundary_;
  unsigned long long p=2;
  unsigned long long found = body_content_.find("\r",p);
  unsigned long long lenbody = body_content_.length(),fsize;
  std::vector<std::pair<std::string,int>> m;
  dirs d;
  d.dir=".";
  std::string bound;
  bound = body_content_.substr(p,found-p);
  while (bound!=boundary_+"--"){
    p=found+2;
    found = body_content_.find("\n",p);
    p=found+1;
    found = body_content_.find("\n",p);
    p=found+1;
    found = body_content_.find("\n",p);
    p=found+1;
    found = body_content_.find("\n",p);
    p=found+1;
    
    found = body_content_.find("\r",p);
    std::vector<unsigned char> v= utility::conversions::from_base64(body_content_.substr(p,found-p));
    std::string meta=std::string(reinterpret_cast<const char*>(&v[0]),v.size());    
    int mode,f;
    parse=3;
    while(meta[parse]!='\\'){
      if(meta[parse]=='/'){
        d.subdirs.push_back(ParseDirMeta(meta));
        f = meta.find("\n",parse);
        std::istringstream t1(meta.substr(parse,f-parse));
        t1>>mode;
        parse=f+1;
        m.push_back(std::make_pair(d.subdirs.back().dir,mode));
      }
      else{
        std::string fname;
        unsigned long fsize;
        f = meta.find(" ",parse);
        fname = meta.substr(parse,f-parse);
        parse = f+1;
        f = meta.find(" ",parse);
        std::istringstream t1(meta.substr(parse,f-parse));
        t1>>fsize;
        parse = f+1;
        f = meta.find("\n",parse);
        std::istringstream t2(meta.substr(parse,f-parse));
        t2>>mode;
        parse=f+1;
        d.file.push_back(std::make_pair(fname,fsize));
        m.push_back(std::make_pair(fname,mode));
      }
    }
    p=found+2;
    found = body_content_.find("\n",p);
    p=found+1;
    found = body_content_.find("\n",p);
    p=found+1;
    found = body_content_.find(" ",p);
    p=found+2;
    found = body_content_.find("=",p);
    p=found+1;
    found = body_content_.find("\r",p);
    p=found+2;
    parse=p;
    d = ParseDirData(d);
    //parse rest of the boundary. also compression details.
    for(int i=0;i<d.file.size();++i){
      std::vector<unsigned char> data = utility::conversions::from_base64(d.fcontent[i]);
    	std::ofstream outfile(d.file[i].first, std::ios::out | std::ios::binary); 
    	outfile.write(reinterpret_cast<const char *>(data.data()), data.size()); 
    }
    for(int i=0;i<d.subdirs.size();++i){
      MakeDirs(fs::path("."),d.subdirs[i]);
    }
    
    return m;
  }
  //return std::make_pair(d,m);
} 
} //namespace web::http
} //namespace web

