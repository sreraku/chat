#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <boost/bind.hpp>
#include <boost/random.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <hiredis/hiredis.h>
#ifdef __cplusplus
}
#endif

#include "../protobuf/zmgchatbuf.pb.h"
//#include "../hiredispp/hiredispp.h"

using namespace google::protobuf::io;
using namespace std;
using namespace chat;
using namespace boost;
//using namespace hiredispp;

#ifdef __cplusplus
extern "C" {
#endif
redisContext *redisConnect(const char *ip, int port);
void *redisCommand(redisContext *c, const char *format, ...);
void freeReplyObject(void *reply);
#ifdef __cplusplus
}
#endif
//publisher to push the messages out
void *publisher (void *arg)
{
    zmq::context_t *ctx = (zmq::context_t*) arg;

    //  publisher thread has a PUB socket to push out data
    zmq::socket_t publish (*ctx, ZMQ_PUB);
    publish.bind("tcp://*:9996");

    zmq::socket_t s_pub (*ctx, ZMQ_REP);

    //  Connect to local.
    s_pub.connect ("inproc://chatty");

    while (1) {
        zmq::message_t request;
        s_pub.recv (&request);
		zMQChatBuf chat_rcv;

		chat_rcv.ParseFromArray(request.data(), request.size());
		std::cout << "pub: Received " << chat_rcv.clientinfo().c_str() << ": " <<
				chat_rcv.chatstring().c_str() << std::endl;
#if 0
		zMQChatBuf chat_pub;

		chat_pub.set_chatString(chat_rcv.chatString());
		chat_pub.set_clientInfo(chat_rcv.clientInfo());
		chat_pub.set_status(status);
		chat_pub.set_time(time);

		std::string chat_serialize;
		chat_pub.SerializeToString(&chat_serialize);

		//  create the msg
		zmq::message_t msg (chat_serialize.size());
		memcpy ((void *) msg.data (), chat_serialize.c_str(), 
				chat_serialize.size());
#endif
		zmq::message_t local_reply;
		s_pub.send(local_reply);

        publish.send(request);
		cout << "Published" << endl;

    }
	//zmq_close(&s_pub);
	//zmq_close(&publish);
}

#define CHAT_TIME_MAX_CHARS 256
int main (int argc,  char **argv) 
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	int status = 0;
	int *join_status = NULL;
	redisReply *redis_reply;
        pthread_t pub;

    //  Prepare our context and publisher
    zmq::context_t ctx (1);

    //  create inproc to send data over to pub
    zmq::socket_t local (ctx, ZMQ_REQ);
    local.bind ("inproc://chatty");

	redisContext *c = redisConnect("127.0.0.1", 6379);
	if (c->err) {
		std::cout << "Unable to connect to redis, bailing out" << std::endl;
		return 0;
	}
    //  create 2 threads. Main thread will be used to get client requests
    for (int i = 0; i < 1; i++) {
        int rc = pthread_create (&pub, NULL, publisher, (void*) &ctx);
        assert (rc == 0);
    }

    zmq::socket_t s (ctx, ZMQ_XREP);
    s.bind("tcp://*:9997");

	//hiredispp::Redis r("localhost");
    while (1) {
		zmq::message_t request;
		zMQChatBuf chat_rcv;

		// receive the request, and parse the protocol buffer from it
		s.recv (&request);

		chat_rcv.ParseFromArray(request.data(), request.size());
#ifdef DEBUG
		std::cout << "server: Received " << chat_rcv.clientinfo().c_str() << ": " <<
				chat_rcv.chatstring().c_str() << std::endl;
#endif
		//  Get the current time. Replace the newline character at the end
		//  by space character.
		std::string timeBuf;
		time_t current_time;
		time (&current_time);
		timeBuf += ctime (&current_time);

		//boost::int64_t i = r.incr("counter");
		redis_reply = (redisReply *)redisCommand(c, "INCR counter");
		if (redis_reply->type == REDIS_REPLY_ERROR) {
				std::cout << "Bailing out for redis Error: " << redis_reply->str << std::endl;
				freeReplyObject(redis_reply);
				return 1;
		} 

		boost::int64_t i = redis_reply->integer;
		freeReplyObject(redis_reply);

		std::string user_name;
		std::string text (user_name);
		std::string k = boost::lexical_cast<std::string>(i);
		text = "job_id :" + k + " " + chat_rcv.chatstring() + ": " + timeBuf;
#if 0
		std::string key;
		key = chat_rcv.clientinfo() + k;
		try {
				r.set(chat_rcv.clientinfo(), text);
		} catch (const hiredispp::RedisException& e) {
				cerr << e.what();
				status = -1;
		}
#endif

		//Write to Redis
		redis_reply = (redisReply *)redisCommand(c, "RPUSH %s %s", chat_rcv.clientinfo().c_str(), text.c_str());
		if (redis_reply->type == REDIS_REPLY_ERROR) {
				std::cout << "Bailing out for redis Error: " << redis_reply->str << std::endl;
				freeReplyObject(redis_reply);
				return 1;
		} 
		freeReplyObject(redis_reply);

		//  create the msg for pub
		zMQChatBuf chat_pub;

		chat_pub.set_chatstring(chat_rcv.chatstring());
		chat_pub.set_clientinfo(chat_rcv.clientinfo());
		chat_pub.set_status(status);
		chat_pub.set_time(timeBuf);

		std::string chat_serialize;
		chat_pub.SerializeToString(&chat_serialize);

		//  create the msg
		zmq::message_t msg (chat_serialize.size());
		memcpy ((void *) msg.data (), chat_serialize.c_str(), 
				chat_serialize.size());
		local.send (msg);

		std::cout << "server: send pub " << endl;
		zmq::message_t local_get;
		local.recv(&local_get);
		std::cout << "server: Dpme pub " << endl;

		// create a response. with status.
		zMQChatBuf chat_resp;
		chat_resp.set_chatstring(chat_rcv.chatstring());
		chat_resp.set_clientinfo(chat_rcv.clientinfo());
		chat_resp.set_status(status);

		//  create the reply
		zmq::message_t reply (chat_serialize.size());
		memcpy ((void *) reply.data (), chat_serialize.c_str(), 
				chat_serialize.size());
		s.send (reply);
		std::cout << "server: Dpme sending " << endl;
    }
	//zmq_close(&local);
	//zmq_close(&s);
	zmq_term(&ctx);
	pthread_join(pub, (void **)&join_status);
    return 0;
}
