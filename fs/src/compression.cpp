#include "fs/compression.hpp"
#include "common/logger.hpp"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace mtfs::fs {

using namespace mtfs::common;

// Simple RLE compression implementation
std::vector<uint8_t> FileCompression::rleCompress(const std::string& data) {
    std::vector<uint8_t> compressed;
    
    if (data.empty()) {
        return compressed;
    }
    
    for (size_t i = 0; i < data.size(); ) {
        char currentChar = data[i];
        uint8_t count = 1;
        
        // Count consecutive identical characters (max 255)
        while (i + count < data.size() && data[i + count] == currentChar && count < 255) {
            count++;
        }
        
        // Store count and character
        compressed.push_back(count);
        compressed.push_back(static_cast<uint8_t>(currentChar));
        
        i += count;
    }
    
    return compressed;
}

std::string FileCompression::rleDecompress(const std::vector<uint8_t>& data) {
    std::string decompressed;
    
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 < data.size()) {
            uint8_t count = data[i];
            char character = static_cast<char>(data[i + 1]);
            
            decompressed.append(count, character);
        }
    }
    
    return decompressed;
}

std::vector<uint8_t> FileCompression::compress(const std::string& data) {
    try {
        LOG_INFO("Compressing data of size: " + std::to_string(data.size()) + " bytes");
        
        // Compress using RLE
        auto rleCompressed = rleCompress(data);
        
        // Create header
        CompressionHeader header;
        header.magic = COMPRESSION_MAGIC;
        header.version = COMPRESSION_VERSION;
        header.originalSize = static_cast<uint32_t>(data.size());
        header.compressedSize = static_cast<uint32_t>(rleCompressed.size());
        header.compressionType = 0; // RLE
        
        // Combine header and compressed data
        std::vector<uint8_t> result;
        result.resize(sizeof(CompressionHeader) + rleCompressed.size());
        
        // Copy header
        std::memcpy(result.data(), &header, sizeof(CompressionHeader));
        
        // Copy compressed data
        std::memcpy(result.data() + sizeof(CompressionHeader), 
                   rleCompressed.data(), rleCompressed.size());
        
        double ratio = calculateCompressionRatio(data.size(), result.size());
        LOG_INFO("Compression completed. Ratio: " + std::to_string(ratio) + "%");
        
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Compression failed: " + std::string(e.what()));
        throw;
    }
}

std::string FileCompression::decompress(const std::vector<uint8_t>& compressedData) {
    try {
        if (compressedData.size() < sizeof(CompressionHeader)) {
            throw std::runtime_error("Invalid compressed data: too small");
        }
        
        // Read header
        CompressionHeader header;
        std::memcpy(&header, compressedData.data(), sizeof(CompressionHeader));
        
        // Validate header
        if (header.magic != COMPRESSION_MAGIC) {
            throw std::runtime_error("Invalid compression magic number");
        }
        
        if (header.version != COMPRESSION_VERSION) {
            throw std::runtime_error("Unsupported compression version");
        }
        
        LOG_INFO("Decompressing data. Original size: " + std::to_string(header.originalSize) + " bytes");
        
        // Extract compressed data
        std::vector<uint8_t> compressedPayload(
            compressedData.begin() + sizeof(CompressionHeader),
            compressedData.end()
        );
        
        // Decompress based on type
        std::string result;
        switch (header.compressionType) {
            case 0: // RLE
                result = rleDecompress(compressedPayload);
                break;
            default:
                throw std::runtime_error("Unsupported compression type: " + 
                                       std::to_string(header.compressionType));
        }
        
        // Validate decompressed size
        if (result.size() != header.originalSize) {
            throw std::runtime_error("Decompressed size mismatch");
        }
        
        LOG_INFO("Decompression completed successfully");
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Decompression failed: " + std::string(e.what()));
        throw;
    }
}

bool FileCompression::compressFile(const std::string& inputPath, const std::string& outputPath) {
    try {
        LOG_INFO("Compressing file: " + inputPath + " -> " + outputPath);
        
        // Read input file
        std::ifstream inputFile(inputPath, std::ios::binary);
        if (!inputFile) {
            throw std::runtime_error("Cannot open input file: " + inputPath);
        }
        
        std::string fileContent((std::istreambuf_iterator<char>(inputFile)),
                               std::istreambuf_iterator<char>());
        inputFile.close();
        
        // Compress data
        auto compressedData = compress(fileContent);
        
        // Write compressed file
        std::ofstream outputFile(outputPath, std::ios::binary);
        if (!outputFile) {
            throw std::runtime_error("Cannot create output file: " + outputPath);
        }
        
        outputFile.write(reinterpret_cast<const char*>(compressedData.data()), 
                        compressedData.size());
        outputFile.close();
        
        LOG_INFO("File compression completed: " + inputPath + " -> " + outputPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("File compression failed: " + std::string(e.what()));
        return false;
    }
}

bool FileCompression::decompressFile(const std::string& inputPath, const std::string& outputPath) {
    try {
        LOG_INFO("Decompressing file: " + inputPath + " -> " + outputPath);
        
        // Read compressed file
        std::ifstream inputFile(inputPath, std::ios::binary);
        if (!inputFile) {
            throw std::runtime_error("Cannot open compressed file: " + inputPath);
        }
        
        std::vector<uint8_t> compressedData((std::istreambuf_iterator<char>(inputFile)),
                                           std::istreambuf_iterator<char>());
        inputFile.close();
        
        // Decompress data
        std::string decompressedContent = decompress(compressedData);
        
        // Write decompressed file
        std::ofstream outputFile(outputPath);
        if (!outputFile) {
            throw std::runtime_error("Cannot create output file: " + outputPath);
        }
        
        outputFile.write(decompressedContent.c_str(), decompressedContent.size());
        outputFile.close();
        
        LOG_INFO("File decompression completed: " + inputPath + " -> " + outputPath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("File decompression failed: " + std::string(e.what()));
        return false;
    }
}

double FileCompression::calculateCompressionRatio(size_t originalSize, size_t compressedSize) {
    if (originalSize == 0) return 0.0;
    return (1.0 - static_cast<double>(compressedSize) / originalSize) * 100.0;
}

bool FileCompression::isCompressed(const std::string& filePath) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return false;
        }
        
        uint32_t magic;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        
        return magic == COMPRESSION_MAGIC;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace mtfs::fs
