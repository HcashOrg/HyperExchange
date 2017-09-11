# Ubuntu 14.04 LTS Build and Install Instructions
The following dependencies were necessary for a clean install of Ubuntu 14.04 LTS:

    sudo apt-get install cmake make libbz2-dev libdb++-dev libdb-dev libssl-dev openssl libreadline-dev autoconf libtool git ntp

## Build Boost 1.57.0 

The Boost which ships with Ubuntu 14.04 is too old.  You need to download the Boost tarball for Boost 1.57.0
(Note, 1.58.0 requires C++14 and will not build on Ubuntu 14.04 LTS; this requirement was an accident, see [this mailing list post](http://boost.2283326.n4.nabble.com/1-58-1-bugfix-release-necessary-td4674686.html)).

    BOOST_ROOT=$HOME/opt/boost_1_57_0
    sudo apt-get update
    sudo apt-get install autotools-dev build-essential g++ libbz2-dev libicu-dev python-dev
    wget -c 'http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.bz2/download' -O boost_1_57_0.tar.bz2
    [ $( sha256sum boost_1_57_0.tar.bz2 | cut -d ' ' -f 1 ) == "910c8c022a33ccec7f088bd65d4f14b466588dda94ba2124e78b8c57db264967" ] || ( echo 'Corrupt download' ; exit 1 )
    tar xjf boost_1_57_0.tar.bz2
    cd boost_1_57_0/
    ./bootstrap.sh "--prefix=$BOOST_ROOT"
    ./b2 install


## Build BitShares Core

    cd ..
    git clone https://github.com/bitshares/bitshares-core.git
    cd bitshares-core
    git submodule update --init --recursive
    cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release .
    make 


# Ubuntu 16.04 LTS

Ubuntu 16.04 LTS ships with Boost 1.58 libraries, so no need to build from source.

    sudo apt install libboost-all-dev

Other steps are same to 14.04 LTS.
