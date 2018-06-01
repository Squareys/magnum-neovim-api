// Auto generated {{date}} from nvim API level:{{api_level}}
#include "neovimapi{{api_level}}.h"

#include <type_traits>

namespace NeovimApi {

using namespace Corrade;
using namespace Magnum;

const char** getNotificationTypeStrings();

void mpack_write(mpack_writer_t* writer, const Vector2i& val) {
    mpack_write_i32(writer, val.x());
    mpack_write_i32(writer, val.y());
}

void mpack_write(mpack_writer_t* writer, const Double val) {
    mpack_write_double(writer, val);
}

void mpack_write(mpack_writer_t* writer, const std::string& val) {
    mpack_write_cstr(writer, val.c_str());
}

void mpack_write(mpack_writer_t* writer, const Object& val) {
    if(val.type == mpack_type_str) {
        mpack_write(writer, val.s);
    } else if(val.type == mpack_type_bool) {
        mpack_write_bool(writer, val.b);
    } else if(val.type == mpack_type_int) {
        mpack_write(writer, val.l);
    } else {
        Warning() << "mpack_write(): unknown type" << Int(val.type);
    }
}

void mpack_write(mpack_writer_t* writer, const std::unordered_map<std::string, Object>& options) {
    mpack_start_map(writer, options.size());
    for(const auto& e : options) {
        mpack_write(writer, e.first);
        mpack_write(writer, e.second);
    }
    mpack_finish_map(writer);
}

template<typename T>
void mpack_write(mpack_writer_t* writer, Containers::ArrayView<T> data) {
    mpack_start_array(writer, data.size());
    for(const auto& e : data) {
        mpack_write(writer, e);
    }
    mpack_finish_array(writer);
}

Object mpack_read_object(mpack_reader_t* reader) {
    Object result{};
    const mpack_tag_t tag = mpack_peek_tag(reader);
    result.type = tag.type;

    if(tag.type == mpack_type_str) {
        char strBuf[512];
        mpack_expect_cstr(reader, strBuf, sizeof(strBuf));
        result.s = std::string(strBuf);
    } else if(tag.type == mpack_type_int) {
        result.l = mpack_expect_i64(reader);
    } else if(tag.type == mpack_type_bool) {
        result.b = mpack_expect_bool(reader);
    } else {
        result.type = mpack_type_nil;
        Warning() << "mpack_read_object(): unsupported type" << result.type;
    }

    return result;
}

template<typename T, typename... Args>
void mpack_write(mpack_writer_t* writer, T value, Args... args) {
    mpack_write(writer, value);
    mpack_write(writer, args...);
}

void mpack_write(mpack_writer_t* writer) {}

template<typename T> T mpack_read(mpack_reader_t* reader);

template<>
bool mpack_read<bool>(mpack_reader_t* reader) {
    return mpack_expect_bool(reader);
}
template<>
Long mpack_read<Long>(mpack_reader_t* reader) {
    return mpack_expect_i64(reader);
}
template<>
Double mpack_read<Double>(mpack_reader_t* reader) {
    return mpack_expect_double(reader);
}
template<>
std::string mpack_read<std::string>(mpack_reader_t* reader) {
    char strBuf[512];
    mpack_expect_cstr(reader, strBuf, sizeof(strBuf));
    return strBuf;
}
template<>
Object mpack_read<Object>(mpack_reader_t* reader) {
    return mpack_read_object(reader);
}
template<>
Vector2i mpack_read<Vector2i>(mpack_reader_t* reader) {
    const Int x = mpack_expect_i32(reader);
    const Int y = mpack_expect_i32(reader);
    return {x, y};
}
template<>
std::unordered_map<std::string, Object> mpack_read<std::unordered_map<std::string, Object>>(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_map(reader);

    std::unordered_map<std::string, Object> options{numElements};

    char buffer[512];
    for(int i = 0; i < numElements; ++i) {
        mpack_expect_cstr(reader, buffer, sizeof(buffer));
        options[std::string(buffer)] = mpack_read_object(reader);
    }

    mpack_done_map(reader);
    return options;
}

template<typename T>
Containers::Array<T> mpack_read_array(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_array(reader);

    Containers::Array<T> data{Containers::DefaultInit, numElements};
    for(int i = 0; i < numElements; ++i) {
        data[i] = mpack_read<T>(reader);
    }

    mpack_done_array(reader);
    return data;
}
template<>
Containers::Array<Long> mpack_read<Containers::Array<Long>>(mpack_reader_t* reader) {
    return mpack_read_array<Long>(reader);
}
template<>
Containers::Array<Object> mpack_read<Containers::Array<Object>>(mpack_reader_t* reader) {
    return mpack_read_array<Object>(reader);
}
template<>
Containers::Array<std::string> mpack_read<Containers::Array<std::string>>(mpack_reader_t* reader) {
    return mpack_read_array<std::string>(reader);
}

NeovimApi{{api_level}}::NeovimApi{{api_level}}(int port, int receiveBufferSize):
    NeovimApi{{api_level}}{"127.0.0.1", port, receiveBufferSize}
{
}

NeovimApi{{api_level}}::NeovimApi{{api_level}}(const std::string& host, int port, int receiveBufferSize):
    _socket{host, port},
    _receiveBuffer{Containers::DefaultInit, receiveBufferSize}
{
}

template<typename... Args>
Int NeovimApi{{api_level}}::dispatch(const std::string& func, Args... args) {
    char* data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    const int msgId = nextMessageId();
    mpack_start_array(&writer, 4);
    mpack_write_i32(&writer, 0);
    mpack_write_i32(&writer, msgId);
    mpack_write_cstr(&writer, func.c_str());

    mpack_start_array(&writer, sizeof...(Args));
    mpack_write(&writer, args...);

    mpack_finish_array(&writer);
    mpack_finish_array(&writer);
    CORRADE_ASSERT(mpack_writer_destroy(&writer) == mpack_ok, "NeovimApi{{api_level}}::dispatch(): Could not pack request", -1);

    _socket.send(Containers::arrayView(data, size));
    return msgId;
}

void NeovimApi{{api_level}}::handleNotification(mpack_reader_t& reader) {
    const NotificationType func = (NotificationType)mpack_expect_enum(&reader, getNotificationTypeStrings(), Int(NotificationType::Count));

    /* Get parameter binary data */
    const char* remainingBuffer;
    const size_t remainingCount = mpack_reader_remaining(&reader, &remainingBuffer);
    Notification n{func, Containers::Array<char>{Containers::NoInit, remainingCount}};
    std::copy_n(remainingBuffer, remainingCount, n.parameters.data());
    _notifications.push_back(std::move(n));
}

template<typename T>
auto NeovimApi{{api_level}}::waitForResponse(Int msgId, Int timeout) {
    do {
        Containers::ArrayView<char> response = _socket.receive(_receiveBuffer, timeout);
        // if(response.size() == 0 || response.data() == nullptr) ...
        if(response.size() == _receiveBuffer.size()) {
            Warning() << "NeovimApi{{api_level}}::waitForResponse(): Receive buffer was full";
            // TODO: Handle requests over multiple buffers
        }

        mpack_reader_t reader;
        mpack_reader_init_data(&reader, response.data(), response.size());

        mpack_expect_array_max(&reader, 4);
        const MessageType type = MessageType(mpack_expect_i32(&reader));

        if(type == MessageType::Notification) {
            handleNotification(reader);
        } else if(type == MessageType::Response) {
            if(mpack_expect_u32(&reader) != msgId) {
                Warning() << "NeovimApi{{api_level}}::waitForResponse(): Skipped a response that was not waited for.";
                continue;
            }
            const mpack_tag_t error = mpack_peek_tag(&reader);
            if(error.type != mpack_type_nil) {
                if(error.type == mpack_type_str) {
                    char strBuf[512];
                    mpack_expect_cstr(&reader, strBuf, sizeof(strBuf));
                    std::string errorMessage(strBuf);
                    if constexpr (std::is_void<T>::value) {
                        CORRADE_ASSERT(false, "NeovimApi{{api_level}}::waitForResponse(): " + errorMessage, );
                    } else {
                        CORRADE_ASSERT(false, "NeovimApi{{api_level}}::waitForResponse(): " + errorMessage, T{});
                    }
                }
            }
            mpack_discard(&reader);
            const mpack_tag_t result = mpack_peek_tag(&reader);

            if constexpr (std::is_void<T>::value) {
                CORRADE_ASSERT(result.type == mpack_type_nil,
                        "NeovimApi{{api_level}}::waitForResponse(): Expected nil return, but got " + std::to_string(result.type), );
            } else {
                T returnValue = mpack_read<T>(&reader);
                mpack_done_array(&reader);
                CORRADE_ASSERT(mpack_reader_destroy(&reader) == mpack_ok,
                        "NeovimApi{{api_level}}::waitForResponse(): Could not unpack response", T{});

                return std::forward<T>(returnValue);
            }
        } else if(type == MessageType::Request) {
            mpack_print(response.data(), response.size());
            const UnsignedInt msgid = mpack_expect_u32(&reader);
            char strBuf[512];
            mpack_expect_cstr(&reader, strBuf, sizeof(strBuf));
            Error() << "Received request" << std::string(strBuf) << "Server -> Client requests are not implemented.";
        } else {
            Error() << "Unknown message type" << Int(type);
        }
    } while(true);
}

static const char* NotificationTypeStrings[]{
{% for n in notifications %}
    "{{ n.name }}",
{% endfor %}
};

const char** getNotificationTypeStrings() {
    return NotificationTypeStrings;
}


{% for f in functions %}

{{f.return_type.native_type}} NeovimApi{{api_level}}::{{f.name}}({{f.argstring}}) {
{% if f.parameters %}
    const Int msgId = dispatch("{{f.name}}", {{ f.parameters|join(", ", attribute='name') }});
{% else %}
    const Int msgId = dispatch("{{f.name}}");
{% endif %}
{% if f.return_type.native_type != "void" %}
    return waitForResponse<{{f.return_type.native_type}}>(msgId);
{% endif %}
}
{% endfor %}

} // Namespace
