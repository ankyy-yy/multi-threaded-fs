#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "common/error.hpp"

namespace mtfs::fs {

class FileCompression {
public:
    // Compression/Decompression methods
    static std::vector<uint8_t> compress(const std::string& data);
    static std::string decompress(const std::vector<uint8_t>& compressedData);
    
    // File operations
    static bool compressFile(const std::string& inputPath, const std::string& outputPath);
    static bool decompressFile(const std::string& inputPath, const std::string& outputPath);
    
    // Utility methods
    static double calculateCompressionRatio(size_t originalSize, size_t compressedSize);
    static bool isCompressed(const std::string& filePath);
    
private:
    // Simple RLE (Run-Length Encoding) compression
    static std::vector<uint8_t> rleCompress(const std::string& data);
    static std::string rleDecompress(const std::vector<uint8_t>& data);
    
    // Compression header format
    static constexpr uint32_t COMPRESSION_MAGIC = 0x4D544653; // "MTFS"
    static constexpr uint16_t COMPRESSION_VERSION = 1;
    
    struct CompressionHeader {
        uint32_t magic;
        uint16_t version;
        uint32_t originalSize;
        uint32_t compressedSize;
        uint8_t compressionType; // 0 = RLE, 1 = LZ77 (future), etc.
    };
};

// Compression statistics
struct CompressionStats {
    size_t totalFilesCompressed{0};
    size_t totalOriginalBytes{0};
    size_t totalCompressedBytes{0};
    double averageCompressionRatio{0.0};
    
    double getOverallCompressionRatio() const {
        return totalOriginalBytes > 0 ? 
            (1.0 - static_cast<double>(totalCompressedBytes) / totalOriginalBytes) * 100.0 : 0.0;
    }
    
    void addCompressionOperation(size_t originalSize, size_t compressedSize) {
        totalFilesCompressed++;
        totalOriginalBytes += originalSize;
        totalCompressedBytes += compressedSize;
        averageCompressionRatio = getOverallCompressionRatio();
    }
};

} // namespace mtfs::fs
