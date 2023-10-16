#pragma once

#include <unordered_map>
#include <functional>

#include <GLFW/glfw3.h>

namespace dfv {

    /**
     * The action that was performed on a key.
     */
    enum class Action {
        Pressed = GLFW_PRESS,
        Released = GLFW_RELEASE,
    };

    /**
     * The signature of a key handler.
     */
    using KeyHandler = std::function<void(Action)>;

    /**
     * The class that handles input events.
     */
    class InputHandler {
      public:
        /**
         * Handles a key event.
         * @param key The key that was pressed (GLFW_KEY_*).
         * @param scancode The system-specific scancode of the key.
         * @param action The action that was performed (GLFW_PRESS or GLFW_RELEASE).
         * @param mods The modifier keys that were held down (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT, GLFW_MOD_SUPER).
         */
        void handleKey(int key, int scancode, int action, int mods) const;

        /**
         * Adds a key mapping to the input handler.
         * @param key The key to map (GLFW_KEY_*).
         * @param handler The handler to execute when the key is pressed.
         */
        void addKeyMapping(int key, const KeyHandler& handler);

        /**
         * Removes a key mapping from the input handler.
         * @param key The key to remove the mapping for (GLFW_KEY_*).
         */
        void removeKeyMapping(int key);

      private:
        std::unordered_map<int, KeyHandler> keyMap; //!< The key mappings.
    };

} // namespace dfv
