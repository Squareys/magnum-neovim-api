// Auto generated {{date}} from nvim API level:{{api_level}}
#include "neovimapi{{api_level}}.h"


namespace NeovimApi {

using namespace Corrade;
using namespace Magnum;

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

Containers::Array<std::string> mpack_read_array_of_str(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_array(reader);

    Containers::Array<std::string> data{Containers::DefaultInit, numElements};

    char buffer[512];
    for(int i = 0; i < numElements; ++i) {
        mpack_expect_cstr(reader, buffer, sizeof(buffer));
        data[i] = std::string(buffer);
    }

    mpack_done_array(reader);
    return data;
}

Containers::Array<Long> mpack_read_array_of_long(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_array(reader);

    Containers::Array<Long> data{Containers::DefaultInit, numElements};

    for(int i = 0; i < numElements; ++i) {
        data[i] = mpack_expect_i64(reader);
    }

    mpack_done_array(reader);
    return data;
}

Containers::Array<Object> mpack_read_array_of_object(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_array(reader);

    Containers::Array<Object> data{Containers::DefaultInit, numElements};

    for(int i = 0; i < numElements; ++i) {
        data[i] = mpack_read_object(reader);
    }

    mpack_done_array(reader);
    return data;
}

std::unordered_map<std::string, Object>&& mpack_read_map(mpack_reader_t* reader) {
    const size_t numElements = mpack_expect_map(reader);

    std::unordered_map<std::string, Object> options{numElements};

    char buffer[512];
    for(int i = 0; i < numElements; ++i) {
        mpack_expect_cstr(reader, buffer, sizeof(buffer));
        options[std::string(buffer)] = mpack_read_object(reader);
    }

    mpack_done_map(reader);
    return std::move(options);
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

{% for f in functions %}

{{f.return_type.native_type}} NeovimApi{{api_level}}::{{f.name}}({{f.argstring}}) {
    char* data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);

    mpack_start_array(&writer, 4);
    mpack_write_i32(&writer, 0);
    mpack_write_i32(&writer, 0);
    mpack_write_cstr(&writer, "{{f.name}}");

    mpack_start_array(&writer, {{f.argcount}});
{% for p in f.parameters %}
{% if p.native_type == "Long" %}
    mpack_write_i64(&writer, {{p.name}});
{% elif p.native_type == "bool" %}
    mpack_write_bool(&writer, {{p.name}});
{% else %}
    mpack_write(&writer, {{p.name}});
{% endif %}
{% endfor %}
    mpack_finish_array(&writer);
    mpack_finish_array(&writer);
    CORRADE_ASSERT(mpack_writer_destroy(&writer) == mpack_ok, "NeovimApi{{api_level}}::{{f.name}}(): Could not pack request", {{"{}" if f.return_type.native_type != "void" else ""}});

    _socket.send(Containers::arrayView(data, size));
    Containers::ArrayView<char> response = _socket.receive(_receiveBuffer);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, response.data(), response.size());

    mpack_expect_array_max(&reader, 4);
    const Int type = mpack_expect_i32(&reader);
    const UnsignedInt msgid = mpack_expect_u32(&reader);
    const mpack_tag_t error = mpack_peek_tag(&reader);
    if(error.type != mpack_type_nil) {
        // TODO: An error occurred...
    }
    mpack_read_tag(&reader);
    const mpack_tag_t result = mpack_peek_tag(&reader);

{% if f.return_type.neovim_type != "void" %}
{% if f.return_type.neovim_type == "String" %}
    char strBuf[512];
    mpack_expect_cstr(&reader, strBuf, sizeof(strBuf));
    std::string returnValue(strBuf);
{% elif f.return_type.native_type == "Long" %}
    Long returnValue = mpack_expect_i64(&reader);
{% elif f.return_type.native_type == "Double" %}
    Double returnValue = mpack_expect_double(&reader);
{% elif f.return_type.native_type == "bool" %}
    bool returnValue = mpack_expect_bool(&reader);
{% elif f.return_type.native_type == "Object" %}
    Object returnValue = mpack_read_object(&reader);
{% elif f.return_type.native_type == "Vector2i" %}
    const Int x = mpack_expect_i32(&reader);
    const Int y = mpack_expect_i32(&reader);
    auto returnValue = Vector2i{x, y};
{% elif f.return_type.neovim_type == "Dictionary" %}
    auto returnValue = mpack_read_map(&reader);
{% elif f.return_type.elemtype == "String" %}
    Containers::Array<std::string> returnValue = mpack_read_array_of_str(&reader);
{% elif f.return_type.native_elemtype == "Long" %}
    Containers::Array<Long> returnValue = mpack_read_array_of_long(&reader);
{% elif f.return_type.elemtype == "Object" %}
    Containers::Array<Object> returnValue = mpack_read_array_of_object(&reader);
{% endif %}
    mpack_done_array(&reader);
    CORRADE_ASSERT(mpack_reader_destroy(&reader) == mpack_ok, "NeovimApi{{api_level}}::{{f.name}}(): Could not unpack response", {});

    return returnValue;
{% else %}
    CORRADE_ASSERT(result.type == mpack_type_nil, "NeovimApi{{api_level}}::{{f.name}}(): Expected nil return, but got " + std::to_string(result.type), );
{% endif %}
}
{% endfor %}

} // Namespace
