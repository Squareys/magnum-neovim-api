/*
    This file is part of MagnumNeoVimApi.

    Copyright © 2018 Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#ifndef Corrade_Net_Socket_h
#define Corrade_Net_Socket_h

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayView.h>

/** @file Contains class @ref Corrade::Net::Socket */

namespace Corrade { namespace Net {

class Socket {
public:
    /**
     * @brief Constructor
     * @param host Host of the server
     * @param port Port to connect to
     *
     * Try connecting to a server. If successful, @ref connected()
     * will return true.
     */
    Socket(const std::string& host, int port);
    /**
     * @brief Destructor
     *
     * Disconnects.
     */
    ~Socket();

    /** @brief Copying is not allowed */
    Socket(const Socket&)=delete;

    /** @brief Copying-assignment is not allowed */
    Socket& operator=(const Socket&)=delete;

    /** @brief Whether this socket was able to connect */
    bool connected() const {
        return _connected;
    }

    /** @brief Blocking method to send data */
    void send(Containers::ArrayView<char> data);

    /**
     * @brief Blocking method to receive data
     * @param buffer Buffer to write to.
     * @return valid range in `buffer`.
     *
     * Returns either an ArrayView of size `0`, which indicates
     * a closed connection, or a prefix of `buffer` with the received
     * data.
     */
    Containers::ArrayView<char> receive(Containers::Array<char>& buffer);

private:
    struct SocketData;

    void close();

    bool _connected = false;
    SocketData* _data;
};

}}

#endif
