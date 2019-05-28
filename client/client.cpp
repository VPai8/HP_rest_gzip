#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include "multipart_parser.h"
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
void disp(dirs dir){
    int i;
    std::cout<<dir.dir<<"\n";
    for(i=0;i<dir.file.size();++i){
        std::cout<<dir.file[i].first<<" "<<dir.file[i].second<<"\n"<<dir.fcontent[i]<<"\n";
    }
    for(i=0;i<dir.subdirs.size();++i){
        
        disp(dir.subdirs[i]);
        
    }
}
int main(int argc, char* argv[]){
      
        MultipartParser p1;
        std::string boundary = p1.boundary();
        p1.AddFile("file","newexp.cpp",1);
        p1.AddFile("folder","xyz",2);
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
	    std::vector<std::pair<std::string,int>> m = p2.GetBodyContent();
        return 0;
}
