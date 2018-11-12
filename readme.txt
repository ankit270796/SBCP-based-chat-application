TCP Simple Broadcast Chat Server and Client
:

In this programming assignment SBCP(Simple Mail Broadcast Protocol) is used to communicate between server and client. SBCP defines a packet structure
which can be used for different purposes: joining chat session, sending message, making a query to the server, like the list of online clients, type 
of message, username.

Whenever a client has to join the chat session, it sends a join request to the server. Server looks into the details of all the present clients. If a client with the same username is present in the system, the server denies the connection and sends a NACK. If the username is not present already server accepts the join request and user is allowed to participate in the chat session. Server sends the list of clienst present in the system. Clients have the capability to check if a user is online. When the client see that an another client has gone offline, it notifies to the server. Upon receiving this information from the client server deletes the record of that user, frees up the username. This makes the usernames re-usuable. Client uses I/O 
multiplexing to handle simultaneous send and receive.
Server works as the central agent, it forwards the message received from a client to everyone except the client who sent the message. Server only accepts the request from the unknown users. When a user makes a request to join, server checks if a client with the same username is already present. If not server adds it to the list of clients and sends it to everyone. If a user exits the chat session unceremoniously, the server detects that and removes that client from the list. Server uses the I/O multiplexing to hear from the listening ports and connected clients simultaneously.  


