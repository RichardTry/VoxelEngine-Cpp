#pragma once

#include <vector>
#include <stdexcept>
#include <functional>
#include <glm/glm.hpp>

namespace util {

    template<class T, typename TCoord=int>
    class AreaMap3D {
    public:
        using OutCallback = std::function<void(TCoord, TCoord, TCoord, T&)>;
    private:
        TCoord offsetX = 0, offsetY = 0, offsetZ = 0;
        TCoord sizeX, sizeY, sizeZ;
        std::vector<T> firstBuffer;
        std::vector<T> secondBuffer;
        OutCallback outCallback;

        size_t valuesCount = 0;
    
        void translate(TCoord dx, TCoord dy, TCoord dz) {
            if (dx == 0 && dy == 0 && dz == 0) {
                return;
            }
            std::fill(secondBuffer.begin(), secondBuffer.end(), T{});
            
            for (TCoord z = 0; z < sizeZ; z++) {
                for (TCoord y = 0; y < sizeY; y++) {
                    for (TCoord x = 0; x < sizeX; x++) {
                        auto& value = firstBuffer[z * sizeY * sizeX + y * sizeX + x];
                        auto nx = x - dx;
                        auto ny = y - dy;
                        auto nz = z - dz;
                        if (value == T{}) {
                            continue;
                        }
                        if (nx < 0 || ny < 0 || nz < 0 || nx >= sizeX || ny >= sizeY || nz >= sizeZ) {
                            if (outCallback) {
                                outCallback(x + offsetX, y + offsetY, z + offsetZ, value);
                            }
                            valuesCount--;
                            continue;
                        }
                        secondBuffer[nz * sizeY * sizeX + ny * sizeX + nx] = value;
                    }
                }
            }
            std::swap(firstBuffer, secondBuffer);
            offsetX += dx;
            offsetY += dy;
            offsetX += dz;
        }
    public:
        AreaMap3D(TCoord width, TCoord height, TCoord length)
            : sizeX(width), sizeY(height), sizeZ(length),
              firstBuffer(width * height * length), secondBuffer(width * height * length) {
        }

        const T* getIf(TCoord x, TCoord y, TCoord z) const {
            auto lx = x - offsetX;
            auto ly = y - offsetY;
            auto lz = z - offsetZ;
            if (lx < 0 || ly < 0 || lz < 0 || lx >= sizeX || ly >= sizeY || lz >= sizeZ) {
                return nullptr;
            }
            return &firstBuffer[lz * sizeY * sizeX + ly * sizeX + lx];
        }

        T get(TCoord x, TCoord y, TCoord z) const {
            auto lx = x - offsetX;
            auto ly = y - offsetY;
            auto lz = z - offsetZ;
            if (lx < 0 || ly < 0 || lz < 0 || lx >= sizeX || ly >= sizeY || lz >= sizeZ) {
                return T{};
            }
            return firstBuffer[lz * sizeY * sizeX + ly * sizeX + lx];
        }

        T get(TCoord x, TCoord y, TCoord z, T& def) const {
            if (auto ptr = getIf(x, y, z)) {
                const auto& value = *ptr;
                if (value == T{}) {
                    return def;
                }
                return value;
            }
            return def;
        }

        bool isInside(TCoord x, TCoord y, TCoord z) const {
            auto lx = x - offsetX;
            auto ly = y - offsetY;
            auto lz = z - offsetZ;
            return !(lx < 0 || ly < 0 || lz < 0 || lx >= sizeX || ly >= sizeY || lz >= sizeZ);
        }

        const T& require(TCoord x, TCoord y, TCoord z) const {
            auto lx = x - offsetX;
            auto ly = y - offsetY;
            auto lz = z - offsetZ;
            if (lx < 0 || ly < 0 || lz < 0 || lx >= sizeX || ly >= sizeY || lz >= sizeZ) {
                throw std::invalid_argument("position is out of window");
            }
            return firstBuffer[lz * sizeY * sizeX + ly * sizeX + lx];
        }

