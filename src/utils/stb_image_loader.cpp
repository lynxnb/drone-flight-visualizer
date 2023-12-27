#include "stb_image_loader.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace dfv {
    StbImageLoader::StbImageLoader(const std::filesystem::path &filename) {
        if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) {
            std::cerr << "File does not exist: " << filename << std::endl;
            return;
        }

        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(filename.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels) {
            std::cerr << "Failed to load texture file: " << filename << std::endl;
            return;
        }

        size_t size = texWidth * texHeight * 4;

        mData = {reinterpret_cast<std::byte *>(pixels), size};
        mWidth = static_cast<uint32_t>(texWidth);
        mHeight = static_cast<uint32_t>(texHeight);
        mChannels = static_cast<uint32_t>(texChannels);
    }

    StbImageLoader::StbImageLoader(const std::span<std::byte> data) {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc *>(data.data()), static_cast<int>(data.size_bytes()),
                                                &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels) {
            std::cerr << "Failed to load texture data from buffer of size 0x" << std::hex << data.size_bytes() << std::endl;
            return;
        }

        size_t size = texWidth * texHeight * 4;

        mData = {reinterpret_cast<std::byte *>(pixels), size};
        mWidth = static_cast<uint32_t>(texWidth);
        mHeight = static_cast<uint32_t>(texHeight);
        mChannels = static_cast<uint32_t>(texChannels);
    }

    std::span<std::byte> StbImageLoader::data() const {
        return mData;
    }

    uint32_t StbImageLoader::width() const {
        return mWidth;
    }

    uint32_t StbImageLoader::height() const {
        return mHeight;
    }

    uint32_t StbImageLoader::channels() const {
        return mChannels;
    }

    StbImageLoader::~StbImageLoader() {
        stbi_image_free(mData.data());
    }
} // namespace dfv
