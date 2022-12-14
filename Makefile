CC = g++ -g

default:
	@$(CC) consumer.cpp -o consumer
	@$(CC) producer.cpp -o producer
	
clean:
	@ rm -f consumer producer

veryClean:
	@$(CC) clean.cpp -o clean
	@ ./clean
	@ rm -f  consumer producer clean
