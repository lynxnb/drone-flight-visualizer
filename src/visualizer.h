#pragma once

#include "flight_data/flight_data.h"
#include "input/input_handler.h"
#include "utils/time_types.h"
#include "vulkan/surface_wrapper.h"
#include "vulkan/vk_engine.h"

namespace dfv {
    /**
     * @brief The visualizer class is responsible for visualizing the flight of an object, given a data source and a surface to render to.
     */
    class Visualizer {
        struct Stats {
            nanoseconds updateTotalTime; //!< The total time spent performing entity updates
            nanoseconds drawTotalTime; //!< The total time spent drawing entities
            uint32_t frameCount; //!< The number of frames drawn, only available when returned from getStats()
        };

      public:
        /**
         * @brief Constructs a new visualizer.
         * @param surface The surface to render to, must be valid for the lifetime of the visualizer.
         * @param data The data source to visualize, must be valid for the lifetime of the visualizer.
         */
        Visualizer(SurfaceWrapper &surface, FlightData &data);

        ~Visualizer();

        /**
         * @brief Initializes the visualizer by loading the data source, initializing the engine and creating the scene.
         */
        void init();

        /**
         * @brief Updates entities and draws the next frame of the visualization.
         * @param deltaTime The time since the last frame.
         */
        void drawFrame(seconds_f deltaTime);

        /**
         * @brief Gets the statistics of the visualizer.
         * @return The statistics of the visualizer.
         */
        Stats getStats();

      protected:
        /**
         * @brief Loads the initial scene.
         */
        virtual void loadInitialScene() = 0;

        /**
         * @brief Updates entities of the visualization.
         * @param deltaTime The time since the last frame.
         */
        virtual void update(seconds_f deltaTime) = 0;

      protected:
        SurfaceWrapper &surface; //!< The surface to render to
        FlightData &data; //!< The data source to visualize

        VulkanEngine engine; //!< The engine that handles rendering
        InputHandler inputHandler; //!< The input handler that handles input actions

        Stats stats; //!< The statistics of the visualizer
    };
} // namespace dfv
