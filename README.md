# YAAW - Yet Another Asio Wrapper

DISCLAIMER: I developed YAAW for educational purposes and it is not meant to be used for commercial use.

I think every formidable C++ developer has written some sort of TCP socket library, so here is mine.

**To get started using CMAKE**
- Add the include directory to your include directory
- Don't forget to define ASIO_STANDALONE on your compiler

To use the library you will need to inherit tcp_client or tcp_server and override some protected methods to handle things like new connections and packets.

Once you have the socket set up, all thats left is to run the socket.
```CPP
  ServerSocket server(9991);
  server.run();
```
```CPP
  ClientSocket client;
  client.connect("127.0.0.1", 9991);
```

All thats left is to send a packet, you can do that by calling the sockets send method with a packet_data object.
```CPP
  auto data = std::make_shared<sl::packet_data>(bytes, size, endpoint);
  send(data);
```
Not the neatest solution, but a fun exercise.
