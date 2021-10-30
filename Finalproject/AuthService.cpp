#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens")
{
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response)
{
    std::string reqBody = request->getBody();
    WwwFormEncodedDict parsedReqBody = request->formEncodedBody();
    std::string username = parsedReqBody.get("username");

    for (size_t i = 0; i < username.length(); i++) //error checking to make sure its not uppercase.
    {
        if (isupper(username[i]))
        {
            response->setStatus(400);
            throw ClientError::notFound();
        }
    }

    std::string password = parsedReqBody.get("password");

    if (username.empty() == true || password.empty() == true)
    { //error checking to make sure these two strings exist.
        response->setStatus(400);
        throw ClientError::notFound();
    }

    std::map<std::string, User *>::iterator it;

    it = m_db->users.find(username); //iterate over users db to find if user exists
    if (username == it->first)
    { //user does exist
        if (password == it->second->password)
        { //password matches
            //craft response object here
            std::string token = StringUtils::createAuthToken();
            std::string user_id = it->second->user_id;

            User *temp = new User;
            temp->username = username;
            temp->password = password;

            m_db->users.insert(std::pair<std::string, User *>(username, temp));
            m_db->auth_tokens.insert(std::pair<std::string, User *>(token, temp));

            Document document;
            Document::AllocatorType &a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("auth_token", token, a);
            o.AddMember("user_id", user_id, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);

            // set the return object
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        }
        else
        {
            response->setStatus(403); //Password doesn't match
        }
    }
    else
    { //user does not exist
        std::string user_id = StringUtils::createAuthToken();
        std::string token = StringUtils::createAuthToken();

        User *temp = new User;
        temp->username = username;
        temp->user_id = user_id;
        temp->password = password;

        m_db->users.insert(std::pair<std::string, User *>(username, temp));
        m_db->auth_tokens.insert(std::pair<std::string, User *>(token, temp));

        Document document;
        Document::AllocatorType &a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("auth_token", token, a);
        o.AddMember("user_id", user_id, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);

        // set the return object
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
        response->setStatus(201);
    }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response)
{
    User *temp = getAuthenticatedUser(request);
    std::vector<std::string> arr = request->getPathComponents();
    std::string new_token = arr.back();
    std::string header_token = request->getAuthToken();

    std::map<std::string, User *>::iterator it;

    it = m_db->auth_tokens.find(new_token);
    if (it->second == temp)
    {
        m_db->auth_tokens.erase(it);
    }
}
