.DEFAULT_GOAL := all


#rules

tcpserver: 
	@$(MAKE) -C server -f server.mk server

tcpclient:
	@$(MAKE) -C client -f client.mk client

cleanall: clean
	$(RM) cscope.out tags

clean:
	@$(MAKE) -C server -f server.mk clean
	@$(MAKE) -C client -f client.mk clean
	$(RM) -r obj
	$(RM) *.json
	@echo ""
	@echo "Cleaned all except tags and cscope.out! Type 'make cleanall' to remove them!"

server: tcpserver
	$(ECHO) 

client: tcpclient
	$(ECHO) 


all: tcpserver tcpclient

