#include "stdafx.h"
#include "messagetypes.h"
#include "MyServer.h"
#include <iostream>
#include<string.h>
#include <cpprest/filestream.h>
#include<thread>
#include<chrono>

using namespace web; 
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
using namespace concurrency::streams; 

MyServer::MyServer(utility::string_t url) : m_listener(url){
    m_listener.support(methods::GET, std::bind(&MyServer::handle_get, this, std::placeholders::_1));
}

void MyServer::handle_get(http_request message){
    ucout <<  message.request_uri().query() << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	auto path_val = uri::split_path(message.request_uri().path());
	auto query_val = uri::split_query(message.request_uri().query());
	std::string token;
	token = path_val[1];
	/* for(int i=0;i<http_get_vars2.size();++i){
		std::cout<<http_get_vars2[i]<<"\n";
	} */
	auto fileStream = std::make_shared<ostream>();
	std::string fname;
	pplx::task<void> requestTask = fstream::open_ostream(U(query_val["file"])).then([=](ostream outFile)
    {
        *fileStream = outFile;
		return message.body().read_to_end(fileStream->streambuf());
    }).then([=](size_t)
    {
        return fileStream->close();
    });
	if(token=="compress"){
		const std::string comm = "sh compress.sh "+query_val["file"];
		system(comm.c_str());
		auto filestream = fstream::open_istream(U(query_val["file"]+".gz")).get();
		message.reply(status_codes::OK,filestream,_XPLATSTR("application/octet-stream"));
	}
	else if(token=="decompress"){
		const std::string comm = "sh decompress.sh "+query_val["file"];
		system(comm.c_str());
		//std::cout<<query_val["file"].substr(0,query_val["file"].length()-3);
		auto filestream = fstream::open_istream(U(query_val["file"].substr(0,query_val["file"].length()-3))).get();
		message.reply(status_codes::OK,filestream,_XPLATSTR("application/octet-stream"));
	}
	else
		message.reply(status_codes::NotFound);
};