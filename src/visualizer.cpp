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

                glm::vec3 heading = {std::sin(dataPoint.yaw),
                                     0.f,
                                     std::cos(dataPoint.yaw)};

                engine.camera.position += positionMask * cameraMovementSpeed * deltaTime.count();
                // Avoid sitting directly on top of the object
                if (target.x == engine.camera.position.x && target.z == engine.camera.position.z)
                    engine.camera.position -= heading * cameraMovementSpeed * deltaTime.count();

                auto direction = glm::normalize(target - engine.camera.position);
                auto distance = glm::length(engine.camera.position - target);
                // Avoid getting too close to the object
                if (distance < 1.f)
                    engine.camera.position = target - direction * 1.f;

                // Look in the direction of the object
                engine.camera.front = direction;
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
            // normalize between min and max altitude
            const float normalizedAltitude =
                    (dataPoint.y - flightData.getMinimumAltitude()) / (flightData.getMaximumAltitude() - flightData.getMinimumAltitude());
            // float pointer to normalizedAltitude
            static std::array<float, 100> values = {};
            static int valuesOffset = 0;
            values[valuesOffset++ % values.size()] = normalizedAltitude;

            const ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration |
                                                  ImGuiWindowFlags_AlwaysAutoResize |
                                                  ImGuiWindowFlags_NoSavedSettings |
                                                  ImGuiWindowFlags_NoFocusOnAppearing |
                                                  ImGuiWindowFlags_NoNav |
                                                  ImGuiWindowFlags_NoMove;

            const ImGuiViewport *viewport = ImGui::GetMainViewport();
            const ImVec2 workPos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            const ImVec2 workSize = viewport->WorkSize;

            constexpr float OVERLAY_PAD = 10.0f;
            const ImVec2 overlayPos = {workPos.x + OVERLAY_PAD,
                                       workPos.y + OVERLAY_PAD};
            ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always, {0.f, 0.f});
            ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
            if (ImGui::Begin("Drone data", nullptr, overlayFlags)) {
                ImGui::Text("Position");
                ImGui::Text("x: %.2f y: %.2f z: %.2f", dataPoint.x, dataPoint.y, dataPoint.z);
                ImGui::Separator();

                const int roundedMaximum = static_cast<int>(flightData.getMaximumAltitude());
                const std::string maxAltitudeString = std::to_string(roundedMaximum) + "m";
                ImGui::PlotLines(maxAltitudeString.c_str(), values.data(), values.size(), valuesOffset, "Altitude graph", 0.f, 1.0f, ImVec2(0, 80.0f));
                ImGui::Separator();
            }
            ImGui::End();

            const ImGuiWindowFlags playerFlags = ImGuiWindowFlags_NoDecoration |
                                                 ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                                 ImGuiWindowFlags_NoNav |
                                                 ImGuiWindowFlags_NoMove |
                                                 ImGuiWindowFlags_NoTitleBar;

            constexpr float PLAYER_PAD = 60.0f;
            const ImVec2 playerPos = {workPos.x + workSize.x / 2.f,
                                      workPos.y + workSize.y - PLAYER_PAD};
            ImGui::SetNextWindowPos(playerPos, ImGuiCond_Always, {0.5f, 1.f});
            ImGui::SetNextWindowBgAlpha(0.5f);
            ImGui::Begin("Player controls", nullptr, playerFlags);

            // Progress bar
            const float progress = (time - flightData.getStartTime()) / (flightData.getEndTime() - flightData.getStartTime());
            ImGui::ProgressBar(progress, ImVec2(workSize.x / 3, 0.f), "");

            ImGui::SameLine();
            // Time text
            const float normTime = time.count() - flightData.getStartTime().count();
            // Format time as HH:MM:SS
            ImGui::Text("%02d:%02d:%02d", static_cast<int>(normTime / 3600),
                        static_cast<int>(normTime / 60) % 60,
                        static_cast<int>(normTime) % 60);

            // Backward button
            if (ImGui::Button("Backward"))
                addToTimeMultiplier(-1.f);

            ImGui::SameLine();
            // Play button
            if (ImGui::Button("Play"))
                changeTimeMultiplier(1.f);

            ImGui::SameLine();
            // Pause button
            if (ImGui::Button("Pause"))
                changeTimeMultiplier(0.f);

            ImGui::SameLine();
            // Fast forward button
            if (ImGui::Button("Fast forward"))
                addToTimeMultiplier(1.f);

            ImGui::SameLine();
            // Multiplier text
            ImGui::Text("x%.1f", timeMultiplier);

            ImGui::SameLine();
            ImGui::Text("Camera: ");

            ImGui::SameLine();
            if (ImGui::Button("Free"))
                setCameraMode(CameraMode::Free);

            ImGui::SameLine();
            if (ImGui::Button("Locked"))
                setCameraMode(CameraMode::LockedOn);

            ImGui::SameLine();
            if (ImGui::Button("1st"))
                setCameraMode(CameraMode::Follow1stPerson);

            ImGui::SameLine();
            if (ImGui::Button("3rd"))
                setCameraMode(CameraMode::Follow3rdPerson);

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
