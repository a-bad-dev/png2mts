#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <string>
#include <fstream>
#include <regex>
#include <cstdint>
#include <zlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Vec4 {
    float x, y, z, w;

    // by default w is 1 to prevent accidentally dividing by 0 when treating this as a 3d vector
    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float x, float y, float z) : x(x), y(y), z(z), w(1) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    std::string to_string() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) +")";
    }

    Vec4 operator+(const Vec4 &other) const {
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vec4 operator-(const Vec4 &other) const {
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    Vec4 operator*(const float &other) const {
        return Vec4(x * other, y * other, z * other, w * other);
    }

    Vec4 operator/(const float &other) const {
        return Vec4(x / other, y / other, z / other, w / other);
    }
    
    float length() const {
        return sqrt(x * x + y * y + z * z + w * w);
    }
};

struct ImageData {
    int width;
    int height;
    int channels;
    unsigned char *data;
};

enum class Axis {
    NONE,
    XY,
    XZ,
    YZ,
};

struct Orientation {
    Axis axis;
    int  angle;
};

// =============================== Image Encoding ===============================
using NodeColorPair = std::pair<std::string, Vec4>;

void write_u8(std::vector<uint8_t> &buf, uint8_t val) { buf.push_back(val); }
void write_u16(std::vector<uint8_t> &buf, uint16_t val) {
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back(val & 0xFF);
}

class Encoder {
private:
    const std::vector<NodeColorPair> palette;

public:
    Encoder(const std::vector<NodeColorPair> &palette) : palette(palette) { };

    std::string get_closest_node_name(Vec4 color) const {
        std::string closest_node_name = "air";
        float closest_distance = std::numeric_limits<float>::max();

        if (color.w == 0.0)
            color = color * 0.0f;

        for (const auto &node : palette) {
            Vec4 diff = color - node.second;
            //float distance = (diff.x * diff.x * 0.30f) + 
            //                 (diff.y * diff.y * 0.59f) + 
            //                 (diff.z * diff.z * 0.11f);
            float distance = diff.length();
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_node_name = node.first;
            }

            if (closest_distance < 0.001)
                break;
        }
        return closest_node_name;
    }

    void encode_image(const ImageData &image, const std::string &output_path, const Orientation &orientation) {
        // Orientation
        Vec4 dimensions;

        switch (orientation.axis) {
            case Axis::XY: {
                dimensions.x = image.width;
                dimensions.y = image.height;
                dimensions.z = 1;
                break;
            }

            case Axis::XZ: {
                dimensions.x = image.width;
                dimensions.y = 1;
                dimensions.z = image.height;
                break;
            }

            case Axis::YZ: {
                dimensions.x = 1;
                dimensions.y = image.height;
                dimensions.z = image.width;
                break;
            }

            default: {
                std::cerr << "Invalid Orientation." << std::endl;
                return;
            }
        }

        // Encode the image
        int node_count = image.width * image.height;

        std::vector<int> image_nodes;
        image_nodes.reserve(node_count);

        std::vector<std::string>             used_names;
        std::unordered_map<std::string, int> used_map;

        std::cout << "Encoding image... [0/" << node_count << "]";

        const int w = image.width;
        const int h = image.height;
        for (int y = 0; y < image.height; y++)
        for (int x = 0; x < image.width; x++)
        {
            int i;

            // I apologize for the lack of readability
            switch (orientation.angle) {
                case 90:  i = (x * w + (h - y)); break;
                case 180: i = ((h - y) * h + (w - x)); break;
                case 270: i = ((w - x) * w + y); break;
                default:  i = (y * h + x); break;
            }
            i *= 4;

            int r = image.data[i];
            int g = image.data[i + 1];
            int b = image.data[i + 2];
            int a = image.data[i + 3];
            Vec4 color(r, g, b, a);

            std::string node = get_closest_node_name(color / 255.0f);

            auto iterator = used_map.find(node);
            if (iterator == used_map.end()) {
                int next = used_names.size();
                used_names.push_back(node);
                used_map[node] = next;

                iterator = used_map.find(node);
            }

            image_nodes.push_back(iterator->second);

            std::cout << "\rEncoding image... [" << i << "/" << node_count << "]" << std::flush;
        }

        std::cout << std::endl << "Encoding Schematic..." << std::endl;

        std::unordered_map<std::string, int> node_to_index;

        for (int i = 0; i < palette.size(); i++) {
            node_to_index.insert({palette[i].first, i});
        }

        // Encode/Compress the nodes
        std::vector<uint8_t> uncompressed;
        uncompressed.reserve(node_count * 4);

        for (int i = 0; i < image_nodes.size(); i++) {
            write_u16(uncompressed, image_nodes[i]);
        }

        // param1/2
        for (int i = 0; i < node_count; i++) uncompressed.push_back(0xFF);
        for (int i = 0; i < node_count; i++) uncompressed.push_back(0x00);

        uLongf compressed_size = compressBound(uncompressed.size());
        std::vector<uint8_t> compressed(compressed_size);

        if (compress(compressed.data(), &compressed_size, uncompressed.data(), uncompressed.size()) != Z_OK) {
            std::cerr << "Compression failed." << std::endl;
            return;
        }
        compressed.resize(compressed_size);

        // Encode the schematic
        std::vector<uint8_t> buf;

        // Header
        buf.push_back('M');
        buf.push_back('T');
        buf.push_back('S');
        buf.push_back('M');
        write_u8(buf, 0);

        // Version
        write_u8(buf, 4);

        // Schematic dimensions
        write_u16(buf, dimensions.x);
        write_u16(buf, dimensions.y);
        write_u16(buf, dimensions.z);

        for (int i = 0; i < dimensions.y; i++) buf.push_back(0xFF);

        // Node list
        write_u16(buf, used_names.size());

        for (const auto& name : used_names) {
            write_u16(buf, name.size());
            for (char c : name) write_u8(buf, c);
        }

        // Nodes
        buf.insert(buf.end(), compressed.begin(), compressed.end());

        std::ofstream out(output_path.c_str(), std::ios::binary);
        out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
        out.close();

        std::cout << "Done" << std::endl;
    }
};


