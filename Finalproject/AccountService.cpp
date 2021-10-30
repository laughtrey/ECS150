#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users")
{
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response)
{
    User *temp = getAuthenticatedUser(request);
    std::string return_email = "";
    int return_balance = 0;
    std::string request_token = request->getAuthToken();
    if (request_token.empty() == true)
    {
        throw ClientError::notFound();
    }
    std::vector<std::string> arr = request->getPathComponents();
    std::string request_user_id = arr.back(); //user id to edit
    bool found = false;

    for (auto const &x : m_db->auth_tokens)
    {
        if (x.first == request_token)
        {
            if (x.second->user_id == request_user_id)
            {
                found = true;
                return_email = temp->email;
                return_balance = temp->balance;

                //craft response object
                Document document;
                Document::AllocatorType &a = document.GetAllocator();
                Value o;
                o.SetObject();
                o.AddMember("balance", return_balance, a);
                o.AddMember("email", return_email, a);
                document.Swap(o);
                StringBuffer buffer;
                PrettyWriter<StringBuffer> writer(buffer);
                document.Accept(writer);

                // set the return object
                response->setContentType("application/json");
                response->setBody(buffer.GetString() + string("\n"));
            }
        }
    }

    //error checking
    if (request_user_id.empty() == true || request_user_id != temp->user_id)
    {
        throw ClientError::notFound();
    }
    if (!found)
    {
        throw ClientError::notFound();
    }
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response)
{
    getAuthenticatedUser(request); //get an authenticated user object
    std::string reqBody = request->getBody();
    WwwFormEncodedDict parsedReqBody = request->formEncodedBody();
    std::string newemail = parsedReqBody.get("email");   //get the new email to update
    std::string request_token = request->getAuthToken(); //get the put requests x-auth token
    //error checking
    if (newemail.empty() == true)
    {
        response->setStatus(404);
    }
    std::vector<std::string> arr = request->getPathComponents();
    std::string user_id_to_edit = arr.back(); //user id to edit

    bool found = false;
    for (auto const &x : m_db->auth_tokens)
    {
        if (x.first == request_token)
        {
            if (x.second->user_id == user_id_to_edit)
            {
                found = true;
                x.second->email = newemail;          //changes it to the new email.
                int current_bal = x.second->balance; //gets the balance for response
                //craft response object
                Document document;
                Document::AllocatorType &a = document.GetAllocator();
                Value o;
                o.SetObject();
                o.AddMember("email", newemail, a);
                o.AddMember("balance", current_bal, a);
                document.Swap(o);
                StringBuffer buffer;
                PrettyWriter<StringBuffer> writer(buffer);
                document.Accept(writer);

                // set the return object
                response->setContentType("application/json");
                response->setBody(buffer.GetString() + string("\n"));
            }
        }
    }
    if (!found)
    {
        throw ClientError::notFound();
    }
}
