#include "deletion_queue.h"

namespace dfv {

    void DeletionQueue::pushFunction(std::function<void()> &&function) {
        deletors.push_back(function);
    }

    void DeletionQueue::flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            (*it)(); // call functors
        deletors.clear();
    }

} // namespace dfv
