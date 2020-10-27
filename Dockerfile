FROM ubuntu:16.04
RUN apt-get update
RUN apt-get -yq dist-upgrade

# Install Dependencies
RUN apt-get -yq install build-essential git

# Clone Sisop 2 - Chat repository
RUN git clone https://github.com/TMinuzzo/chat.git /chat

# Build Chat
RUN cd /chat && make rebuild

# Open container on build dir
WORKDIR  /chat/bin

