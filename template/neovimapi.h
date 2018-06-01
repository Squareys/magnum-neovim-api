// Auto generated {{date}} from nvim API level: {{api_level}}
#ifndef NEOVIM_API{{api_level}}
#define NEOVIM_API{{api_level}}

#include <unordered_map>

#include "mpack/mpack.h"
#include "Corrade/Net/Socket.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayView.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

#include <vector>

namespace NeovimApi {

using Magnum::Int;
using Magnum::Long;
using Magnum::Vector2i;

enum class MessageType: Int {
    Request = 0,
    Response = 1,
    Notification = 2
};

enum class NotificationType: Int {
{% for n in notifications %}
    {{ n.name }},
{% endfor %}
    Count
};

/**
@brief A multi-type storage

Inefficient implementation of a variadic, stores either a
string, bool or Long.
*/
struct Object {
    /** Indicates which member is valid */
    mpack_type_t type;
    union {
        bool b;
        Long l;
    };
    std::string s;
};

/**
@brief A msgpack-rpc notification

A message from the server to the client.
*/
struct Notification {
    /** Method called by the server */
    NotificationType method;
    /** Parameters, binary data in msgpack format, the parameters array. */
    Corrade::Containers::Array<char> parameters;
};

/**
@brief A neovim API client

Allows dispatching msgpack-rpc requests to a neovim instance via TCP and then
receive notification and responses back.

Warning: Nothing is thread safe here at the moment!
*/
class NeovimApi{{api_level}} {
public:

    /** @brief Constructor to connect to a local neovim */
    NeovimApi{{api_level}}(int port, int receiveBufferSize=2048*32);

    /** @brief Constructor to connect to neovim */
    NeovimApi{{api_level}}(const std::string& host, int port, int receiveBufferSize=2048*32);

    /**
     * @brief Dispatch a msgpack-rpc request
     * @param func Name of the function to call
     * @param args Arguments to the function
     * @return An automatically incremented msgId to use with @ref waitForResponse()
     */
    template<typename... Args>
    Int dispatch(const std::string& func, Args... args);

    /**
     * @brief Wait for a reponse to a dispatched request with given id
     * @param msgId Message id to wait for a response to
     * @return The return value from the response
     *
     * This method will save all encountered notifications for later.
     * See @ref pollNotifications().
     */
    template<typename T>
    auto waitForResponse(Int msgId);

    /**
     * @brief Poll pending notifications
     * @return array of notifications
     *
     * Copies currently pending notifications and then clears pending notifications.
     */
    Corrade::Containers::Array<Notification> pollNotifications() {
        Corrade::Containers::Array<Notification> notifications(Corrade::Containers::DefaultInit, _notifications.size());
        for(int i = 0; i < _notifications.size(); ++i) {
            notifications[i] = std::move(_notifications[i]);
        }
        _notifications.clear();
        return notifications;
    }

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
    std::vector<Notification> _notifications;

    int _nextMessageId = 0;
    /* Generate the next message id */
    int nextMessageId() { return _nextMessageId++; }

    void handleNotification(mpack_reader_t& reader);
};

}
#endif
