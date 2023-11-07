#include "visualizer.h"

#include <iostream>

namespace dfv {
    Visualizer::Visualizer(SurfaceWrapper &surface, FlightData &data)
        : surface(surface), data(data), engine(), inputHandler(), stats() {}

    Visualizer::~Visualizer() {
        engine.cleanup();
    }

    void Visualizer::init() {
        // Load flight data
        if (!data.load()) {
            std::cerr << "Failed to load flight data" << std::endl;
            return;
        }

        // Load initial map chunks
        FlightBoundingBox bbox = data.getBoundingBox();
        // TODO: Load map chunks semi-synchronously (first few chunks synchronously, rest with std::async)

        engine.init(surface);

        loadInitialScene();
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

} // namespace dfv
