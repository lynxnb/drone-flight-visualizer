#pragma once

#include <filesystem>

#include "vk_types.h"

#include <span>

namespace dfv {
    struct Texture {
        VkExtent3D extent{};
        VkFormat format{VK_FORMAT_UNDEFINED};
        AllocatedImage image;
        VkImageView imageView{VK_NULL_HANDLE};
    };

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
         * @return A span of the image data.
         * @note The number of channels can be queried with the channels() method.
         */
        std::span<std::byte> data() const;

        uint32_t width() const;
        uint32_t height() const;
        uint32_t channels() const;

        ~StbImageLoader();

      private:
        std::span<std::byte> mData{};
        uint32_t mWidth{};
        uint32_t mHeight{};
        uint32_t mChannels{};
    };
} // namespace dfv
