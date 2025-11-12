#pragma once
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

class SettingsNode {
  public:
    using object_t = std::unordered_map<std::string, SettingsNode>;
    using array_t = std::vector<SettingsNode>;
    using value_t =
        std::variant<std::monostate, bool, std::int64_t, double, std::string, array_t, object_t>;

    SettingsNode();
    explicit SettingsNode(bool value);
    explicit SettingsNode(std::int64_t value);
    explicit SettingsNode(double value);
    explicit SettingsNode(std::string value);
    explicit SettingsNode(array_t value);
    explicit SettingsNode(object_t value);

    template <typename T>
    T as() const {
        if constexpr (std::is_same_v<T, bool>) {
            return access<bool>("boolean");
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            if (std::holds_alternative<std::int64_t>(value_)) {
                return static_cast<T>(std::get<std::int64_t>(value_));
            }
            if (std::holds_alternative<double>(value_)) {
                return static_cast<T>(std::get<double>(value_));
            }
            raiseTypeError("integral");
        } else if constexpr (std::is_floating_point_v<T>) {
            if (std::holds_alternative<double>(value_)) {
                return static_cast<T>(std::get<double>(value_));
            }
            if (std::holds_alternative<std::int64_t>(value_)) {
                return static_cast<T>(std::get<std::int64_t>(value_));
            }
            raiseTypeError("floating point");
        } else if constexpr (std::is_same_v<T, std::string>) {
            return access<std::string>("string");
        } else if constexpr (std::is_same_v<T, array_t>) {
            return access<array_t>("array");
        } else if constexpr (std::is_same_v<T, object_t>) {
            return access<object_t>("object");
        } else if constexpr (std::is_same_v<T, SettingsNode>) {
            return *this;
        } else {
            static_assert(dependent_false<T>::value, "Unsupported SettingsNode::as<T>() instantiation");
        }
    }

    bool isNull() const { return std::holds_alternative<std::monostate>(value_); }
    bool isObject() const { return std::holds_alternative<object_t>(value_); }
    bool isArray() const { return std::holds_alternative<array_t>(value_); }

    const SettingsNode &at(const std::string &key) const;
    const SettingsNode &operator[](const std::string &key) const { return at(key); }

    const SettingsNode &at(std::size_t index) const;
    const SettingsNode &operator[](std::size_t index) const { return at(index); }

    std::size_t size() const;

  private:
    template <typename T>
    struct dependent_false : std::false_type {};

    template <typename T>
    T access(const char *expected) const {
        if (!std::holds_alternative<T>(value_)) {
            raiseTypeError(expected);
        }
        return std::get<T>(value_);
    }

    [[noreturn]] void raiseTypeError(const char *expected) const;

    value_t value_;

    friend class Settings;
};

class Settings {
  public:
    explicit Settings(const std::string &filename);

    const SettingsNode &operator()(const std::string &key) const;

    template <typename T>
    T get(const std::string &key) const {
        return (*this)(key).template as<T>();
    }

    const SettingsNode &root() const noexcept { return root_; }

  private:
    SettingsNode root_;
};

// Inline implementations ------------------------------------------------------

inline SettingsNode::SettingsNode() : value_(std::monostate{}) {}
inline SettingsNode::SettingsNode(bool value) : value_(value) {}
inline SettingsNode::SettingsNode(std::int64_t value) : value_(value) {}
inline SettingsNode::SettingsNode(double value) : value_(value) {}
inline SettingsNode::SettingsNode(std::string value) : value_(std::move(value)) {}
inline SettingsNode::SettingsNode(array_t value) : value_(std::move(value)) {}
inline SettingsNode::SettingsNode(object_t value) : value_(std::move(value)) {}

inline const SettingsNode &SettingsNode::at(const std::string &key) const {
    if (!isObject()) {
        throw std::runtime_error("Settings node is not an object");
    }
    const auto &obj = std::get<object_t>(value_);
    const auto iter = obj.find(key);
    if (iter == obj.end()) {
        throw std::runtime_error("Settings key is missing: " + key);
    }
    return iter->second;
}

inline const SettingsNode &SettingsNode::at(std::size_t index) const {
    if (!isArray()) {
        throw std::runtime_error("Settings node is not an array");
    }
    const auto &arr = std::get<array_t>(value_);
    if (index >= arr.size()) {
        throw std::runtime_error("Settings array index is out of bounds");
    }
    return arr[index];
}

inline std::size_t SettingsNode::size() const {
    if (isArray()) {
        return std::get<array_t>(value_).size();
    }
    if (isObject()) {
        return std::get<object_t>(value_).size();
    }
    return 0;
}

inline [[noreturn]] void SettingsNode::raiseTypeError(const char *expected) const {
    throw std::runtime_error(std::string("Settings node does not contain a ") + expected);
}
