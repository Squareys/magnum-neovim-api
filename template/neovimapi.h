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

enum class EventType: Int {
{% for e in events %}
    {{ e.name }},
{% endfor %}
    Count,
};

/** @debugoperatorenum{NeovimApi::EventType} */
Magnum::Debug& operator<<(Magnum::Debug& debug, EventType value);

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
class Notification {
public:

    /** @brief Constructor which copies data from an ArrayView
     * @param data mpack data to copy and then parse
     *
     * Copies and parses the mpack tree of given data.
     */
    Notification(Corrade::Containers::ArrayView<char> data):
        _data{Corrade::Containers::DefaultInit, data.size()}
    {
        std::copy(data.begin(), data.end(), _data.data());
        parse();
    }

    /** @brief Constructor
     * @param data mpack data to parse
     *
     * Parses the mpack tree of given data.
     */
    Notification(Corrade::Containers::Array<char>&& data): _data{std::move(data)} {
        parse();
    }

    /* @brief Move constructor */
    Notification(Notification&& other): _data{std::move(other._data)} {
        _tree = other._tree;
        _root = other._root;
        /* other.data will be nullptr and therefore not deinitialized */
    }

    /** @brief Destructor
     *
     * Frees held data and destroys the mpack tree.
     */
    ~Notification() {
        if(_data.data() == nullptr) return;
        CORRADE_ASSERT(mpack_tree_destroy(&_tree) == mpack_ok, "NeovimApi::Notification::~Notification(): Error parsing notification data",);
    }

    /** Method called by the server */
    std::string methodName() const {
        char* name = mpack_node_cstr_alloc(mpack_node_array_at(_root, 1), 256);
        std::string func{name};
        MPACK_FREE(name);
        return func;
    }

    /** @brief Get node containing the parameters of this notification */
    mpack_node_t parameters() const {
        return mpack_node_array_at(_root, 2);
    }

    /** @brief Get the underlying mpack tree handle */
    mpack_tree_t& tree() {
        return _tree;
    }

    /** @brief Get a view on the underlying data */
    Corrade::Containers::ArrayView<const char> data() const {
        return Corrade::Containers::arrayView(_data);
    }

private:
    /* Initialize mpack structures for parsing */
    void parse() {
        mpack_tree_init(&_tree, _data.data(), _data.size());
        mpack_tree_parse(&_tree);
        _root = mpack_tree_root(&_tree);
    }

    Corrade::Containers::Array<char> _data;
    mpack_tree_t _tree;
    mpack_node_t _root;
};

/** Get event type strings for use with mpack_expect_enum() */
const char** getEventTypeStrings();

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
     * @param timeout Timeout in ms, `-1` for infinite timeout, default `-1`.
     * @return The return value from the response
     *
     * This method will save all encountered notifications for later.
     * See @ref pollNotifications().
     */
    template<typename T>
    auto waitForResponse(Int msgId, Int timeout=-1);

    /**
     * @brief Wait for the next notification
     * @param timeout Timeout in ms, `-1` for infinite timeout, default `-1`.
     *
     * Waits for the next notification. Blocks in case there are no notifications stored,
     * otherwise returns the first notification from stored notifications.
     * If the wait timed out, will return notification type @ref NotificationType::Timeout.
     *
     * @see pollNotifications()
     */
    std::unique_ptr<Notification> waitForNotification(Int timeout=-1);

    /**
     * @brief Poll pending notifications
     * @return array of notifications
     *
     * Copies currently pending notifications and then clears pending notifications.
     */
    Corrade::Containers::Array<std::unique_ptr<Notification>> pollNotifications() {
        Corrade::Containers::Array<std::unique_ptr<Notification>> notifications(Corrade::Containers::DefaultInit, _notifications.size());
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
    std::vector<std::unique_ptr<Notification>> _notifications;

    int _nextMessageId = 0;
    /* Generate the next message id */
    int nextMessageId() { return _nextMessageId++; }

};

}
#endif
