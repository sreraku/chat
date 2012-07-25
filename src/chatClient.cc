
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

#include "../protobuf/zmgchatbuf.pb.h"

using namespace google::protobuf::io;
using namespace std;
using namespace chat;
using namespace boost;

#define MAX_USER_NAME 1024
int main (int argc, char **argv)
{
  zMQChatBuf chat;
  
	std::string user_name;
	std::string chat_client;

  zmq::context_t ctx (1);

	if(argc < 6 || (string(argv[1]) != "-r" && string(argv[1]) != "-s")) {
		std::cout << "Usage: chatClient -r -u <username> -i <server_listen_ipaddress:port> or chatClient -s -u <username> -i <server_ipaddress:port> " << std::endl;
		return 0;
	}

	for (int i = 1; i < argc; i++) { 
		/* We will iterate over argv[] to get the parameters stored inside.
		 * Note that we're starting on 1 because we don't need to know the 
                 * path of the program, which is stored in argv[0] */
                if (string(argv[i]) == "-r") {
			// no need to do anything
			continue;
                } else if (string(argv[i]) == "-u") {
			user_name += string(argv[i+1]);
                } else if (string(argv[i]) == "-i") {
			chat_client += string(argv[i+1]);
		}
        }

	if (string(argv[1]) == "-r") {

		std::cout << "Starting the client to receive chats" << std::endl;
		zmq::socket_t subscriber (ctx, ZMQ_SUB);
		subscriber.connect(chat_client.c_str());

		subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

		while (1) {
				zmq::message_t update;
				subscriber.recv(&update);
				zMQChatBuf chat_rcv;
				chat_rcv.ParseFromArray(update.data(), update.size());
				std::string output;
				output = chat_rcv.time() + ":" + chat_rcv.clientinfo() + ":" + chat_rcv.chatstring();
				std::replace (output.begin(), output.end(), '\n', ' ');
				std::cout << output.c_str() << endl;

		};
		zmq_close(&subscriber);
	} else {
		std::cout << "Starting the client to send chats" << std::endl;
		zmq::socket_t requestor(ctx, ZMQ_XREQ);
		requestor.connect(chat_client.c_str());

		while (1) {
		//user to input the message. Add username
		char text[1024];

		std::cin.getline(text, sizeof(text), '\n');

		zMQChatBuf chat_req;
		chat_req.set_chatstring(text);
		chat_req.set_clientinfo(user_name);

		std::string chat_serialize;
		chat_req.SerializeToString(&chat_serialize);

		//  create the req
		zmq::message_t req (chat_serialize.size());
		memcpy ((void *) req.data (), chat_serialize.c_str(), 
				chat_serialize.size());
		requestor.send (req);
		};
		zmq_close(&requestor);
	}
	zmq_term(&ctx);
}
