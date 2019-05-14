#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

int main(int argc, char* argv[]){
    auto outfileStream = std::make_shared<ostream>();
    pplx::task<void> requestTask = fstream::open_istream(U(argv[1]))
    .then([=](istream inFile){
        http_client client(U("http://localhost:8080/HPServer/"));
        return client.request(methods::GET, argv[2] , inFile , _XPLATSTR("application/octet-stream"));
    })
    .then([=](http_response response){
        printf("Received response status code:%u\n", response.status_code());
    	std::string fname;
        if(argv[2][0]=='c')
            fname = std::string(argv[1])+".gz";
        else{
            std::string inter = std::string(argv[1]);
            fname = inter.substr(0,inter.length()-3);
        }
	    auto storeTask = fstream::open_ostream(U(fname)).get();   
        *outfileStream = storeTask; 
	    return response.body().read_to_end(outfileStream->streambuf());
    })
    .then([=](size_t){
        return outfileStream->close();
    });
    // Wait for all the outstanding I/O to complete and handle any exceptions
    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        printf("Error exception:%s\n", e.what());
    }

    return 0;
}
