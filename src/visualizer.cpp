#include <chrono>

#include "visualizer.h"
#include <map/map_manager.h>

#include <glm/gtx/transform.hpp>
#include <imgui.h>

namespace dfv {
    Visualizer::Visualizer(const VisualizerCreateInfo &createInfo)
        : flightData(createInfo.flightData), surface(createInfo.surface), engine(createInfo.surface),
          droneModelPath(createInfo.droneModelPath), droneScale(createInfo.droneScale) {}

    Visualizer::~Visualizer() {
        engine.cleanup();
    }

    void Visualizer::start() {
        // Load flight data
        if (!flightData.load())
            throw std::runtime_error("Failed to load flight data");

        // Start the visualization at the start time of the flight data
        time = flightData.getStartTime();

        // Tell the map manager to start loading the map
        mapManager.startLoad(flightData, false);

        engine.init();
        createScene();

        // Run user-defined start-up operations
        onStart();
    }

    void Visualizer::drawFrame(const seconds_f deltaTime) {
        using namespace std::chrono_literals;

        // Update entities
        const auto updateStart = clock::now();
        update(deltaTime);

        // Spin loop so that update time is at least 0.5ms, fixes stuttering caused by low update times
        while (clock::now() - updateStart < 0.5ms)
            ;
        stats.updateTotalTime += clock::now() - updateStart;

        // Draw frame
        const auto drawStart = clock::now();
        engine.draw();
        stats.drawTotalTime += clock::now() - drawStart;
    }

    Visualizer::Stats Visualizer::getStats() {
        stats.frameCount = engine.getFrameNumber();
        return stats;
    }

    void Visualizer::setObjectTransform(const glm::vec3 &position, const glm::vec3 &attitude) {
        const auto drone = engine.getRenderObject(droneRenderHandle);
        drone->transform = glm::translate(position) *
                           glm::rotate(attitude.x, glm::vec3{0.f, 1.f, 0.f}) * // yaw
                           glm::rotate(attitude.y, glm::vec3{1.f, 0.f, 0.f}) * // pitch
                           glm::rotate(attitude.z, glm::vec3{0.f, 0.f, 1.f}) * // roll
                           glm::scale(glm::vec3{droneScale});
    }

    void Visualizer::setCameraMovement(const CameraMovement &movement) {
        cameraMovement = movement;
    }

    void Visualizer::turnCamera(const glm::vec3 &rotation) {
        switch (cameraMode) {
            case CameraMode::Free: {
                engine.camera.orientation += rotation;
            } break;

            default:
                break;
        }
    }

    void Visualizer::recenterCamera() {
        if (cameraMode == CameraMode::Free) {
            const auto point = flightData.getPoint(time);
            const auto direction = glm::normalize(glm::vec3{point.x, point.y, point.z} - engine.camera.position);

            engine.camera.orientation = {std::atan2(direction.z, direction.x),
                                         std::asin(direction.y),
                                         0.f};
            engine.camera.updateFront();
        }
    }

    void Visualizer::setCameraMode(const CameraMode mode) {
        cameraMode = mode;
    }

    void Visualizer::createScene() {
        if (!is_regular_file(droneModelPath))
            throw std::runtime_error("Invalid object model file provided");

        const auto firstDataPoint = flightData.getPoint(flightData.getStartTime());

        // Set the initial camera position and direction
        engine.camera.position = glm::vec3{firstDataPoint.x, firstDataPoint.y, firstDataPoint.z} +
                                 glm::vec3{0.f, 1.f, -5.f};
        engine.camera.orientation = {glm::radians(90.f), 0.f, 0.f};
        engine.camera.updateFront();

        const auto droneMesh = engine.createMesh("drone", droneModelPath);

        // adds the drone to the scene setting material and position
        auto [drone, droneHandle] = engine.allocateRenderObject();
        *drone = {.mesh = droneMesh,
                  .material = engine.getMaterial("drone"),
                  .transform = glm::mat4{1.f}};

        droneRenderHandle = droneHandle;
    }

    void Visualizer::update(const seconds_f deltaTime) {
        time += deltaTime * timeMultiplier;

        auto point = flightData.getPoint(time);
        setObjectTransform(glm::vec3{point.x, point.y, point.z},
                           glm::vec3{point.yaw, point.pitch, point.roll});

        static RenderHandle sMapHandle{NullHandle};

        // Try to add the map to the engine if it's ready
        auto meshOpt = mapManager.getMapMesh();
        if (meshOpt) {
            auto [mapObject, mapHandle] = engine.allocateRenderObject();
            *mapObject = {.mesh = engine.insertMesh("map", std::move(*meshOpt)),
                          .material = engine.getMaterial("map_simple"),
                          .transform = glm::mat4{1.f}};
            sMapHandle = mapHandle;
        }

        auto textureOpt = mapManager.getMapTexture();
        if (textureOpt) {
            const auto texture = engine.insertTexture("map", *textureOpt, true);
            engine.applyTexture(sMapHandle, texture, engine.getMaterial("map_textured"));
        }

        // Update camera
        updateCamera(deltaTime, point);

        updateUi(point);

        // Run user-defined updates
        onUpdate(deltaTime);
    }

