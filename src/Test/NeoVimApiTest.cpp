/*
    This file is part of MagnumNeovimApi.

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

#include <sstream>

#include <Corrade/TestSuite/Tester.h>
#include "Corrade/Net/Socket.h"
#include "mpack/mpack.h"

#include "Corrade/Net/Socket.h"

#include "neovimapi2.h"

namespace NeovimApi {

using namespace Corrade;
using namespace Magnum;

struct Test: TestSuite::Tester {
    explicit Test();

    void test();

    void testGenerated();
};

Test::Test() {
    addTests({
        &Test::test,
        &Test::testGenerated
    });
}

void Test::test() {
    // ./nvim.exe --headless -c "call serverstart('127.0.0.1:6666')"

    char* buffer = nullptr;
    size_t size = 0;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &buffer, &size);

    mpack_start_array(&writer, 4);

    const int type = 0;
    const int msgid = 42;

    mpack_write_i32(&writer, type);
    mpack_write_i32(&writer, msgid);
    mpack_write_cstr(&writer, "nvim_eval");

    /* params */
    mpack_start_array(&writer, 1);

    mpack_write_cstr(&writer, "'hello' . 'world!\n'");
    mpack_finish_array(&writer);

    mpack_finish_array(&writer);

    CORRADE_VERIFY(mpack_writer_destroy(&writer) == mpack_ok);
    CORRADE_VERIFY(size != 0);

    /* Open connection to nvim */
    Net::Socket client("127.0.0.1", 6666);

    client.send(Containers::arrayView(buffer, size));

    Containers::Array<char> receiveBuffer{Containers::DefaultInit, 256};
    Containers::ArrayView<char> response = client.receive(receiveBuffer);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, response.data(), response.size());
    mpack_expect_array_max(&reader, 4);

    CORRADE_COMPARE(mpack_expect_i32(&reader), 1);
    CORRADE_COMPARE(mpack_expect_u32(&reader), msgid);
    mpack_type_t errorTag = mpack_read_tag(&reader).type;
    CORRADE_COMPARE(errorTag, mpack_type_nil);

    const mpack_tag_t result = mpack_peek_tag(&reader);
    char strBuf[512];
    mpack_expect_cstr(&reader, strBuf, sizeof(strBuf));
    CORRADE_COMPARE("helloworld!\n", std::string(strBuf));

    mpack_done_array(&reader);
    CORRADE_VERIFY(mpack_reader_destroy(&reader) == mpack_ok);
}

void Test::testGenerated() {
    NeovimApi::NeovimApi2 nvim{6666};
    Object o = nvim.nvim_eval("'hello' . 'world!\n'");
    CORRADE_COMPARE("helloworld!\n", o.s);

    nvim.nvim_ui_attach(25, 25, {});

    const Notification notification = nvim.waitForNotification();
    CORRADE_VERIFY(notification.method != NotificationType::Timeout);
}

}

CORRADE_TEST_MAIN(NeovimApi::Test)
