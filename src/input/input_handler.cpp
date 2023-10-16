#include "input_handler.h"

namespace dfv {

    void InputHandler::handleKey(int key, int scancode, int action, int mods) const {
        auto it = keyMap.find(key);
        if (it != keyMap.end())
            it->second(static_cast<Action>(action));
    }

    void InputHandler::addKeyMapping(int key, const dfv::KeyHandler &handler) {
        keyMap.insert_or_assign(key, handler);
    }

    void InputHandler::removeKeyMapping(int key) {
        keyMap.erase(key);
    }

} // namespace dfv
