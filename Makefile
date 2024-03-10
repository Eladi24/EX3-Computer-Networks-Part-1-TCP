# Use the gcc compiler.
CC = gcc

# Flags for the compiler.
CFLAGS = -g -Wall -Wextra -Werror -std=c99 -pedantic

# Command to remove files.
RM = rm -f

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean run_tcp_server runts runtr #runuc runus

# Default target - compile everything and create the executables and libraries.
all: TCP_Receiver TCP_Sender

# Alias for the default target.
default: all

TCP: TCP_Receiver TCP_Sender


############
# Programs #
############

# Compile the tcp server.
TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) -o $@ $^

# Compile the tcp client.
TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) -o $@ $^


################
# Run programs #
################

# Run tcp Receiver in cubic.
runtRC: TCP_Receiver
	./TCP_Receiver -p 9999 -algo cubic

# Run tcp Receiver in reno.
runtRR: TCP_Receiver
	./TCP_Receiver -p 9999 -algo reno

# Run tcp Sender in cubic.
runtSC: TCP_Sender
	./TCP_Sender -ip 127.0.0.1 -p 9999 -algo cubic

# Run tcp Sender in reno.
runtSR: TCP_Sender
	./TCP_Sender -ip 127.0.0.1 -p 9999 -algo reno


################
# System Trace #
################

# Run the tcp server with system trace.
runtr_trace: TCP_Receiver
	strace ./TCP_Receiver

# Run the tcp client with system trace.
runts_trace: TCP_Sender
	strace ./TCP_Sender

################
# Object files #
################

# Compile all the C files into object files.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


#################
# Cleanup files #
#################

# Remove all the object files, shared libraries and executables.
clean:
	$(RM) *.o *.so TCP_Receiver TCP_Sender