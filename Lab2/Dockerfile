FROM alpine:latest
RUN apk update && apk add --no-cache coreutils util-linux
COPY concurrent_file_manager.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/concurrent_file_manager.sh
CMD ["/usr/local/bin/concurrent_file_manager.sh"]
