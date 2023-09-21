#pragma once

#include <functional>
#include <deque>

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
    };

} // namespace dfv
