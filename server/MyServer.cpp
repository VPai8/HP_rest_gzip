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
	std::map<std::string,std::string> m = p1.GetBodyContent();
    std::map<std::string,std::string>::iterator it;
	MultipartParser p2;
	std::string fname;
	for(it=m.begin();it!=m.end();++it){ //run parallel
		std::vector<unsigned char> data = utility::conversions::from_base64(it->second);
    	std::ofstream outfile(it->first, std::ios::out | std::ios::binary); 
    	outfile.write(reinterpret_cast<const char *>(data.data()), data.size()); 
		if(token=="compress"){
			const std::string comm = "sh compress.sh "+ it->first;
			system(comm.c_str());
			fname = it->first+".gz";
		}
		else{
			const std::string comm = "sh decompress.sh "+ it->first;
			system(comm.c_str());
			fname = (it->first).substr(0,(it->first).length()-3);
		}
		p2.AddFile("file", fname);
	}
	std::string boundary = p2.boundary();
    std::string body = p2.GenBodyContent();
	message.reply(status_codes::OK,body,U("multipart/form-data; boundary=" + boundary));
};