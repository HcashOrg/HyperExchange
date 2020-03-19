BitShares OS X Build Instructions
===============================

1. Install XCode and its command line tools by following the instructions here: https://guide.macports.org/#installing.xcode. 
   In OS X 10.11 (El Capitan) and newer, you will be prompted to install developer tools when running a devloper command in the terminal. This step may not be needed.


2. Install Homebrew by following the instructions here: http://brew.sh/

3. Initialize Homebrew:
   ```
   brew doctor
   brew update
   ```

4. Install dependencies:
   ```
   brew install boost cmake git openssl autoconf automake libtool
   brew link --force openssl 

   ```
      If openssl cannot be linked ,try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR

5. *Optional.* To support importing Bitcoin wallet files:
   ```
   brew install berkeley-db
   ```

6. *Optional.* To use TCMalloc in LevelDB:
   ```
   brew install google-perftools
   ```

7. Clone the HyperExchange repository:
   ```
   git clone https://github.com/HcashOrg/HyperExchange.git
   ```
   cd HyperExchange

8. Build HyperExchange:
   ```
   git submodule update --init --recursive
   cmake .
   make
   ```