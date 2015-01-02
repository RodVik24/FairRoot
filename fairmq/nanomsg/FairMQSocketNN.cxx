/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSocketNN.cxx
 *
 * @since 2012-12-05
 * @author A. Rybalchenko
 */

#include <sstream>

#include "FairMQSocketNN.h"
#include "FairMQMessageNN.h"
#include "FairMQLogger.h"

FairMQSocketNN::FairMQSocketNN(const string& type, int num, int numIoThreads)
    : fSocket()
    , fId()
    , fBytesTx(0)
    , fBytesRx(0)
    , fMessagesTx(0)
    , fMessagesRx(0)
{
    stringstream id;
    id << type << "." << num;
    fId = id.str();

    if (numIoThreads > 1)
    {
        LOG(INFO) << "number of I/O threads is not used in nanomsg";
    }

    if (type == "router" || type == "dealer")
    {
        // Additional info about using the sockets ROUTER and DEALER with nanomsg can be found in:
        // http://250bpm.com/blog:14
        // http://www.freelists.org/post/nanomsg/a-stupid-load-balancing-question,1
        fSocket = nn_socket(AF_SP_RAW, GetConstant(type));
    }
    else
    {
        fSocket = nn_socket(AF_SP, GetConstant(type));
        if (type == "sub")
        {
            nn_setsockopt(fSocket, NN_SUB, NN_SUB_SUBSCRIBE, NULL, 0);
        }
    }


    LOG(INFO) << "created socket #" << fId;
}

string FairMQSocketNN::GetId()
{
    return fId;
}

void FairMQSocketNN::Bind(const string& address)
{
    LOG(INFO) << "bind socket #" << fId << " on " << address;

    int eid = nn_bind(fSocket, address.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << "failed binding socket #" << fId << ", reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::Connect(const string& address)
{
    LOG(INFO) << "connect socket #" << fId << " to " << address;

    int eid = nn_connect(fSocket, address.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << "failed connecting socket #" << fId << ", reason: " << nn_strerror(errno);
    }
}

int FairMQSocketNN::Send(FairMQMessage* msg, const string& flag)
{
    void* ptr = msg->GetMessage();
    int rc = nn_send(fSocket, &ptr, NN_MSG, 0);
    if (rc < 0)
    {
        LOG(ERROR) << "failed sending on socket #" << fId << ", reason: " << nn_strerror(errno);
    }
    else
    {
        fBytesTx += rc;
        ++fMessagesTx;
        static_cast<FairMQMessageNN*>(msg)->fReceiving = false;
    }

    return rc;
}

int FairMQSocketNN::Receive(FairMQMessage* msg, const string& flag)
{
    void* ptr = NULL;
    int rc = nn_recv(fSocket, &ptr, NN_MSG, 0);
    if (rc < 0)
    {
        LOG(ERROR) << "failed receiving on socket #" << fId << ", reason: " << nn_strerror(errno);
    }
    else
    {
        fBytesRx += rc;
        ++fMessagesRx;
        msg->SetMessage(ptr, rc);
        static_cast<FairMQMessageNN*>(msg)->fReceiving = true;
    }

    return rc;
}

void FairMQSocketNN::Close()
{
    nn_close(fSocket);
}

void FairMQSocketNN::Terminate()
{
    nn_term();
}

void* FairMQSocketNN::GetSocket()
{
    return NULL; // dummy method to comply with the interface. functionality not possible in zeromq.
}

int FairMQSocketNN::GetSocket(int nothing)
{
    return fSocket;
}

void FairMQSocketNN::SetOption(const string& option, const void* value, size_t valueSize)
{
    int rc = nn_setsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0)
    {
        LOG(ERROR) << "failed setting socket option, reason: " << nn_strerror(errno);
    }
}

void FairMQSocketNN::GetOption(const string& option, void* value, size_t* valueSize)
{
    int rc = nn_getsockopt(fSocket, NN_SOL_SOCKET, GetConstant(option), value, valueSize);
    if (rc < 0) {
        LOG(ERROR) << "failed getting socket option, reason: " << nn_strerror(errno);
    }
}

unsigned long FairMQSocketNN::GetBytesTx()
{
    return fBytesTx;
}

unsigned long FairMQSocketNN::GetBytesRx()
{
    return fBytesRx;
}

unsigned long FairMQSocketNN::GetMessagesTx()
{
    return fMessagesTx;
}

unsigned long FairMQSocketNN::GetMessagesRx()
{
    return fMessagesRx;
}

int FairMQSocketNN::GetConstant(const string& constant)
{
    if (constant == "")
        return 0;
    if (constant == "sub")
        return NN_SUB;
    if (constant == "pub")
        return NN_PUB;
    if (constant == "xsub")
        return NN_SUB;
    if (constant == "xpub")
        return NN_PUB;
    if (constant == "push")
        return NN_PUSH;
    if (constant == "pull")
        return NN_PULL;
    if (constant == "req")
        return NN_REQ;
    if (constant == "rep")
        return NN_REP;

    if (constant == "dealer")
        return NN_REQ;
    if (constant == "router")
        return NN_REP;

    if (constant == "pair")
        return NN_PAIR;

    if (constant == "snd-hwm")
        return NN_SNDBUF;
    if (constant == "rcv-hwm")
        return NN_RCVBUF;

    if (constant == "snd-more") {
        LOG(ERROR) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }
    if (constant == "rcv-more") {
        LOG(ERROR) << "Multipart messages functionality currently not supported by nanomsg!";
        return -1;
    }

    if (constant == "linger")
        return NN_LINGER;
    if (constant == "no-block")
        return NN_DONTWAIT;

    return -1;
}

FairMQSocketNN::~FairMQSocketNN()
{
    Close();
}
