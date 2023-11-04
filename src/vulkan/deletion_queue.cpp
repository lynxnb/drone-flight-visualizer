#include "deletion_queue.h"

#include <ranges>

namespace dfv {

    void DeletionQueue::pushFunction(std::function<void()> &&function) {
        deletors.push_back(function);
    }

    void DeletionQueue::flush() {
        // Reverse iterate the deletion queue to execute all the functions
        for (auto &deletor : std::ranges::reverse_view(deletors))
            deletor();
        deletors.clear();
    }

} // namespace dfv
