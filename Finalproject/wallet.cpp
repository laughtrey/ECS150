#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;

void command(std::vector<string> input);

int main(int argc, char *argv[])
{
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0)
  {
    cout << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
  {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

  if (argc > 2)
  {
    fstream newfile;
    newfile.open(argv[1], ios::in); // open a file to perform write operation using file object
    if (newfile.is_open())
    { //checking whether the file is open
      string tp;
      while (getline(newfile, tp))
      {                       //read data from file object and put it into string.
        istringstream ss(tp); //print the data of the string
        string word;
        vector<string> sentence;
        while (ss >> word)
        {
          // print the read word
          sentence.push_back(word);
        }
        command(sentence);
      }

      newfile.close(); //close the file object.
    }
  }

  return 0;
}
else
{
  while (running)
  {
    //get input
    cout << "D$> ";
    string input;
    getline(std::cin, input);
    //split into words
    istringstream ss(input);
    string word;
    vector<string> sentence;
    while (ss >> word)
    {
      // print the read word
      sentence.push_back(word);
    }

    running = command(sentence);
  }
}

return 0;
}

int command(std::vector<string> input)
{
  if (input[0] == "auth")
  {
    auth(input);
  }

  if (input[0] == "balance")
  {
    balance(input);
  }
  if (input[0] == "deposit")
  {
    deposit(input);
  }
  if (input[0] == "send")
  {
    send(input);
  }
  if (input[0] == "logout")
  {
    del(input);
    return 0;
  }
  return 1;
}