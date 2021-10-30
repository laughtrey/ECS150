#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;

HttpService::HttpService(string pathPrefix)
{
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)
{
  std::string reqBody = request->getBody();
  WwwFormEncodedDict parsedReqBody = request->formEncodedBody();
  std::string auth_token = request->getAuthToken(); //gets auth_token from request
  std::map<std::string, User *>::iterator it;
  it = m_db->auth_tokens.find(auth_token); 
  if (auth_token == it->first) //if the auth token exists in the auth_token map
  {
    return it->second; //return the user object value tied to the auth_token key
  }
  else{
    throw ClientError::notFound(); //else 404 ?
    return NULL;
  }
}

string HttpService::pathPrefix()
{
  return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response)
{
  cout << "HEAD " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response)
{
  cout << "GET " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response)
{
  cout << "PUT " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response)
{
  cout << "POST " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response)
{
  cout << "DELETE " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}
