# YAAW - Yet Another Asio Wrapper

I think every formidable C++ developer has written some sort of TCP socket library, so here is mine.

To get started - 
  Simply grab the include directory and you're good to go.

To start recieving messages - 
  Inherit from TcpClient or TcpServer and override the on_message_recieve method. 
  YAAW doesn't keep a buffer of recieved messages - so you will need to ensure that you're not blocking that read thread.

YAAW is not yet complete and (if it's not obvious) should not even be considered for commercial use.
