#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include "multipart_parser.h"
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

int main(int argc, char* argv[]){
      
        MultipartParser p1;
        p1.AddFile("file", "abc.jpeg");
        p1.AddFile("file","splice.mp4");
        p1.AddFile("file", "client.cpp");
        utility::string_t boundary = p1.boundary();
        std::string body = p1.GenBodyContent();
        
        http_request req;
        http_client client(U("http://localhost:8080/HPServer/"));
        req.set_request_uri("compress");
        req.set_method(web::http::methods::GET);
        req.set_body(body,U("multipart/form-data; boundary="+boundary));
        
        pplx::task<http_response> reqtask = client.request(req);
        http_response response = reqtask.get(); 
        printf("Received response status code:%u\n", response.status_code());
        
        MultipartParser p2;
        p2.SetBody(response.extract_string(true).get());
        p2.SetBound(response.headers().content_type());
	    std::map<std::string,std::string> m = p2.GetBodyContent();
        std::map<std::string,std::string>::iterator it;
        std::vector<unsigned char> data;
        for(it=m.begin();it!=m.end();++it){
            data = utility::conversions::from_base64(it->second);
            std::ofstream outfile(it->first, std::ios::out | std::ios::binary); 
            outfile.write(reinterpret_cast<const char *>(data.data()), data.size()); 
        }
        
        return 0;
}
