#include "visualizer.h"

#include <glm/gtx/transform.hpp>

#include <iostream>

namespace dfv {
    Visualizer::Visualizer(const VisualizerCreateInfo &createInfo)
        : surface(createInfo.surface), flightData(createInfo.flightData), objectModelPath(createInfo.objectModelPath),
          objectScale(createInfo.objectScale), objectRenderHandle(), engine(), inputHandler(), stats() {}

    Visualizer::~Visualizer() {
        engine.cleanup();
    }

    void Visualizer::start() {
        // Load flight data
        if (!flightData.load())
            throw std::runtime_error("Failed to load flight data");

        // Load initial map chunks
        FlightBoundingBox bbox = flightData.getBoundingBox();
        // TODO: Load map chunks semi-synchronously (first few chunks synchronously, rest with std::async)

        engine.init(surface);
        createScene();

        // Run user-defined start-up operations
        onStart();
    }

    void Visualizer::drawFrame(dfv::seconds_f deltaTime) {
        // Update entities
        auto updateStart = dfv::clock::now();
        update(deltaTime);
        stats.updateTotalTime += dfv::clock::now() - updateStart;

        // Draw frame
        auto drawStart = dfv::clock::now();
        engine.draw();
        stats.drawTotalTime += dfv::clock::now() - drawStart;
    }

    Visualizer::Stats Visualizer::getStats() {
        stats.frameCount = engine.getFrameNumber();
        return stats;
    }

    void Visualizer::setObjectTransform(const glm::vec3 &position, const glm::vec3 &attitude) {
        auto ufo = engine.getRenderObject(objectRenderHandle);
        ufo->transform = glm::translate(position) *
                         glm::rotate(attitude.x, glm::vec3{1.f, 0.f, 0.f}) *
                         glm::rotate(attitude.y, glm::vec3{0.f, 1.f, 0.f}) *
                         glm::rotate(attitude.z, glm::vec3{0.f, 0.f, 1.f}) *
                         glm::scale(glm::vec3{objectScale});
    }

    void Visualizer::createScene() {
        if (!is_regular_file(objectModelPath))
            throw std::runtime_error("Invalid object model file provided");

        auto ufoMesh = engine.createMesh("ufo", objectModelPath);
        auto defaultMat = engine.getMaterial("defaultmesh");

        auto [ufo, ufoHandle] = engine.allocateRenderObject();
        *ufo = {.mesh = ufoMesh,
                .material = defaultMat,
                .transform = glm::mat4{1.f}};

        objectRenderHandle = ufoHandle;
    }

} // namespace dfv
