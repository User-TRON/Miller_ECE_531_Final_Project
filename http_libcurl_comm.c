//<Path to buildroot>/output/host/usr/bin/arm-linux-gcc -o http_libcurl_comm -lcurl -uClibc -lc http_libcurl_comm.c 

// ^ Compilation Command

//***************************
// ECE 531 Module 3 Project
//     Nathaniel Miller    
//***************************

/* This program uses libcurl to communicate via HTTP and get, post, put, or delete strings on a designated server.*/

#include <stdio.h>
#include <curl/curl.h>

#define OK		0
#define INIT_ERR	1
#define HTTP_ERR	2
#define INPUT_ERR       3

#define URL	"http://192.168.1.240:80"

int main(int argc, char** argv){
  fprintf(stderr, "Start http_libcurl_comm\n");

  process_arguments();



  CURL		*curl;
  CURLcode	res;

  curl = curl_easy_init();
  if(curl) {
    fprintf(stderr, "Curl init successful\n");
    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
      return HTTP_ERR;
    }
    curl_easy_cleanup(curl);
  }else{
    return INIT_ERR;
  }

  return OK;

}

void process_arguments(int argc, char** argv) {
  fprintf(stderr, "Num Arguments = %i\n",argc);

}
