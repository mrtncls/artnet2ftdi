FROM alpine:latest AS build
COPY . /src
RUN apk update && apk add --no-cache \
    gcc \
    libc-dev \
    pkgconfig \
    libftdi1-dev
RUN cd /src && gcc -o artnet2ftdi artnet2ftdi.c $(pkg-config --cflags --libs libftdi1) -Wall

FROM alpine:latest AS runtime
COPY --from=build /src/artnet2ftdi /bin/artnet2ftdi
RUN apk update
RUN apk add libftdi1
EXPOSE 6454/udp
CMD ["/bin/artnet2ftdi","6454"]
#ENTRYPOINT ["/bin/artnet2ftdi"]
