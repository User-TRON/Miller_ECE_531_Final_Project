//<Path to buildroot>/output/host/usr/bin/arm-linux-gcc -o http_libcurl_comm -lcurl -uClibc -lc http_libcurl_comm.c 

// ^ Compilation Command

//***************************
// ECE 531 Module 3 Project
//     Nathaniel Miller    
//***************************
/* This program uses libcurl to communicate via HTTP and get, post, put, or delete strings on a designated server.*/

#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

#define OK		0
#define INIT_ERR	1
#define HTTP_ERR	2
#define INPUT_ERR       3

#define DEBUG 1
#undef DEBUG

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif //Debug macro from Allen Holub's book "Enough Rope to Shoot Yourself in the Foot: Rules for C and C++ Programming"

#define MAX_MESSAGE_SIZE 10000

char* url="";
char message[MAX_MESSAGE_SIZE]="";
long return_code=-1;

enum command_type{unknown=0,post,get,put,delete};
enum command_type command = unknown;

CURL *curl;
CURLcode res;

struct MemoryStruct {
  char *memory;
  size_t size;
};

struct MemoryStruct chunk;

void print_help(void);
void process_arguments(int argc, char** argv);
void curl_init(void);
void curl_run(void);
void curl_cleanup(void);
void run_post(void);
void run_get(void);
void run_put(void);
void run_delete(void);
void run_command(void);

int main(int argc, char** argv){
  D(fprintf(stderr, "Start http_libcurl_comm\n"));

  chunk.memory = malloc(1);  //initialize memory to store curl response
  chunk.size = 0;    //set intial response to zero

  process_arguments(argc, argv); //process command line arguments
  run_command(); //run curl based upon command line arguments

  free(chunk.memory); //clear allocated memory

  return OK;
}

//WriteMemoryCallback()
//input - void *contents - new response string
//      - size_t size - size of new response string
//      - size_t nmemb - size of new response string
//      - void *userp - struct of response data
//output - size_t
//HTTP delete request
//From curl.se/libcurl/c/getinmemory.html
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb; //set size of new response string
  struct MemoryStruct *mem = (struct MemoryStruct *)userp; //populate memory struct

  char *ptr = realloc(mem->memory, mem->size + realsize + 1); //add space for new response string
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr; //save larger space
  memcpy(&(mem->memory[mem->size]), contents, realsize); //copy new response to larger space
  mem->size += realsize;  //update size of string
  mem->memory[mem->size] = 0; //reset struct

  return realsize;
}

//curl_init()
//input - none
//output - none
//Initializes curl, sets curl url, sets curl to use function to collect return data
void curl_init(void){
  D(fprintf(stderr, "Curl init\n"));

  curl = curl_easy_init();
  if(!curl){
    fprintf(stderr, "CURL ERROR\n");
    exit(INIT_ERR);
  }

  if(url == NULL){
    fprintf(stderr, "MISSING URL\n");
    exit(INPUT_ERR);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); //callback function to save curl response to memory
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk); //memory to store curl response
}

//curl_run()
//input - none
//output - none
//Runs curl command, collects response code, prints response code and return data
void curl_run(void){
  D(fprintf(stderr,"Curl run\n"));
  res = curl_easy_perform(curl);
  if(res != CURLE_OK){
    exit(HTTP_ERR);
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code); //get curl response code

  D(fprintf(stderr, "RESPONSE CODE: "));
  fprintf(stderr, "%ld\n", return_code);

  D(fprintf(stderr, "RESPONSE DATA:\n"));
  fprintf(stderr, "%s\n", chunk.memory); 
}

//curl_cleanup()
//input - none
//output - none
//Run curl cleanup command
void curl_cleanup(void){
  D(fprintf(stderr, "Curl cleanup\n"));
  curl_easy_cleanup(curl);
}

//run_post()
//input - none
//output - none
//HTTP post request
void run_post(void){
  D(fprintf(stderr, "post\n"));

  curl_init();

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message); //add message contents to POST request

  curl_run();

  curl_cleanup();
}

