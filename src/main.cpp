#include <iostream>
#include <thread>

#include "context.h"
#include "renderer/renderer.h"

using namespace dfv;

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {
    std::cout << "drone_flight_visualizer" << std::endl;

    Context context;

    startRendererThread(context);

    context.waitExit();
    return 0;
}
