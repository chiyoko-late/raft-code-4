

CC = g++ -g

All: leader followers client

# leader: arpc_leader.cpp appendentries.h
# 	$(CC)  arpc_leader.cpp -o leader

# followers: arpc_followers.cpp appendentries.h
# 	$(CC) arpc_followers.cpp -o followers

client: client.cpp appendentries.h
	$(CC)  client.cpp -o client

leader: appendentriesRPC_leader.cpp appendentries.h
	$(CC)  appendentriesRPC_leader.cpp -o leader

followers: appendentriesRPC_followers.cpp appendentries.h
	$(CC) appendentriesRPC_followers.cpp -o followers

clean:
	rm -f leader followers client

# $(CC) appendentriesRPC_leader.o appendentriesRPC_followers.o -o append