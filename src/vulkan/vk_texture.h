#pragma once

#include <filesystem>

namespace dfv {
    /**
     * @brief The class handling the loading of an image using stb_image.
     */
    class StbImageLoader {
      public:
        /**
         * @brief Constructs a new StbImageLoader object and loads the image from a file.
         * @param filename The path to the image file.
         */
        explicit StbImageLoader(const std::filesystem::path &filename);

        /**
         * @return A pointer to the pixel data, in 32-bit per channel format.
         * @note The number of channels can be queried with the channels() method.
         */
        void *data() const;

        int width() const;
        int height() const;
        int channels() const;

        ~StbImageLoader();

      private:
        void *mData{nullptr};
        int mWidth{};
        int mHeight{};
        int mChannels{};
    };
} // namespace dfv
