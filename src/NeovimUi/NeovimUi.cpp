#include <Magnum/Magnum.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Platform/GlfwApplication.h>

#include "neovimapi2.h"

using namespace Magnum;
using namespace NeovimApi;

class NeovimUiApplication: public Platform::Application {
    public:
        NeovimUiApplication(const Arguments& arguments);

    private:
        void drawEvent() override;

        NeovimApi2 _neovim;
        Containers::Array<char> _fg;
        int _cursor = 0;

        const Vector2i _size{25, 25};
};

NeovimUiApplication::NeovimUiApplication(const Arguments& arguments):
    Platform::Application{arguments},
    _neovim{6666}
{
    _neovim.nvim_ui_attach(_size.x(), _size.y(), {});
    _fg = Containers::Array<char>(Containers::DefaultInit, _size.product());
}

void NeovimUiApplication::drawEvent() {
    std::unique_ptr<Notification> notification = _neovim.waitForNotification(1000);

    if (notification->methodName().empty() || notification->data().data() == nullptr || notification->data().empty()) {
        redraw(); /* Wait for next notification */
        return;
    }

    bool needsRedraw = false;

    auto node = notification->parameters();
    const size_t numEvents = mpack_node_array_length(node);
    Debug() << "Received" << numEvents << "events";
    for (int i = 0; i < numEvents; ++i) {
        auto event = mpack_node_array_at(node, i);

        const EventType type = (EventType)mpack_node_enum(mpack_node_array_at(event, 0), 
                                                          getEventTypeStrings(), Int(EventType::Count));
        switch (type) {
        case EventType::put: {
            auto contentNode = mpack_node_array_at(event, 1);
            auto textNode = mpack_node_array_at(contentNode, 0);

            char* str = mpack_node_utf8_cstr_alloc(textNode, 2048);
            MPACK_FREE(str);

            _fg[_cursor] = str[0];

            break;
        }
        case EventType::cursor_goto: {
            auto contentNode = mpack_node_array_at(event, 1);
            auto x = mpack_node_u32(mpack_node_array_at(contentNode, 0));
            auto y = mpack_node_u32(mpack_node_array_at(contentNode, 1));
            _cursor = x + y*_size.x();
            break;
        }
        default:
            Debug() << "Unhandled event:" << type;
            mpack_node_print(event);
        }
    }

    for(int y = 0; y < _size.y(); ++y) {
        Debug() << std::string(_fg.prefix(y*_size.x()), _size.x());
    }

    if (needsRedraw) {
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
        swapBuffers();
    }

    Debug() << "Error:" << mpack_tree_error(&notification->tree());

    redraw();
}

MAGNUM_APPLICATION_MAIN(NeovimUiApplication)
