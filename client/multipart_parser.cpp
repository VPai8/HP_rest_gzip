#include "multipart_parser.h"
#include <cpprest/filestream.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <future>
#include<cpprest/containerstream.h>

namespace web{
namespace http{

const std::string MultipartParser::boundary_prefix_("----CppRestSdkClient");
const std::string MultipartParser::rand_chars_("0123456789"
                                               "abcdefghijklmnopqrstuvwxyz"
                                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
MultipartParser::MultipartParser()
{
  int i = 0;
  int len = rand_chars_.size();
  boundary_ = boundary_prefix_;
  while(i < 16)
  {
    int idx = rand() % len;
    boundary_.push_back(rand_chars_[idx]);
    ++i;
  }
}

std::string &MultipartParser::GenBodyContent()
{
  std::vector<std::future<std::vector<unsigned char>>> futures;
  body_content_.clear();
  for(auto &file:files_)
  {
    std::future<std::vector<unsigned char>> content_futures = std::async(std::launch::async, [&file]()
    {
      std::ifstream ifile(file.second, std::ios::binary | std::ios::ate);
      std::streamsize size = ifile.tellg();
      ifile.seekg(0, std::ios::beg);
      std::vector<unsigned char> buff(size);
      ifile.read((char*)&buff[0], size);
      ifile.close();
      return buff;
    });
    futures.push_back(std::move(content_futures));
  }

  for(size_t i = 0; i < files_.size(); ++i){
    std::string file_content = utility::conversions::to_base64(futures[i].get());
    body_content_ += boundary_;
    body_content_ += "\nContent-Disposition: form-data; name=\"file\"; filename=\"";
    body_content_ += files_[i].second;
    body_content_ += "\"\nContent-Length=\""; //added for ease in parsing body. 
            //should be content-type. can be obtained with file --mime-type 
    body_content_ += std::to_string(file_content.length());
    body_content_ += "\"\n\n";
    body_content_ += file_content;
    body_content_ += "\n";  
  }
  body_content_ += boundary_;
  return body_content_;
}

std::map<std::string,std::string> MultipartParser::GetBodyContent(){
  std::string boundary = boundary_;
  unsigned long long c=0;
  unsigned long long found = body_content_.find_first_of("\n",c);
  unsigned long long lenbody = body_content_.length(),fsize;
  std::map<std::string,std::string> m,n;
  std::string bound,fname,fcontent;
  while (found<=lenbody){
      bound = body_content_.substr(c,found-c);
      if(bound==boundary){
        c=found+1;
        found = body_content_.find("filename",c);
        c=found+10;
        found = body_content_.find("\"",c);
        fname = body_content_.substr(c,found-c);
        c=found+1;
        found = body_content_.find("Content-Length",c);
        c=found+16;
        found = body_content_.find("\"",c);
        std::istringstream iss(body_content_.substr(c,found-c));
        iss>>fsize;
        c=found+3;        
        fcontent = body_content_.substr(c,fsize);
        m.insert(std::pair<std::string,std::string>(fname,fcontent));
        c=c+fsize+1;
        found = body_content_.find_first_of("\n",c);
      }
      else
       return n;
  }
  return m;
}

void MultipartParser::_get_file_name_type(const std::string &file_path, std::string *filename, std::string *content_type)
{
  if (filename == NULL || content_type == NULL) return;

  size_t last_spliter = file_path.find_last_of("/\\");
  *filename = file_path.substr(last_spliter + 1);
  size_t dot_pos = filename->find_last_of(".");
  if (dot_pos == std::string::npos)
  {
    *content_type = "application/octet-stream";
    return;
  }
  std::string ext = filename->substr(dot_pos + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  if (ext == "jpg" || ext == "jpeg")
  {
    *content_type = "image/jpeg";
    return;
  }
  if (ext == "txt" || ext == "log")
  {
    *content_type = "text/plain";
    return;
  }
  *content_type = "application/octet-stream";
  return;
}

} //namespace web::http
} //namespace web