// =============================== Command Line Arguments ===============================
struct CommandArgs {
    std::string palette_path = "palettes/default.txt";
    std::string image_path;
    std::string output_path;
    Orientation orientation = {
        Axis::XZ,
        0
    };
    bool flip_y = false;

    char **argv;
    int argc;

    std::string next_arg(int &i) const {
        if (i + 1 < argc) {
            return std::string(argv[i + 1]);
        }

        return "";
    }

    CommandArgs(int argc, char **argv) : argc(argc), argv(argv) {
        bool s_orientation = false;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--palette") {
                palette_path = next_arg(i);

            } else if (arg == "--image") {
                image_path = next_arg(i);

            } else if (arg == "--output") {
                output_path = next_arg(i);

            } else if (arg == "--axis") {
                if      (next_arg(i) == "XY") orientation.axis = Axis::XY;
                else if (next_arg(i) == "XZ") orientation.axis = Axis::XZ;
                else if (next_arg(i) == "YZ") orientation.axis = Axis::YZ;
                else                          orientation.axis = Axis::NONE;

            } else if (arg == "--rotate") {
                // im too lazy to make a better rotation solver
                if      (next_arg(i) == "0")   orientation.angle = 0;
                else if (next_arg(i) == "90")  orientation.angle = 90;
                else if (next_arg(i) == "180") orientation.angle = 180;
                else if (next_arg(i) == "270") orientation.angle = 270;
                else                           orientation.angle = -1;

            } else if (arg == "--flip-y") {
                flip_y = true;

            } else if (arg == "--help") {
                print_help();
                exit(0);
            }
        }

        if (output_path.empty())
            output_path = image_path + ".mts";
    }

    bool is_valid() const {
        return !palette_path.empty() && !image_path.empty() && 
               !output_path.empty() && orientation.axis != Axis::NONE &&
               orientation.angle != -1;
    }

    void print_help() const {
        std::cout << "Usage: " << argv[0] << " [OPTIONS] --image <image_path>" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --palette <palette_path>  Path to the palette file" << std::endl;
        std::cout << "  --image <image_path>      Path to the image file" << std::endl;
        std::cout << "  --output <output_path>    Path to the output file (If not provided it defaults to `<image_path>.mts`)" << std::endl;
        std::cout << "  --axis <XY|XZ|ZY>         Reorient the schematic to one of the listed axis (If not provided it defaults to `XZ`)" << std::endl;
        std::cout << "  --rotate <0|90|180|270>   Rotate the schematic by the listed angles (If not provided it defaults to `0`)" << std::endl;
        std::cout << "  --help                    Print this help message" << std::endl;
    }
};

int main(int argc, char **argv) {
    // command line arguments
    CommandArgs args(argc, argv);

    if (!args.is_valid()) {
        args.print_help();
        return -1;
    }

    // Palette Loading
    std::vector<NodeColorPair> palette;

    std::ifstream palette_file(args.palette_path.c_str());
    std::string line;
    while (std::getline(palette_file, line)) {
        std::regex regex("^([\\S^#]+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)[\\s+]?(\\d+)?$");
        std::smatch match;
        if (std::regex_match(line, match, regex)) {
            std::string name = match[1].str();
            float r = std::stof(match[2].str()) / 255;
            float g = std::stof(match[3].str()) / 255;
            float b = std::stof(match[4].str()) / 255;
            float a = match[5].str().empty() ? 1.0f : std::stof(match[5].str()) / 255;
            Vec4 color(r, g, b, a);
            palette.push_back({name, color});
        }
    }
    palette_file.close();
    
    // Image Loading
    int width, height, channels;
    stbi_set_flip_vertically_on_load(!args.flip_y);
    unsigned char *data = stbi_load(args.image_path.c_str(), &width, &height,
                                    &channels, 4);
    if (!data) {
        std::cerr << "Error in loading the image" << std::endl;
        std::cerr << stbi_failure_reason() << std::endl;
        return -1;
    }

    ImageData image = {width, height, channels, data};
    Encoder encoder(palette);

    encoder.encode_image(image, args.output_path, args.orientation);

    stbi_image_free(data);

    return 0;
}