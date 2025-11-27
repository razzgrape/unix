FROM alpine:latest
RUN apk update && apk add --no-cache coreutils util-linux
COPY script.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/script.sh
CMD ["/usr/local/bin/script.sh"]
