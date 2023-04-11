

CC = gcc -g

All: leader followers client

# leader: arpc_leader.cpp appendentries.h
# 	$(CC)  arpc_leader.cpp -o leader

# followers: arpc_followers.cpp appendentries.h
# 	$(CC) arpc_followers.cpp -o followers

client: client.c appendentries.h
	$(CC)  client.c -o client

leader: appendentriesRPC_leader.c appendentries.h
	$(CC)  appendentriesRPC_leader.c -o leader

followers: appendentriesRPC_followers.c appendentries.h
	$(CC) appendentriesRPC_followers.c -o followers

clean:
	rm -f leader followers client

# $(CC) appendentriesRPC_leader.o appendentriesRPC_followers.o -o append