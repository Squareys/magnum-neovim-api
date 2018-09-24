#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Containers/Optional.h>

#include <Magnum/Magnum.h>
#include <Magnum/Renderer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/Renderer.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/DistanceFieldVector.h>

#include "neovimapi4.h"

using namespace Magnum;
using namespace NeovimApi;

struct TextLayer {
    std::string _content;
    std::unique_ptr<Text::AbstractFont> _font;
    Text::DistanceFieldGlyphCache _cache{Vector2i(2048), Vector2i(512), 22};
    std::unique_ptr<Text::Renderer3D> _renderer;
    Vector2i _size;

    void resize(Vector2i size) {
        if(size == _size) return;
        _size = size;
        _content.clear();
        _content.assign(" ", size.product() + size.y());
    }

    void clear() {
        for(int i = 0; i < _content.size(); ++i) _content[i] = (i % (_size.x() + 1) == 0) ? '\n' : ' ';
    }
};

class NeovimUiApplication: public Platform::Application {
    public:
        NeovimUiApplication(const Arguments& arguments);

    private:
        void drawEvent() override;
        void clear();

        NeovimApi4 _neovim;

        const Vector2i _size{38, 20};

        TextLayer _standard;
        TextLayer _bold;
        TextLayer _itallic;

        TextLayer* _textLayers[3]{&_standard, &_bold, &_itallic};

        struct {
            int index = 0;
            Vector2i pos{};
        } _cursor;

        /** Advance the cursor by 1, wrapping at end of line */
        void advanceCursor() {
            ++_cursor.pos.x();
            if (_cursor.pos.x() > _size.x()) {
                _cursor.pos.x() = 0;
                ++_cursor.pos.y();
            }
            _cursor.index = _cursor.pos.x() + _cursor.pos.y()*(_size.x() + 1);
        }

        static Color3 mpack_node_color(mpack_node_t node) {
            UnsignedInt color = UnsignedInt(mpack_node_i32(node));
            return Color3(Color3ub::from(reinterpret_cast<UnsignedByte*>(&color)));
        }

        /* Text rendering */
        std::string _cachedGlyphs =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789?!:;,.()][{}/\\ "
            "<>\"§$%&|#~+*-_`´'=";
        Magnum::Shaders::DistanceFieldVector3D _textShader;
};

NeovimUiApplication::NeovimUiApplication(const Arguments& arguments):
    Platform::Application{arguments},
    _neovim{6666}
{
    _neovim.nvim_ui_attach(_size.y(), _size.x(), {});
    /* Text rendering setup */
    static PluginManager::Manager<Text::AbstractFont> manager{};
    /* Open the font and fill glyph cache */
    static const char* fonts[3] = {"C:\\Windows\\Fonts\\CONSOLA-Powerline.ttf", "C:\\Windows\\Fonts\\CONSOLAB-Powerline.ttf", "C:\\Windows\\Fonts\\CONSOLAI-Powerline.ttf"};
    for(int i = 0; i < 3; ++i) {
        TextLayer& layer = *_textLayers[i];
        layer._font = std::move(manager.loadAndInstantiate("FreeTypeFont"));
        if (!layer._font) std::exit(1);

        if (!layer._font->openFile(fonts[i], 110.0f)) {
            Error() << "Could not load font" << fonts[i];
            std::exit(1);
        }

        layer._renderer.reset(new Text::Renderer3D(*layer._font, layer._cache, 0.1f));
        layer._renderer->reserve(1024*10, BufferUsage::StaticDraw, BufferUsage::DynamicDraw);
    }
    _textShader.setColor(Color4{1.0f})
           .setSmoothness(0.075f)
           .setOutlineRange(0.5f, 0.5f);

    clear();
}