        bool set(TCoord x, TCoord y, TCoord z, T value) {
            auto lx = x - offsetX;
            auto ly = y - offsetY;
            auto lz = z - offsetZ;
            if (lx < 0 || ly < 0 || lz < 0 || lx >= sizeX || ly >= sizeY || lz >= sizeZ) {
                return false;
            }
            auto& element = firstBuffer[lz * sizeY * sizeX + ly * sizeX + lx];
            if (value && !element) {
                valuesCount++;
            }
            if (element && !value) {
                valuesCount--;
            }
            element = std::move(value);
            return true;
        }

        void setOutCallback(const OutCallback& callback) {
            outCallback = callback;
        }

        void resize(TCoord newSizeX, TCoord newSizeY, TCoord newSizeZ) {
            if (newSizeX < sizeX) {
                TCoord delta = sizeX - newSizeX;
                translate(delta / 2, 0, 0);
                translate(-delta, 0, 0);
                translate(delta, 0, 0);
            }
            if (newSizeY < sizeY) {
                TCoord delta = sizeY - newSizeY;
                translate(0, delta / 2, 0);
                translate(0, -delta, 0);
                translate(0, delta, 0);
            }
            if (newSizeZ < sizeZ) {
                TCoord delta = sizeZ - newSizeZ;
                translate(0, 0, delta / 2);
                translate(0, 0, -delta);
                translate(0, 0, delta);
            }
            const TCoord newVolume = newSizeX * newSizeY * newSizeZ;
            std::vector<T> newFirstBuffer(newVolume);
            std::vector<T> newSecondBuffer(newVolume);
            for (TCoord z = 0; z < sizeZ && z < newSizeZ; z++) {
                for (TCoord y = 0; y < sizeY && y < newSizeY; y++) {
                    for (TCoord x = 0; x < sizeX && x < newSizeX; x++) {
                        newFirstBuffer[z * newSizeY * newSizeX + y * newSizeX + x] = firstBuffer[z * sizeY * sizeX + y * sizeX + x];
                    }
                }
            }
            sizeX = newSizeX;
            sizeY = newSizeY;
            sizeZ = newSizeZ;
            firstBuffer = std::move(newFirstBuffer);
            secondBuffer = std::move(newSecondBuffer);
        }

        void setCenter(TCoord centerX, TCoord centerY, TCoord centerZ) {
            auto deltaX = centerX - (offsetX + sizeX / 2);
            auto deltaY = centerY - (offsetY + sizeY / 2);
            auto deltaZ = centerZ - (offsetZ + sizeZ / 2);
            if (deltaX | deltaY | deltaZ) {
                translate(deltaX, deltaY, deltaZ);
            }
        }

        void clear() {
            for (TCoord z = 0; z < sizeZ; z++) {
                for (TCoord y = 0; y < sizeY; y++) {
                    for (TCoord x = 0; x < sizeX; x++) {
                        auto i = z * sizeY * sizeX + y * sizeX + x;
                        auto value = firstBuffer[i];
                        firstBuffer[i] = {};
                        if (outCallback && value != T {}) {
                            outCallback(x + offsetX, y + offsetY, z + offsetZ, value);
                        }
                    }
                }
            }
            valuesCount = 0;
        }

        TCoord getOffsetX() const {
            return offsetX;
        }

        TCoord getOffsetY() const {
            return offsetY;
        }

        TCoord getOffsetZ() const {
            return offsetZ;
        }

        TCoord getWidth() const {
            return sizeX;
        }

        TCoord getHeight() const {
            return sizeX;
        }

        TCoord getLenght() const {
            return sizeZ;
        }

        const std::vector<T>& getBuffer() const {
            return firstBuffer;
        }

        size_t count() const {
            return valuesCount;
        }

        TCoord area() const {
            return sizeX * sizeY;
        }
    };
}
