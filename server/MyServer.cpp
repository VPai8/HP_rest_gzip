#include "stdafx.h"
#include "messagetypes.h"
#include "MyServer.h"
#include <iostream>
#include<string.h>
#include <cpprest/filestream.h>
#include<thread>
#include<chrono>
#include "multipart_parser.h"
#include "cmdm.h"
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
	std::vector<std::pair<std::string,std::vector<int>>> m = p1.GetBodyContent();
	 /* for(int i=0;i<m.size();++i){
		std::cout<<m[i].first<<" ";
		for(int j=0;j<m[i].second.size();++j)
			std::cout<<m[i].second[j]<<" ";
		std::cout<<"\n";
	}  */
    MultipartParser p2;
	std::string fname,ftype;
	fs::create_directory(fs::path("send"));
	string s="";
	if(token=="compress"){
		s+="Filename\tMode\tUsize\tCsize\tPerR\tTime\n";
		for(int i=0;i<m.size();++i){
			fs::path p("receive/"+m[i].first);
			if(fs::is_directory(p)){
				ftype="folder";
				for(int j=0;j<m[i].second.size();++j){
					fname=to_string(m[i].second[j])+"_"+m[i].first;
					string opath= "send/"+fname;
    				fs::create_directory(fs::path(opath));
					s+=compdir(fs::directory_iterator(p),m[i].second[j],fs::path(opath));
				}
			}
			else{
				ftype="file";
				for(int j=0;j<m[i].second.size();++j){
					fname=to_string(m[i].second[j])+"_"+m[i].first+".gz";
					s+=compress(p.string(),m[i].second[j],"send");
				}
			}
		}
		p2.AddFile(ftype,fname);
	}
	else{
		s+="Filename\tTime\n";
		for(int i=0;i<m.size();++i){
			fs::path p("receive/"+m[i].first);
			if(fs::is_directory(p)){
				fname=m[i].first;
				ftype="folder";
				s+=decompdir(fs::directory_iterator(p));
			}
			else{
				fname=m[i].first.substr(0,m[i].first.length()-3);
				ftype="file";
				s+=decompress(p.string(),"send");
			}
		}
		p2.AddFile(ftype,fname);
	}
	p2.SetFileDetails(s);
	std::string boundary = p2.boundary();
    std::string body = p2.GenBodyContent();
	string del = "rm -rf receive send";
	system(del.c_str());
	message.reply(status_codes::OK,body,U("multipart/form-data; boundary=" + boundary));
};