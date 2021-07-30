//<Path to buildroot>/output/host/usr/bin/arm-linux-gcc -o thermostat -lcurl -uClibc -lc thermostat.c 

// ^ Compilation Command

//***************************
// ECE 531 Thermostat Daemon
//       Final Project
//     Nathaniel Miller    
//***************************
/* This daemon reads temperature from a thermocouple, reads/writes Command & Control data from a server, and turns on/off a heater system.*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <curl/curl.h>

#define true 1
#define false 0

enum error_type{OK=0, ERR_FORK, ERR_SETSID, ERR_CHDIR, ERR_NO_LOG_FILE, ERR_DISABLE_PRINT, ERR_USLEEP, ERR_TIME, ERR_SAVE_VAR, INIT_ERR, HTTP_ERR, INPUT_ERR, ERR_WTF};

#define DAEMON_NAME "thermostat"
#define ERROR_FORMAT "Received Errno %s"
#define ERROR_TIME_FORMAT "Received time errno %s"
#define ERROR_USLEEP_FORMAT "Received usleep errno %s"

//#define LOG_FILE_NAME "/var/log/messages"

#define DEBUG 1
//#undef DEBUG

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif //Debug macro from Allen Holub's book "Enough Rope to Shoot Yourself in the Foot: Rules for C and C++ Programming"



#define THERMOCOUPLE_FILE "/tmp/temp"
#define HEATER_POWER_FILE "/tmp/status"
#define AWS_STATUS_SERVER_URL "http://ec2-18-119-152-234.us-east-2.compute.amazonaws.com/thermostat_status_server.php"
#define AWS_SCHEDULE_SERVER_URL "http://ec2-18-119-152-234.us-east-2.compute.amazonaws.com/thermostat_schedule_server.php"


#define CONFIG_SLEEP 5
#define MAX_MESSAGE_SIZE 10000

char message[MAX_MESSAGE_SIZE]="";
long return_code=-1;

struct MemoryStruct {
  char *memory;
  size_t size;
};
struct MemoryStruct chunk;

struct StatusStruct {
  int id;
  unsigned long time_last_programmed;
  unsigned long time_last_update;
  int curr_temp;
  int set_temp;
  int power;
  int new_temp;
};

struct StatusStruct status;

enum DayEnum{Monday=1, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday};

struct ScheduleStruct {
  int id;
  enum DayEnum day;
  unsigned long time;
  int temperature;
  struct ScheduleStruct* next;
};

struct ScheduleStruct *schedule_head = NULL;
struct ScheduleStruct *schedule_current = NULL;

CURL *curl;
CURLcode res;

double thermocouple_temperature = 0.0;

static void read_server_status(void);
static void read_server_schedule(void);
static void read_thermocouple(void);
static void curl_init(char* url);
static void curl_run(void);
static void curl_cleanup(void);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static void extract_server_status(void);
static void extract_server_schedule(void);
static void create_schedule(void);
static void add_schedule(void);
static void delete_schedule(void);

static void read_server_status(void){
  D(syslog(LOG_INFO, "Read server config\n"));

  curl_init(AWS_STATUS_SERVER_URL);
  curl_run();
  extract_server_status();
  curl_cleanup();

}

static void read_server_schedule(void){
  D(syslog(LOG_INFO, "Read server schedule\n"));

  curl_init(AWS_SCHEDULE_SERVER_URL);
  curl_run();
  extract_server_schedule();
  curl_cleanup();

}

static void extract_server_status(void){
  D(syslog(LOG_INFO, "Extract server config\n"));

  char * token = strtok(chunk.memory, "[");
//  token = strtok(NULL, "]");

//  D(syslog(LOG_INFO, "EXTRACT RESULTS:"));
//  D(syslog(LOG_INFO, "%s", token));

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.id = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.time_last_programmed = (unsigned long)strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.time_last_update = (unsigned long)strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.curr_temp = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.set_temp = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.power = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.new_temp = strtol(token, NULL, 10);

    D(syslog(LOG_INFO, "ID                   = %i", status.id));
    D(syslog(LOG_INFO, "TIME_LAST_PROGRAMMED = %lu", status.time_last_programmed));
    D(syslog(LOG_INFO, "TIME_LAST_UPDATE     = %lu", status.time_last_update));
    D(syslog(LOG_INFO, "CURR_TEMP            = %i", status.curr_temp));
    D(syslog(LOG_INFO, "SET_TEMP             = %i", status.set_temp));
    D(syslog(LOG_INFO, "POWER                = %i", status.power));
    D(syslog(LOG_INFO, "NEW_TEMP             = %i", status.new_temp));

}

static void extract_server_schedule(void){
  D(syslog(LOG_INFO, "Extract server schedule\n"));

  char * token = strtok(chunk.memory, "[");
  token = strtok(NULL, "]");

  D(syslog(LOG_INFO, "EXTRACT RESULTS:"));
  D(syslog(LOG_INFO, "%s", token));

  create_schedule();

}

static void create_schedule(void){
  D(syslog(LOG_INFO, "Create schedule\n"));

}

static void add_schedule(void){
  D(syslog(LOG_INFO, "Add schedule\n"));

}

static void delete_schedule(void){
  D(syslog(LOG_INFO, "Delete schedule\n"));

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
static void curl_init(char* url){
  D(syslog(LOG_INFO, "Curl init\n"));


  chunk.memory = malloc(1);  //initialize memory to store curl response
  chunk.size = 0;    //set intial response to zero

  curl = curl_easy_init();
  if(!curl){
    syslog(LOG_INFO, "CURL ERROR\n");
    exit(INIT_ERR);
  }

  if(url == NULL){
    syslog(LOG_INFO, "MISSING URL\n");
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
static void curl_run(void){
  D(syslog(LOG_INFO, "Curl run\n"));
  res = curl_easy_perform(curl);
  if(res != CURLE_OK){
    exit(HTTP_ERR);
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code); //get curl response code

  D(syslog(LOG_INFO, "RESPONSE CODE: "));
  syslog(LOG_INFO, "%ld\n", return_code);

  D(syslog(LOG_INFO, "RESPONSE DATA:\n"));
  syslog(LOG_INFO, "%s\n", chunk.memory);
}

//curl_cleanup()
//input - none
//output - none
//Run curl cleanup command
static void curl_cleanup(void){
  D(syslog(LOG_INFO, "Curl cleanup\n"));
  curl_easy_cleanup(curl);

  free(chunk.memory); //clear allocated memory

}



static void read_thermocouple(void){
  syslog(LOG_INFO, "Read thermocouple\n");

  FILE* file_ptr;
  
  if((file_ptr = fopen(THERMOCOUPLE_FILE, "r")) == NULL){
    syslog(LOG_INFO, "Unable to read Thermocouple file, will try again\n");
    return;
  }

  fscanf(file_ptr, "%lf", &thermocouple_temperature);

  syslog(LOG_INFO, "Got thermocouple temperature = %f\n", thermocouple_temperature);

  fclose(file_ptr);

}

//_signal_handler()
//input - const int signal
//output - none
//Processes signal, closes syslog and exits when SIGTERM is received.
static void _signal_handler(const int signal){
  switch(signal){
    case SIGHUP: //continue
      break;
    case SIGTERM:
      syslog(LOG_INFO, "received SIGTERM, exiting.");
      closelog(); //close syslog
      exit(OK);
      break;
    default:
      syslog(LOG_INFO, "received unhandled signal"); //note unknown signal
  }
}

//_do_work()
//input - none
//output - none
//Function runs indefinitely, printing system time to syslog once per second.
static void _do_work(void){
  time_t sec, prev_sec;
  sec = time(NULL); //initialize time
  prev_sec = sec;

  syslog(LOG_INFO, "Initial Time since Epoch = %ld\n", sec);

  while(true){ //infinite loop

    //read config from server
    read_server_status();
    //read data from thermocouple
    read_thermocouple();
    //determine heater power state
    read_server_schedule();
    //set heater power
    //write data to server

    //sleep
    sleep(CONFIG_SLEEP);

/*    if(usleep(999000) != 0){ //sleep for just under 1 second and confirm sucess
      syslog(LOG_ERR, ERROR_USLEEP_FORMAT, strerror(errno)); //log error  and exit
      exit(ERR_USLEEP);
    }

    sec = time(NULL);
    if((sec < prev_sec) || (sec > prev_sec+1)){ //check if returned time is valid
      syslog(LOG_ERR, ERROR_TIME_FORMAT, strerror(errno));  //log error and exit
      exit(ERR_TIME);
    }

    while(sec < (prev_sec+1)){ //while current time is not 1 second past previous time
      if(usleep(1000) != 0){ //sleep for 1000th of a second and confirm sucess
        syslog(LOG_ERR, ERROR_USLEEP_FORMAT, strerror(errno)); //log error  and exit
        exit(ERR_USLEEP);
      }

      sec = time(NULL);
      if((sec < prev_sec) || (sec > prev_sec+1)){//check if returned time is valid
        syslog(LOG_ERR, ERROR_TIME_FORMAT, strerror(errno));  //log error and exit
        exit(ERR_TIME);
      }

    }

    prev_sec = sec; //save current time to previous
    if(prev_sec != sec){
      syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
      exit(ERR_SAVE_VAR);
    }

    syslog(LOG_INFO, "Time = %ld\n", sec); //log current time
   */
  }
}

int main(void){

  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON); //configure syslog information and parameters
  syslog(LOG_INFO, "Starting thermostat\n");

  pid_t pid=fork(); //fork to prevent blocking syslogd or initd

  if(pid<0) { //fork failed
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
    return ERR_FORK;
  } 

  if(pid>0){ //exit if parent
    return OK;
  }

  if(setsid() < -1){ //start new session and ensure leader
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log session error
    return ERR_SETSID;
  }

  //disable print functionality
  if((close(STDIN_FILENO) !=0) || (close(STDOUT_FILENO) !=0) || (close(STDERR_FILENO)!=0)){
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log disable print error
    return ERR_DISABLE_PRINT;
  }

  umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //set file permissions

  if(chdir("/")<0){ //set root directory
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log error setting root directory
    return ERR_CHDIR;
  }

//  if(access(LOG_FILE_NAME, F_OK) != 0){ //check if log file exists
//    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log error and exit
//    return ERR_NO_LOG_FILE;
//  }

  signal(SIGTERM, _signal_handler); //send signals to _signal_handler function
  signal(SIGHUP, _signal_handler);

  _do_work(); //run main loop

  return ERR_WTF; //main loop crashed
}
