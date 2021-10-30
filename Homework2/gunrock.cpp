#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <deque>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
int BUFFER_SIZE = 1;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "/dev/null";

vector<HttpService *> services;

HttpService *find_service(HTTPRequest *request)
{
  // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++)
  {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0)
    {
      return services[idx];
    }
  }

  return NULL;
}

void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response)
{
  stringstream payload;

  // invoke the service if we found one
  if (service == NULL)
  {
    // not found status
    response->setStatus(404);
  }
  else if (request->isHead())
  {
    payload << "HEAD " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->head(request, response);
  }
  else if (request->isGet())
  {
    payload << "GET " << request->getPath();
    sync_print("invoke_service_method", payload.str());
    cout << payload.str() << endl;
    service->get(request, response);
  }
  else
  {
    // not implemented status
    response->setStatus(405);
  }
}

void handle_request(MySocket *client)
{
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;

  // read in the request
  bool readResult = false;
  try
  {
    payload << "client: " << (void *)client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  }
  catch (...)
  {
    // swallow it
  }

  if (!readResult)
  {
    // there was a problem reading in the request, bail
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return;
  }

  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str("");
  payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *)client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());

  delete response;
  delete request;

  payload.str("");
  payload.clear();
  payload << " client: " << (void *)client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
}

/**===func I made===
====================**/

deque<MySocket *> BUFFER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buff_avail = PTHREAD_COND_INITIALIZER;
pthread_cond_t client_avail = PTHREAD_COND_INITIALIZER;

void *producer();
void *consumer(void *arg);

//end student made functs.

int main(int argc, char *argv[])
{

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:")) != -1)
  {
    switch (option)
    {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    default:
      cerr << "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }
  set_log_file(LOGFILE);

  sync_print("init", "");

  services.push_back(new FileService(BASEDIR));

  sync_print("waiting_to_accept", "");
  producer();
}

void *producer()
{
  pthread_t thread[THREAD_POOL_SIZE];
  for (int i = 0; i < THREAD_POOL_SIZE; i++)
  {                                                   //Create threads based on the size passed into argument
    dthread_create(&thread[i], NULL, consumer, NULL); //they are going to do the consumer function
    dthread_detach(thread[i]);
  }

  MyServerSocket *server = new MyServerSocket(PORT);
  MySocket *client;

  while (true)
  {
    sync_print("waiting_to_accept", "");
    client = server->accept(); //accept HTTP request from over the network
    sync_print("client_accepted", "");

    dthread_mutex_lock(&lock); //Lock the threads

    while ((int)BUFFER.size() == BUFFER_SIZE)
    {                                        //if the buffer is full based on arg value of buffer
      dthread_cond_wait(&buff_avail, &lock); //wait, and release lock
    }                                        //end while loop when buffer isnt full
    BUFFER.push_back(client);                //add client to buffer if the buffer isn't full
    dthread_cond_signal(&client_avail);      //signal thread availability
    dthread_mutex_unlock(&lock);             //unlock threads
    //handle_request(client);
  }
}

void *consumer(void *arg)
{
  MySocket *client;          //declare a MySocket
  while (true)
  {
    dthread_mutex_lock(&lock); //lock to check the buffer

    while (BUFFER.empty())
    { //If the buffer is empty, ie no jobs for the threads
      dthread_cond_wait(&client_avail, &lock); //wait, and release lock
    }                                          //exit when there is something in the queue

    client = BUFFER.front();          //make the client the oldest element on the queue
    BUFFER.pop_front();               //remove the oldest element on the queue
    dthread_cond_signal(&buff_avail); //signal that the queue is no longer full, if it was
    dthread_mutex_unlock(&lock);      //unlock so other threads can access the queue
    handle_request(client);           //serve the request.
  }
  //return NULL;                                  //this makes the compiler happy.
}