//run_get()
//input - none
//output - none
//HTTP get request
void run_get(void){
  D(fprintf(stderr, "get\n"));

  curl_init();

  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L); //configure curl for get request

  curl_run();
 
  curl_cleanup();
}

//run_put()
//input - none
//output - none
//HTTP put request
void run_put(void){
  D(fprintf(stderr, "put\n"));

  curl_init();

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); //configure curl to ask for PUT request
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message); //add message contents to PUT request

  curl_run();

  curl_cleanup();
}

//run_delete()
//input - none
//output - none
//HTTP delete request
void run_delete(void){
  D(fprintf(stderr, "delete\n"));

  curl_init();

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE"); //configure curl to ask for DELETE request
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message); //add message contents to DELETE request

  curl_run();

  curl_cleanup();
}

//run_command()
//input - none
//output - none
//switches based on command line argument input and runs corresponding http request type
void run_command(void){
  D(fprintf(stderr, "run_command\n"));

  switch (command){ //command line argument for request type
    case post:
      run_post();
      break;
    case get:
      run_get();
      break;
    case put:
      run_put();
      break;
    case delete:
      run_delete();
      break;
    case unknown:	    
      fprintf(stderr, "INVALID COMMAND\n");
      exit(INPUT_ERR);

  }
}

//print_help()
//input - none
//output - none
//Prints the help information about how to use http_libcurl_comm
void print_help(void){
  fprintf(stderr, "Usage: http_libcurl_comm [-h] -g|-o|-p|-d -u <url> [<message>]\n");
  fprintf(stderr, "    -h, --help     Print help text\n");
  fprintf(stderr, "    -g, --get      Get message from specified URL\n");
  fprintf(stderr, "    -o, --post     Post message to specified URL\n");
  fprintf(stderr, "    -p, --put      Put message at specified URL\n");
  fprintf(stderr, "    -d, --delete   Delete message from specified URL\n");
  fprintf(stderr, "    -u, --url      URL to communcate with\n");
  fprintf(stderr, " NOTE: Maximum message size is limited to 10,000 characters\n");
}

//process_arguments()
//input - argc - number of arguments
//      - char** argv - array of argument strings
//output - none
//Processes command line argument inputs, calls print_help if error is encountered or help argument passed, sets command, url, and message if populated.
void process_arguments(int argc, char** argv) {
  D(fprintf(stderr, "Num Arguments = %i\n",argc));

  if(argc <= 1){
    fprintf(stderr, "TOO FEW ARGUMENTS\n");
    print_help();
    exit(INPUT_ERR);
  }

  for(int i=1; i<argc; i++){
    D(fprintf(stderr, "%d %s\n", i, argv[i]));

    if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ){
      print_help();
      exit(OK);

    }else if(!strcmp(argv[i], "-g") || !strcmp(argv[i], "--get") ){
      //get
      D(fprintf(stderr, "get command\n"));
      command = get;

    }else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--post") ){
      //post
      D(fprintf(stderr, "post command\n"));
      command = post;

    }else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--put") ){
      //put
      D(fprintf(stderr, "put command\n"));
      command = put;

    }else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--delete") ){
      //delete
      D(fprintf(stderr, "delete command\n"));
      command = delete;

    }else if(!strcmp(argv[i], "-u") || !strcmp(argv[i], "--url") ){
      //url
      i++;
      if(i >= argc){
        fprintf(stderr, "MISSING URL\n");
	print_help();
	exit(INPUT_ERR);
      }

      D(fprintf(stderr, "%d %s\n", i, argv[i]));
      url = argv[i];
      D(fprintf(stderr, "Saved URL: %s\n", url));

    }else if((command != unknown) && (command != get)){ //assume any argument that did not match above is the message
      if(message !=NULL){ //add space since command line doesn't add spaces unless "quotes" are used around entire message string
        strcat(message," ");
      }
      strcat(message,argv[i]); //add next segment of message to previously processed message strings
      D(fprintf(stderr, "Got Command %d, saved message: %s\n",command,message));

    }else{
      fprintf(stderr, "INVALID ARGUMENT INPUT\n");
      print_help();
      exit(INPUT_ERR);
    }
  }
}
