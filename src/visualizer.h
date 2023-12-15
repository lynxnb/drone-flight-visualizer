#pragma once

#include <filesystem>

#include "flight_data/flight_data.h"
#include <utils/time_types.h>
#include "vulkan/surface_wrapper.h"
#include "vulkan/vk_engine.h"

#include <map/map_manager.h>

namespace dfv {
    /**
     * @brief Structure containing creation parameters for the visualizer.
     */
    struct VisualizerCreateInfo {
        SurfaceWrapper &surface; //!< The surface to render to, must be valid for the lifetime of the visualizer
        FlightData &flightData; //!< The data source to visualize, must be valid for the lifetime of the visualizer
        std::filesystem::path objectModelPath; //!< The path to the flying object 3D model
        float objectScale; //!< The scale of the flying object model relative to the world coordinates (1u = 1m)
    };

    /**
     * @brief A structure describing the current movement of the camera.
     */
    struct CameraMovement {
        float surge; //!< The camera is moving forward or backwards (1 forward, -1 backwards)
        float sway; //!< The camera is moving left or right (1 right, -1 left)
        float heave; //!< The camera is moving up or down (1 up, -1 down)
        float tilt; //!< The camera is tilting (1 up, -1 down)
        float pan; //!< The camera is panning (1 left, -1 right)
    };

    /**
     * @brief The camera mode of the visualizer.
     */
    enum class CameraMode {
        Free, //!< The camera is free to move around the scene
        LockedOn, //!< The camera is locked onto the flying object but can be moved around it
        Follow1stPerson, //!< The camera follows the flying object in 1st person
        Follow3rdPerson, //!< The camera follows the flying object in 3rd person
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

        virtual ~Visualizer();

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
         * @brief Sets the movement state of the camera.
         */
        void setCameraMovement(const CameraMovement &movement);

        /**
         * @brief Turns the camera by the given amount.
         * @param rotation A vector containing the amount to rotate the camera by in radians (yaw, pitch, roll).
         */
        void turnCamera(const glm::vec3 &rotation);

        /**
         * @brief Recenters the camera on the flying object.
         * Applies to non-following camera modes only.
         */
        void recenterCamera();

        /**
         * @brief Sets the camera mode.
         */
        void setCameraMode(CameraMode mode);

        /**
         * @brief Gets the statistics of the visualizer.
         * @return The statistics of the visualizer.
         */
        Stats getStats();

        /**
         * @brief Changes the time multiplier of the drone visualization.
         * @param multiplier The new time multiplier. 0 to pause, 1 to resume, 2 to double the speed, etc.
         */
        void changeTimeMultiplier(float multiplier = 2.f);

      protected:
        /**
         * @brief Performs user-defined start-up operations
         */
        virtual void onStart() {}

        /**
         * @brief User-defined method called on every update (frame) of the visualization.
         * @param deltaTime The time since the last update.
         */
        virtual void onUpdate(seconds_f deltaTime) {}

        /**
         * @brief Sets the new position and attitude of the flying object.
         * @param position x, y, z coordinates of the flying object.
         * @param attitude yaw, pitch, roll angles of the flying object.
         */
        void setObjectTransform(const glm::vec3 &position, const glm::vec3 &attitude);

        FlightData &flightData; //!< The data source to visualize

      private:
        /**
         * @brief Loads the initial scene and sets up the camera.
         */
        void createScene();

        /**
         * @brief Updates the entities of the visualization.
         * @param deltaTime The time since the last update.
         */
        void update(seconds_f deltaTime);

        /**
         * @brief Updates the camera based on the current camera mode and movement state.
         * @param deltaTime The time since the last update.
         * @param dataPoint The current flight data point.
         */
        void updateCamera(seconds_f deltaTime, FlightDataPoint &dataPoint);

        SurfaceWrapper &surface; //!< The surface to render to
        VulkanEngine engine; //!< The engine that handles rendering
        MapManager mapManager; //!< The class that handles map loading

        CameraMode cameraMode{CameraMode::Free}; //!< The current camera mode
        float cameraMovementSpeed{5.f}; //!< The speed of the camera movement in m/s
        float cameraRotationSpeed{glm::radians(60.f)}; //!< The speed of the camera rotation in radians/s
        CameraMovement cameraMovement{}; //!< The current movement state of the camera

        std::filesystem::path objectModelPath; //!< The path to the flying object 3D model
        float objectScale; //!< The scale of the flying object model
        RenderHandle objectRenderHandle{}; //!< The render handle of the flying object

        seconds_f time{0}; //!< The current time of the visualization

        Stats stats{}; //!< The statistics of the visualizer
        float droneTimeMultiplier{1.f}; //!< The time multiplier of the drone visualization
    };
} // namespace dfv
