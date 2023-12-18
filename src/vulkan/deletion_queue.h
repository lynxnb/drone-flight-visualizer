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

        /**
         * Pushes a function to the deletion queue
         * @param function Function to push
         */
        void pushFunction(DeletionFunc &&function);

        /**
         * Flushes the deletion queue
         */
        void flush();

        /**
         * Resets the deletion queue, without executing any deletors
         */
        void reset();

        DeletionQueue() = default;

        // Dissallow copying
        DeletionQueue(const DeletionQueue &) = delete;
        DeletionQueue &operator=(const DeletionQueue &) = delete;
        // Allow moving
        DeletionQueue(DeletionQueue &&) noexcept = default;
        DeletionQueue &operator=(DeletionQueue &&) = default;

      private:
        std::deque<DeletionFunc> deletors;
    };

} // namespace dfv