    void Visualizer::updateCamera(seconds_f deltaTime, FlightDataPoint &dataPoint) {
        switch (cameraMode) {
            case CameraMode::Free: {
                // Remove the y component to avoid moving up/down when moving on the xz plane
                glm::vec3 frontPlane = glm::normalize(glm::vec3{engine.camera.front.x, 0.f, engine.camera.front.z});
                glm::vec3 rightPlane = glm::normalize(glm::cross(engine.camera.up, frontPlane));

                glm::vec3 positionMask = cameraMovement.surge * frontPlane +
                                         cameraMovement.sway * rightPlane +
                                         cameraMovement.heave * engine.camera.up;
                glm::vec3 rotationMask = {cameraMovement.pan, // yaw
                                          cameraMovement.tilt, // pitch
                                          0};

                engine.camera.position += positionMask * cameraMovementSpeed * deltaTime.count();
                engine.camera.orientation += rotationMask * cameraRotationSpeed * deltaTime.count();
                // Limit pitch range to prevent the camera from flipping
                engine.camera.orientation.y = glm::clamp(engine.camera.orientation.y,
                                                         glm::radians(-89.f), glm::radians(89.f));
                engine.camera.updateFront();
            } break;

            case CameraMode::LockedOn: {
                glm::vec3 frontPlane = glm::normalize(glm::vec3{engine.camera.front.x, 0.f, engine.camera.front.z});
                glm::vec3 rightPlane = glm::normalize(glm::cross(engine.camera.up, frontPlane));

                // The amount to move the camera in each direction x, y, z
                glm::vec3 positionMask = cameraMovement.surge * frontPlane +
                                         cameraMovement.sway * rightPlane +
                                         cameraMovement.heave * engine.camera.up;

                glm::vec3 target = {dataPoint.x,
                                    dataPoint.y,
                                    dataPoint.z};

                engine.camera.position += positionMask * cameraMovementSpeed * deltaTime.count();
                // Look in the direction of the object
                engine.camera.front = glm::normalize(target - engine.camera.position);
                engine.camera.updateOrientation();
            } break;

            case CameraMode::Follow1stPerson: {
                glm::vec3 target = {dataPoint.x,
                                    dataPoint.y + 0.4f, // Float the camera slightly above the object
                                    dataPoint.z};

                // TODO: implement configurable vertical first-person offset
                glm::vec3 heading = {std::sin(dataPoint.yaw),
                                     0.f,
                                     std::cos(dataPoint.yaw)};

                engine.camera.position = target;
                engine.camera.front = heading;
                engine.camera.updateOrientation();
            } break;

            case CameraMode::Follow3rdPerson: {
                glm::vec3 target = {dataPoint.x,
                                    dataPoint.y,
                                    dataPoint.z};

                // TODO: implement configurable follow distance
                glm::vec3 heading = {std::sin(dataPoint.yaw),
                                     -0.4f, // Float the camera slightly above the object
                                     std::cos(dataPoint.yaw)};

                // Place the camera 5m behind the object in the direction it's heading
                engine.camera.position = target - heading * 5.f;
                engine.camera.front = glm::normalize(target - engine.camera.position);
                engine.camera.updateOrientation();
            } break;
        }
    }

    void Visualizer::updateUi(const FlightDataPoint &dataPoint) {
        engine.submitUi([&] {
            static int location = 0;
            // normalize between min and max altitude
            float normalizedAltitude = (dataPoint.y - flightData.getMinimumAltitude()) / (flightData.getMaximumAltitude() - flightData.getMinimumAltitude());
            // float pointer to normalizedAltitude
            static std::array<float,100> values = {};
            static int valuesOffset = 0;
            values[valuesOffset++ % values.size()] = normalizedAltitude;

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
            if (location >= 0)
            {
                const float PAD = 10.0f;
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
                ImVec2 work_size = viewport->WorkSize;
                ImVec2 window_pos, window_pos_pivot;
                window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
                window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
                window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
                window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
                window_flags |= ImGuiWindowFlags_NoMove;
            }
            else if (location == -2)
            {
                // Center window
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                window_flags |= ImGuiWindowFlags_NoMove;
            }
            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            if (ImGui::Begin("Drone data", nullptr, window_flags))
            {
                ImGui::Text("Position");
                ImGui::Text("x: %.2f y: %.2f z: %.2f",dataPoint.x, dataPoint.y, dataPoint.z);
                ImGui::Separator();

                int roundedMaximum = (int)flightData.getMaximumAltitude();
                std::string maxAltitudeString = std::to_string(roundedMaximum) + "m";
                ImGui::PlotLines(maxAltitudeString.c_str(), values.data(), values.size(), valuesOffset, "Altitude graph", 0.f, 1.0f, ImVec2(0, 80.0f));
                ImGui::Separator();

            }
            ImGui::End();
        });
    }

    void Visualizer::changeTimeMultiplier(const float multiplier) {
        timeMultiplier = multiplier;
    }

    void Visualizer::addToTimeMultiplier(float addend) {
        timeMultiplier += addend;
    }

} // namespace dfv