void NeovimUiApplication::drawEvent() {
    std::unique_ptr<Notification> notification = _neovim.waitForNotification(1000);

    if (notification->data().data() == nullptr || notification->data().empty() || notification->methodName().empty()) {
        redraw(); /* Wait for next notification */
        return;
    }

    bool needsRedraw = false;

    char buffer[256];
    auto node = notification->parameters();
    const size_t numEvents = mpack_node_array_length(node);
    Debug() << "Received" << numEvents << "events";

    Color3 bgColor, fgColor, spColor;
    struct {
        bool bold;
        Color3 bg, fg;
    } highlight;

    for (int e = 0; e < numEvents; ++e) {
        auto event = mpack_node_array_at(node, e);
        if(mpack_node_is_nil(event)) break;

        const EventType type = (EventType)mpack_node_enum_optional(mpack_node_array_at(event, 0),
                                                          getEventTypeStrings(), Int(EventType::Count));

        switch (type) {
        case EventType::put: {
            const int numChars = mpack_node_array_length(event);

            TextLayer& layer = highlight.bold ? _bold : _standard;
            for(int i = 1; i < numChars; ++i) {
                auto contentNode = mpack_node_array_at(event, i);
                auto textNode = mpack_node_array_at(contentNode, 0);

                mpack_node_copy_utf8_cstr(textNode, buffer, 256);
                if(_cachedGlyphs.find(buffer) == std::string::npos) {
                    _cachedGlyphs.append(buffer);
                }
                layer._content.replace(_cursor.index, 1, buffer);
                advanceCursor();
            }

            needsRedraw = true;
            break;
        }
        case EventType::cursor_goto: {
            auto contentNode = mpack_node_array_at(event, 1);
            auto x = mpack_node_u32(mpack_node_array_at(contentNode, 0));
            auto y = mpack_node_u32(mpack_node_array_at(contentNode, 1));
            _cursor.pos = { Int(x), Int(y) };
            _cursor.index = x + y*(_size.x() + 1);
            break;
        }
        case EventType::clear: {
            clear();
            break;
        }
        case EventType::highlight_set: {
            auto contentNode = mpack_node_array_at(mpack_node_array_at(event, 1), 0);

            mpack_node_t boldNode = mpack_node_map_cstr_optional(contentNode, "bold");
            CORRADE_ASSERT(!mpack_node_is_nil(boldNode), "highlight_set: Error parsing bool property",);
            highlight.bold = mpack_node_is_missing(boldNode) ? false : mpack_node_bool(boldNode);

            mpack_node_t foregroundNode = mpack_node_map_cstr_optional(contentNode, "foreground");
            CORRADE_ASSERT(!mpack_node_is_nil(foregroundNode), "highlight_set: Error parsing foreground property",);
            highlight.fg = mpack_node_is_missing(foregroundNode) ? fgColor : mpack_node_color(foregroundNode);

            mpack_node_t backgroundNode = mpack_node_map_cstr_optional(contentNode, "background");
            CORRADE_ASSERT(!mpack_node_is_nil(backgroundNode), "highlight_set: Error parsing background property",);
            highlight.bg = mpack_node_is_missing(backgroundNode) ? fgColor : mpack_node_color(backgroundNode);

            break;
        }
        case EventType::update_fg: {
            auto contentNode = mpack_node_array_at(event, 1);
            fgColor = mpack_node_color(mpack_node_array_at(contentNode, 0));
            break;
        }
        case EventType::update_bg: {
            auto contentNode = mpack_node_array_at(event, 1);
            bgColor = mpack_node_color(mpack_node_array_at(contentNode, 0));
            break;
        }
        case EventType::update_sp: {
            auto contentNode = mpack_node_array_at(event, 1);
            spColor = mpack_node_color(mpack_node_array_at(contentNode, 0));
            break;
        }
        case EventType::resize: {
            auto contentNode = mpack_node_array_at(event, 1);
            auto w = mpack_node_u32(mpack_node_array_at(contentNode, 0));
            auto h = mpack_node_u32(mpack_node_array_at(contentNode, 1));

            for(auto layer : _textLayers) layer->resize({Int(w), Int(h)});
            break;
        }
        default:
            Debug() << "Unhandled event:" << type;
            mpack_node_print(event);
        }
    }

    if (needsRedraw) {
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

        for (TextLayer* layer : _textLayers) {
            layer->_font->fillGlyphCache(layer->_cache, _cachedGlyphs);
            _textShader.bindVectorTexture(layer->_cache.texture());
            layer->_renderer->render(layer->_content);

            /* Render all text */
            Renderer::setDepthMask(false);
            Renderer::enable(Renderer::Feature::Blending);

            _textShader.setTransformationProjectionMatrix(Matrix4::orthographicProjection({3, 3}, 0.001f, 10.0f)*Matrix4::translation({-1.0f, 1.0f, -2.0f}));
            layer->_renderer->mesh().draw(_textShader);

            Renderer::setDepthMask(true);
            Renderer::disable(Renderer::Feature::Blending);
        }

        swapBuffers();
    }

    Debug() << "Error:" << mpack_tree_error(&notification->tree());

    redraw();
}

void NeovimUiApplication::clear() {
    for(auto layer : _textLayers) layer->clear();
}

MAGNUM_APPLICATION_MAIN(NeovimUiApplication)
