# Lab10

## Build Instrctions:

mkdir build
cd build
cmake ..
make

Will generate server.out and client.out


## Source Layout:
./shared/: Things that will be shared between them. So far only info is present and that will contain the server/port information.  

./client/: All code relating to the client.  

./server/: All code relating to the server.  
