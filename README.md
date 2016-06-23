# ReverseShell

How to use it is shown in 
https://www.youtube.com/watch?v=hy_5hHccGq4

The Ip to connect to is hardcoded in RemoteShellClient.cpp line 48 : server.sin_addr.s_addr = inet_addr("127.0.0.1");
Just change it to wherever you want , I tested it in different machines and it works fine .
