
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

  zmq::context_t ctx (1);

cout << argc;

	if(argc < 4) {
		std::cout << "Usage: chatClient -r -m <username> or chatClient -s -m <username> " << std::endl;
		return 0;
	}

	user_name += string(argv[3]);
	if (string(argv[1]) == "-r") {
		std::string chat_client = "tcp://127.0.0.1:9996";

		std::cout << "Starting the client to receive chats" << std::endl;
		zmq::socket_t subscriber (ctx, ZMQ_SUB);
		subscriber.connect(chat_client.c_str());

		subscriber.setsockopt(ZMQ_SUBSCRIBE, NULL, 0);

		while (1) {
				zmq::message_t update;
				subscriber.recv(&update);
				zMQChatBuf chat_rcv;
				chat_rcv.ParseFromArray(update.data(), update.size());
				std::cout << chat_rcv.time().c_str() << ":" <<chat_rcv.clientinfo().c_str() << ": " <<
				chat_rcv.chatstring().c_str() << std::endl;

#if 0
    // use protocol buffer's io stuff to package up a bunch of stuff into 
    // one stream of serialized messages. the first part of the stream 
    // is the number of messages. next, each message is preceeded by its
    // size in bytes.
    ZeroCopyInputStream* raw_input = 
      new ArrayInputStream(update.data(), update.size());
    CodedInputStream* coded_input = new CodedInputStream(raw_input);

    // find out how many updates are in this message
    uint32_t num_updates;
    coded_input->ReadLittleEndian32(&num_updates);
    std::cout << "received update " << std::endl;
	
    // now for each message in the stream, find the size and parse it ...
    for(int i = 0; i < num_updates; i++) {

#ifdef DEBUG
      std::cout << "\titem: " << i << std::endl;
#endif
      std::string serialized_update;
      uint32_t serialized_size;
      zMQChatBut chat;
      
      // the message size
      coded_input->ReadVarint32(&serialized_size);
      // the serialized message data
      coded_input->ReadString(&serialized_update, serialized_size);
      
      // parse it
      chat.ParseFromString(serialized_update);
      std::cout << chat.get_time() << \t: " 
				<< chat.get_clientInfo()
				<< chat.get_chatString() << std::endl;
    }

    delete coded_input;
    delete raw_input;
#endif
		};
	//zmq_close(&subscriber);
	} else {
		std::string chat_client = "tcp://127.0.0.1:9997";
		std::cout << "Starting the client to send chats" << std::endl;
		zmq::socket_t requestor(ctx, ZMQ_XREQ);
		requestor.connect(chat_client.c_str());

		while (1) {
		//user to input the message. Add username
		char text[1024];
		fgets (text, sizeof (text), stdin);

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

cout << "sent msg" << endl;
		// Get the reply
		//zmq::message_t reply;
        //requestor.recv (&reply);
cout << "received a reply" << endl;

		};
			//zmq_close(&requestor);
	}
	zmq_term(&ctx);
}
