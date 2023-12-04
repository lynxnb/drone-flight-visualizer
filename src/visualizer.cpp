#include "visualizer.h"
#include "map/data_fetcher.h"
#include "structs/data_structs.h"

#include <glm/gtx/transform.hpp>

#include <iostream>

namespace dfv {
    Mesh setupMap() {
        dfv::structs::DiscreteBox box = {0, 0, 10, 10}; // Example values

        // Create a sample drone path
        std::vector<dfv::structs::Node> drone_path = {
                dfv::structs::Node("drone", 1, 5.5, 5.5, {}),
                dfv::structs::Node("drone", 2, 6.5, 6.5, {})
                // Add more nodes as needed
        };

        // Call the createGrid method with some debug values
        double sparsity = 0.5; // Example density
        double box_size = 2; // Example box size
        double node_density_coefficient = 0.8; // Example coefficient

        std::vector<std::vector<dfv::structs::DiscreteBoxInfo>> box_matrix = dfv::map::createGrid(box, drone_path, sparsity, box_size, node_density_coefficient);

        double lrLatBound = box_matrix.back().back().dots.back().back().lat;
        double lrLonBound = box_matrix.back().back().dots.back().back().lon;
        std::vector<dfv::structs::Triangle> mesh_array = dfv::map::createMeshArray(&box_matrix,
                                                                                   box_matrix[0][0].dots[0][0].lat,
                                                                                   box_matrix[0][0].dots[0][0].lon,
                                                                                   lrLatBound,
                                                                                   lrLonBound);
        Mesh mesh = {};
        for (auto &triangle : mesh_array) {
            mesh.vertices.push_back({
                    {triangle.a->x, triangle.a->y, triangle.a->z},
                    {0.f, 1.f, 0.f},
                    {0.f, 0.f}
            });
            mesh.indices.push_back(mesh.indices.size());
            mesh.vertices.push_back({
                    {triangle.b->x, triangle.b->y, triangle.b->z},
                    {0.f, 1.f, 0.f},
                    {0.f, 0.f}
            });
            mesh.indices.push_back(mesh.indices.size());
            mesh.vertices.push_back({
                    {triangle.c->x, triangle.c->y, triangle.c->z},
                    {0.f, 1.f, 0.f},
                    {0.f, 0.f}
            });
            mesh.indices.push_back(mesh.indices.size());
        }

        return mesh;
    }

    Visualizer::Visualizer(const VisualizerCreateInfo &createInfo)
        : flightData(createInfo.flightData), surface(createInfo.surface), engine(),
          objectModelPath(createInfo.objectModelPath), objectScale(createInfo.objectScale) {}

    Visualizer::~Visualizer() {
        engine.cleanup();
    }

    void Visualizer::start() {
        // Load flight data
        if (!flightData.load())
            throw std::runtime_error("Failed to load flight data");

        // Start the visualization at the start time of the flight data
        time = flightData.getStartTime();

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
                         glm::rotate(attitude.x, glm::vec3{0.f, 1.f, 0.f}) * // yaw
                         glm::rotate(attitude.y, glm::vec3{1.f, 0.f, 0.f}) * // pitch
                         glm::rotate(attitude.z, glm::vec3{0.f, 0.f, 1.f}) * // roll
                         glm::scale(glm::vec3{objectScale});
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
            auto point = flightData.getPoint(time);
            auto direction = glm::normalize(glm::vec3{point.x, point.y, point.z} - engine.camera.position);

            engine.camera.orientation = {std::atan2(direction.z, direction.x),
                                         std::asin(direction.y),
                                         0.f};
            engine.camera.updateFront();
        }
    }

    void Visualizer::setCameraMode(CameraMode mode) {
        cameraMode = mode;
    }

    void Visualizer::createScene() {
        if (!is_regular_file(objectModelPath))
            throw std::runtime_error("Invalid object model file provided");

        auto firstDataPoint = flightData.getPoint(flightData.getStartTime());

        // Set the initial camera position and direction
        engine.camera.position = glm::vec3{firstDataPoint.x, firstDataPoint.y, firstDataPoint.z} +
                                 glm::vec3{0.f, 1.f, -5.f};
        engine.camera.orientation = {glm::radians(90.f), 0.f, 0.f};
        engine.camera.updateFront();
        engine.camera.up = {0.f, 1.f, 0.f};

        auto ufoMesh = engine.createMesh("ufo", objectModelPath);
        auto defaultMat = engine.getMaterial("defaultmesh");

        auto [ufo, ufoHandle] = engine.allocateRenderObject();
        *ufo = {.mesh = ufoMesh,
                .material = defaultMat,
                .transform = glm::mat4{1.f}};

        objectRenderHandle = ufoHandle;

        auto [mapObject, mapHandle] = engine.allocateRenderObject();
        *mapObject = {.mesh = engine.insertMesh("map", setupMap()),
                      .material = defaultMat,
                      .transform = glm::mat4{1.f}};
    }

    void Visualizer::update(seconds_f deltaTime) {
        time += deltaTime * droneTimeMultiplier;

        auto point = flightData.getPoint(time);
        setObjectTransform(glm::vec3{point.x, point.y, point.z},
                           glm::vec3{point.yaw, point.pitch, point.roll});

        // Update camera
        updateCamera(deltaTime, point);

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

                // Direction from the camera to the object
                glm::vec3 direction = glm::normalize(target - engine.camera.position);

                engine.camera.position += positionMask * cameraMovementSpeed * deltaTime.count();
                // Look in the direction of the object
                engine.camera.front = direction;
                engine.camera.updateOrientation();
            } break;

            case CameraMode::Follow1stPerson: {
                glm::vec3 target = {dataPoint.x,
                                    dataPoint.y + 0.3f, // Float the camera slightly above the object
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

    void Visualizer::changeTimeMultiplier(float multiplier) {
        droneTimeMultiplier = multiplier;
    }

} // namespace dfv
