#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;
//const std::string publishable_key = "pk_test_51IyXIeHidJuZ1SfjAIDzPqOoc2cy9ZAlinTeZ2uOOxQFbqMFR56rhNh1Ad9AYyWvzjX8mJTUrwhtD9rVmLRTaSFi006A9wL7Jq"

DepositService::DepositService() : HttpService("/deposits")
{
}

void DepositService::post(HTTPRequest *request, HTTPResponse *response)
{
    User *destination = getAuthenticatedUser(request);
    std::string request_token = request->getAuthToken();
    std::string reqBody = request->getBody();
    WwwFormEncodedDict parsedReqBody = request->formEncodedBody();
    int request_amount = stoi(parsedReqBody.get("amount"));
    std::string request_stripe_token = parsedReqBody.get("stripe_token");

    //error checking
    if (request_amount <= 49 || request_token.empty() == true || request_stripe_token.empty() == true)
    {
        throw ClientError::unauthorized();
    }

    WwwFormEncodedDict body;
    body.set("amount", request_amount);
    body.set("currency", "USD");
    body.set("source", request_stripe_token);
    std::string encoded_body = body.encode();
    HttpClient client("api.stripe.com", 443, true);
    client.set_basic_auth(m_db->stripe_secret_key, "");
    HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);

    if (!client_response->success())
    {
        throw ClientError::unauthorized();
    }
    Document *d = client_response->jsonBody();
    string id = (*d)["id"].GetString();
    delete d;

    for (auto const &x : m_db->auth_tokens)
    {
        if (x.first == request_token)
        {
            std::string username = x.second->username; //get the username for response object
            x.second->balance += request_amount;       //add the deposit to the account

            Deposit *temp = new Deposit;
            temp->to = destination;
            temp->amount = request_amount;
            temp->stripe_charge_id = id;

            m_db->deposits.push_back(temp);
            //craft response object
            Document document;
            Document::AllocatorType &a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("balance", x.second->balance, a);
            //array here
            Value array;
            array.SetArray();
            for (int i = 0; i < int(m_db->deposits.size()); i++) //iterate over vector and find deposit history
            {
                if (destination == m_db->deposits[i]->to) //if this deposits user matches the entry in the deposit vector
                {
                    Value to;
                    to.SetObject();
                    to.AddMember("to", m_db->deposits[i]->to->username, a);
                    to.AddMember("amount", m_db->deposits[i]->amount, a);
                    to.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id, a);
                    array.PushBack(to, a);
                }
            }

            o.AddMember("deposits", array, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            //set response object
            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        }
    }
}
