#pragma once

#include <filesystem>

#include "flight_data/flight_data.h"
#include "input/input_handler.h"
#include "utils/time_types.h"
#include "vulkan/surface_wrapper.h"
#include "vulkan/vk_engine.h"

namespace dfv {
    /**
     * @brief Structure containing creation parameters for the visualizer.
     */
    struct VisualizerCreateInfo {
        SurfaceWrapper &surface; //!< The surface to render to, must be valid for the lifetime of the visualizer.
        FlightData &flightData; //!< The data source to visualize, must be valid for the lifetime of the visualizer.
        std::filesystem::path objectModelPath; //!< The path to the flying object 3D model
        float objectScale; //!< The scale of the flying object model relative to the world coordinates (1u = 1m)
    };

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
         * @param createInfo A structure containing creation parameters for the visualizer.
         */
        explicit Visualizer(const VisualizerCreateInfo &createInfo);

        ~Visualizer();

        /**
         * @brief Initializes the visualizer by loading the data source, initializing the engine and creating the scene.
         */
        void start();

        /**
         * @brief Updates entities and draws the next frame of the visualization.
         * @param deltaTime The time since the last frame.
         * @details This is meant to be called continuously in a loop.
         */
        void drawFrame(seconds_f deltaTime);

        /**
         * @brief Gets the statistics of the visualizer.
         * @return The statistics of the visualizer.
         */
        Stats getStats();

      protected:
        /**
         * @brief Performs user-defined start-up operations
         */
        virtual void onStart() {};

        /**
         * @brief User-defined method called on every update (frame) of the visualization.
         * @param deltaTime The time since the last update.
         */
        virtual void onUpdate(seconds_f deltaTime) {};

        /**
         * @brief Sets the new position and attitude of the flying object.
         * @param position x, y, z coordinates of the flying object.
         * @param attitude pitch, yaw, roll angles of the flying object.
         */
        void setObjectTransform(const glm::vec3 &position, const glm::vec3 &attitude);

        FlightData &flightData; //!< The data source to visualize

      private:
        /**
         * @brief Loads the initial scene.
         */
        void createScene();

        /**
         * @brief Updates the entities of the visualization.
         * @param deltaTime The time since the last update.
         */
        void update(seconds_f deltaTime);

        SurfaceWrapper &surface; //!< The surface to render to

        std::filesystem::path objectModelPath; //!< The path to the flying object 3D model
        float objectScale; //!< The scale of the flying object model
        RenderHandle objectRenderHandle; //!< The render handle of the flying object

        VulkanEngine engine; //!< The engine that handles rendering
        InputHandler inputHandler; //!< The input handler that handles input actions

        seconds_f time{}; //!< The current time of the visualization

        Stats stats; //!< The statistics of the visualizer
    };
} // namespace dfv
