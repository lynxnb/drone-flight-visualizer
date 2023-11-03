#include "render_object.h"

#include <glm/gtx/transform.hpp>

namespace dfv {

    void RenderObject::computeTransform() {
        transformMatrix = glm::translate(glm::mat4{1.0f}, position) *
                          glm::scale(glm::mat4{1.0f}, scale) *
                          glm::rotate(glm::mat4{1.0f}, orientation.x, glm::vec3{1.0f, 0.0f, 0.0f}) *
                          glm::rotate(glm::mat4{1.0f}, orientation.y, glm::vec3{0.0f, 1.0f, 0.0f});
    }

} // namespace dfv
