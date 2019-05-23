/*********************************************************************************
*     File Name           :     gen_multipart.h
*     Created By          :     Ye Yangang
*     Creation Date       :     [2017-02-20 16:50]
*     Last Modified       :     [AUTO_UPDATE_BEFORE_SAVE]
*     Description         :     Generate multipart/form-data POST body
**********************************************************************************/

#ifndef GEN_MULTIPART_H_
#define GEN_MULTIPART_H_

#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <cpprest/filestream.h>
namespace web{
namespace http{

class MultipartParser
{
public:
  MultipartParser();
  inline const std::string body_content()
  {
    return body_content_;
  }
  inline const std::string boundary()
  {
    return boundary_;
  }
  inline void AddParameter(const std::string &name, const std::string &value)
  {
    params_.push_back(std::move(std::pair<std::string, std::string>(name, value)));
  }
  inline void AddFile(const std::string &name, const std::string &value)
  {
    files_.push_back(std::move(std::pair<std::string, std::string>(name, value)));
  }
  std::string &GenBodyContent();
  std::map<std::string,std::string> GetBodyContent();
  inline void SetBody(std::string content){
    body_content_= content;
  }
  inline void SetBound(std::string mime){
    int c = mime.find("boundary");
    boundary_ = mime.substr(c+9);
  }
private:
  void _get_file_name_type(const std::string &file_path, std::string *filenae, std::string *content_type);
private:
  static const std::string boundary_prefix_;
  static const std::string rand_chars_;
  std::string boundary_;
  std::string body_content_;
  std::vector<std::pair<std::string, std::string> > params_;
  std::vector<std::pair<std::string, std::string> > files_;
};

} //namespace web::http
} //namespace web

#endif
