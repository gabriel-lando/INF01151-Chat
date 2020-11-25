FROM ubuntu:16.04
RUN apt-get update
RUN apt-get -yq dist-upgrade

# Install Dependencies
RUN apt-get -yq install build-essential git net-tools

# Clone Sisop 2 - Chat repository
# RUN git clone https://github.com/TMinuzzo/chat.git /chat
# Copy files to Docker image
ADD ./Makefile /chat/Makefile
ADD ./src/* /chat/src/

# Build Chat
RUN cd /chat && make rebuild

# Open container on build dir
WORKDIR  /chat/bin

