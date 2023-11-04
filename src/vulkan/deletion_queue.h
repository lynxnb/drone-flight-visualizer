#pragma once

#include <deque>
#include <functional>

namespace dfv {

    class DeletionQueue {
      public:
        /**
         * Type of the deletion function
         */
        typedef std::function<void()> DeletionFunc;

      private:
        std::deque<DeletionFunc> deletors;

      public:
        /**
         * Pushes a function to the deletion queue
         * @param function Function to push
         */
        void pushFunction(DeletionFunc &&function);

        /**
         * Flushes the deletion queue
         */
        void flush();

        DeletionQueue() = default;

        // Dissallow copying but allow moving
        DeletionQueue(const DeletionQueue &) = delete;
        DeletionQueue(DeletionQueue &&) noexcept = default;

        // Disallow reassignment
        void operator=(const DeletionQueue &) = delete;
        void operator=(DeletionQueue &&) = delete;
    };

} // namespace dfv
