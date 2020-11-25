# chat

---------------
## How to Build
---------------

## Option 1: Run in a Docker container
### Install Docker
```bash
# Update apt lists:
sudo apt update

# Install dependencies:
sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common

# Add Docker GPG Key:
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

# Add repository key:
sudo apt-key fingerprint 0EBFCD88

# Add Docker repository
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

# Install Docker
sudo apt-get update

sudo apt-get install docker-ce docker-ce-cli containerd.io

sudo usermod -a -G docker $USER
```

### Create a Docker image
```bash
sudo docker build . -t "chat"
```

- In different consoles, run:

```bash
sudo docker run --net="host" --name client -ti chat
```

```bash
sudo docker run --net="host" --name front -ti chat
```

```bash
sudo docker run --net="host" --name server -ti chat
```

```bash
sudo docker run --net="host" --name backup1 -ti chat
```

```bash
sudo docker run --net="host" --name backup2 -ti chat
```

```bash
sudo docker run --net="host" --name backup3 -ti chat
```

## Option 2: Compile and run directly on host:
```bash
make
```
- The executables will be generated inside folder `bin`

---------------
## How to Run
---------------
### Server
```bash
./server <port> <N_msgs>
```

### Backups
```bash
./server <server_port> <N_msgs> <bkp_port>
```

### Front-end
```bash
./front <front_port> <server_port>
```

### Client
```bash
./client <username> <groupname> <front_ip> <front_port>
```
