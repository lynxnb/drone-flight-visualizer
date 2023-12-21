#include "vk_texture.h"

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

        mData = pixels;
        mWidth = texWidth;
        mHeight = texHeight;
        mChannels = texChannels;
    }

    void *StbImageLoader::data() const {
        return mData;
    }

    int StbImageLoader::width() const {
        return mWidth;
    }

    int StbImageLoader::height() const {
        return mHeight;
    }

    int StbImageLoader::channels() const {
        return mChannels;
    }

    StbImageLoader::~StbImageLoader() {
        stbi_image_free(mData);
    }
} // namespace dfv
