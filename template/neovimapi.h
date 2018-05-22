// Auto generated {{date}} from nvim API level: {{api_level}}
#ifndef NEOVIM_API{{api_level}}
#define NEOVIM_API{{api_level}}

#include <unordered_map>

#include "mpack/mpack.h"
#include "Corrade/Net/Socket.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

namespace NeovimApi {

typedef mpack_tag_t Object;
using Magnum::Long;
using Magnum::Vector2i;

class NeovimApi{{api_level}} {
public:

    /** @brief Constructor to connect to a local neovim */
    NeovimApi{{api_level}}(int port, int receiveBufferSize=512);

    /** @brief Constructor to connect to neovim */
    NeovimApi{{api_level}}(const std::string& host, int port, int receiveBufferSize=512);

{% for f in functions %}

    /** {{f.signature()}} */
{% if f.deprecated() %}
    CORRADE_DEPRECATED("Refer to the nvim API documentation for more information.")
{% endif %}
    {{f.return_type.native_type}} {{f.name}}({{f.argstring}});
{% endfor %}

private:

    Corrade::Net::Socket _socket;
    Corrade::Containers::Array<char> _receiveBuffer;
};

}
#endif
