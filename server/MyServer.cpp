#include "stdafx.h"
#include "messagetypes.h"
#include "MyServer.h"
#include <iostream>
#include<string.h>
#include <cpprest/filestream.h>
#include<thread>
#include<chrono>
#include "multipart_parser.h"
using namespace web; 
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
using namespace concurrency::streams; 

MyServer::MyServer(utility::string_t url) : m_listener(url){
    m_listener.support(methods::GET, std::bind(&MyServer::handle_get, this, std::placeholders::_1));
}

void MyServer::handle_get(http_request message){
    
	auto path_val = uri::split_path(message.request_uri().path());
	std::string token;
	token = path_val[1];  
		
	MultipartParser p1; 
    p1.SetBody(message.extract_string(true).get());
    p1.SetBound(message.headers().content_type());
	std::vector<std::pair<std::string,int>> m = p1.GetBodyContent();
    MultipartParser p2;
	std::string fname,ftype;
	for(int i=0;i<m.size();++i){ //run parallel
		if(token=="compress"){
			if(fs::is_directory(fs::path(m[i].first))){
				fname=m[i].first;
				ftype="folder";
			}
			else{
				fname=m[i].first+".gz";
				ftype="file";
			}
			const std::string comm = "gzip -r -"+ std::to_string(m[i].second) +" " + m[i].first;
			system(comm.c_str());
		}
		else{
			if(fs::is_directory(fs::path(m[i].first))){
				fname=m[i].first;
				ftype="folder";
			}
			else{
				fname=m[i].first.substr(0,m[i].first.length()-3);
				ftype="file";
			}
			const std::string comm = "gzip -rd "+ m[i].first;
			system(comm.c_str());
		}
		p2.AddFile(ftype,fname);
	}
	std::string boundary = p2.boundary();
    std::string body = p2.GenBodyContent();
	message.reply(status_codes::OK,body,U("multipart/form-data; boundary=" + boundary));
};