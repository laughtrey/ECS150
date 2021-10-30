#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") {}

void TransferService::post(HTTPRequest *request, HTTPResponse *response)
{
    User *from = getAuthenticatedUser(request); //Source user object
    std::string reqBody = request->getBody();
    WwwFormEncodedDict parsedReqBody = request->formEncodedBody();
    int amount = stoi(parsedReqBody.get("amount"));         //getting the amount and making it an int
    std::string destination_user = parsedReqBody.get("to"); //destination username

    int new_balance = from->balance - amount; //their new balance, subtracting the money they transfer.

    //error checking passing tests
    if (parsedReqBody.get("amount") == "" || new_balance < 0 || destination_user.empty() == true || amount < 0 || from->balance < amount)
    {
        throw ClientError::notFound();
    }
    std::map<std::string, User *>::iterator it;
    it = m_db->users.find(destination_user); //iterate over the usernames for the destination user
    if (it == m_db->users.end())
    {
        throw ClientError::notFound();
    }
    //end error checking

    bool found = false;
    it = m_db->users.find(destination_user); //iterate over the usernames for the destination user
    if (destination_user == it->first)
    { //user does exist
        found = true;
        it->second->balance += amount; //depositing the amount into their balance.
        from->balance -= amount;       //setting the new balance.

        Transfer *temp = new Transfer;
        temp->from = from;
        temp->to = it->second;
        temp->amount = amount;

        m_db->transfers.push_back(temp);

        //craft response object
        Document document;
        Document::AllocatorType &a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("balance", from->balance, a);
        //array here
        Value array;
        array.SetArray();
        for (int i = 0; i < int(m_db->transfers.size()); i++) //iterate over vector and find transfer history
        {
            if (from == m_db->transfers[i]->from) //if this deposits user matches the entry in the transfer vector
            {
                Value to;
                to.SetObject();
                to.AddMember("from", m_db->transfers[i]->from->username, a);
                to.AddMember("to", m_db->transfers[i]->to->username, a);
                to.AddMember("amount", m_db->transfers[i]->amount, a);
                array.PushBack(to, a);
            }
        }

        o.AddMember("transfers", array, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
    }
    if (!found)
    {
        throw ClientError::notFound();
    }
}
