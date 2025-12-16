#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 6666
#define BUFFER_SIZE 256

volatile sig_atomic_t wasSigHup = 0;
int accepted_client_fd = -1;

void sigHupHandler(int r) {
    wasSigHup = 1; 
}

int setup_server_socket() {
    int server_fd;
    struct sockaddr_in address; 
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("Listening on port %d. FD %d\n", PORT, server_fd);
    return server_fd;
}

void register_signal_handler() {
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = 0; 
    sigaction(SIGHUP, &sa, NULL);
    printf("SIGHUP handler registered.\n");
}

void block_signal(sigset_t *origMask) {
    sigset_t blockedMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, origMask);
    printf("SIGHUP blocked. Ready to run pselect().\n");
}

void main_loop(int server_fd, const sigset_t *origSigMask) {
    int max_fd = server_fd;
    
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        
        if (accepted_client_fd != -1) {
            FD_SET(accepted_client_fd, &fds); 

            if (server_fd > accepted_client_fd) {
                max_fd = server_fd;
            } 
            else {
                max_fd = accepted_client_fd;
            }
        } 
        else {
            max_fd = server_fd;
        }

        int activity = pselect(max_fd + 1, &fds, NULL, NULL, NULL, origSigMask);

        if (activity == -1) {
            if (errno == EINTR) { 
                if (wasSigHup) {
                    wasSigHup = 0;
                    printf("SIGHUP received\n");
                }
            } 
            else {
                perror("pselect");
                break;
            }
        }
        
        if (FD_ISSET(server_fd, &fds)) {
            struct sockaddr_in client_address;
            socklen_t addrlen = sizeof(client_address);
            
            int new_socket = accept(server_fd, (struct sockaddr *)&client_address, &addrlen);
            
            printf("New connection on FD %d.\n", new_socket);
            
            if (accepted_client_fd == -1) {
                accepted_client_fd = new_socket;
                printf("Connection accepted.\n");
            } 
            else {
                printf("Connection rejected. Closing FD %d.\n", new_socket);
                close(new_socket);
            }
        }
        
        if (accepted_client_fd != -1 && FD_ISSET(accepted_client_fd, &fds)) {
            char buffer[BUFFER_SIZE];
            ssize_t valread = read(accepted_client_fd, buffer, BUFFER_SIZE - 1);
            
            if (valread > 0) {
                printf("Data received from FD %d. Bytes: %zd\n", accepted_client_fd, valread);
            } 
            else {
                printf("Client disconnected. Closing FD %d.\n", accepted_client_fd);
                close(accepted_client_fd);
                accepted_client_fd = -1;
            }
        }
    }
}

int main() {
    sigset_t origSigMask;

    int server_fd = setup_server_socket();

    register_signal_handler();

    block_signal(&origSigMask);

    main_loop(server_fd, &origSigMask);

    if (accepted_client_fd != -1) close(accepted_client_fd);
    close(server_fd);

    sigprocmask(SIG_SETMASK, &origSigMask, NULL);

    return 0;
}