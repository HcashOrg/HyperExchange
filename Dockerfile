FROM ubuntu:16.04

RUN apt-get update -y
RUN apt-get install -y vim gcc ninja autoconf cmake make automake libtool git libboost-all-dev libssl-dev g++ libcurl4-openssl-dev libleveldb-dev libreadline-dev
RUN mkdir -p /code_deps
WORKDIR /code_deps
RUN git clone https://github.com/BlockLink/blocklink_crosschain_privatekey
WORKDIR /code_deps/blocklink_crosschain_privatekey
RUN cmake -DCMAKE_BUILD_TYPE=Release .
RUN make
WORKDIR /code_deps
RUN git clone https://github.com/BlockLink/eth_crosschain_privatekey.git
WORKDIR /code_deps/eth_crosschain_privatekey/eth_sign/cryptopp/
RUN make
WORKDIR /code_deps/eth_crosschain_privatekey/eth_sign
RUN cmake .
RUN make
ENV CROSSCHAIN_PRIVATEKEY_PROJECT=/code_deps/blocklink_crosschain_privatekey
ENV ETH_CROSSCHAIN_PROJECT=/code_deps/eth_crosschain_privatekey

WORKDIR /code



