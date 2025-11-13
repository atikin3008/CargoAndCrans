#include "../include/Settings.h"

#include <nlohmann/json.hpp>

namespace {

SettingsNode fromJson(const nlohmann::json &json) {
    if (json.is_object()) {
        SettingsNode::object_t object;
        object.reserve(json.size());
        for (auto it = json.begin(); it != json.end(); ++it) {
            object.emplace(it.key(), fromJson(it.value()));
        }
        return SettingsNode(std::move(object));
    }

    if (json.is_array()) {
        SettingsNode::array_t array;
        array.reserve(json.size());
        for (const auto &value : json) {
            array.emplace_back(fromJson(value));
        }
        return SettingsNode(std::move(array));
    }

    if (json.is_boolean()) {
        return SettingsNode(json.get<bool>());
    }

    if (json.is_number_integer() || json.is_number_unsigned()) {
        return SettingsNode(json.get<std::int64_t>());
    }

    if (json.is_number_float()) {
        return SettingsNode(json.get<double>());
    }

    if (json.is_string()) {
        return SettingsNode(json.get<std::string>());
    }

    return SettingsNode();
}

} // namespace

Settings::Settings(const std::string &filename) {
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open settings file: " + filename);
    }

    nlohmann::json jsonDoc;
    try {
        input >> jsonDoc;
    } catch (const nlohmann::json::parse_error &err) {
        throw std::runtime_error("Failed to parse settings JSON: " + std::string(err.what()));
    }

    if (!jsonDoc.is_object()) {
        throw std::runtime_error("Settings JSON root must be an object: " + filename);
    }

    root_ = fromJson(jsonDoc);

    if (!root_.isObject()) {
        throw std::runtime_error("Settings root must be an object");
    }
}

const SettingsNode &Settings::operator()(const std::string &key) const {
    return root_.at(key);
}
