// Auto generated {{date}} from nvim API level:{{api_level}}
#include "neovimapi{{api_level}}.h"


namespace NeovimApi {

using namespace Corrade;

void mpack_write(mpack_writer_t* writer, Object val) {
    mpack_write_tag(writer, val);
}

void mpack_write(mpack_writer_t* writer, const Vector2i& val) {
    mpack_write_i32(writer, val.x());
    mpack_write_i32(writer, val.y());
}

void mpack_write(mpack_writer_t* writer, const std::string& val) {
    mpack_write_cstr(writer, val.c_str());
}

void mpack_write(mpack_writer_t* writer, const std::unordered_map<std::string, Object>& options) {
    mpack_start_map(writer, options.size());
    for(const std::pair<std::string, Object>& e : options) {
        mpack_write(writer, e.first);
        mpack_write(writer, e.second);
    }
    mpack_finish_map(writer);
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
{% if f.return_type.native_type != "void" %}
    Containers::ArrayView<char> response = _socket.receive(_receiveBuffer);
    mpack_print(response.data(), response.size());

    return {};
{% endif %}
}
{% endfor %}

} // Namespace
