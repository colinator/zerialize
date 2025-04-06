#pragma once

#include <zerialize/zerialize.hpp>
#include <yyjson.h>
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <stdexcept> 
#include <cstring>

namespace zerialize {

// --- Base64 Utilities ---
inline string base64Encode(span<const uint8_t> data) {
    static constexpr char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    string encoded;
    size_t i = 0;
    uint32_t value = 0;
    size_t input_size = data.size();
    size_t input_idx = 0;

    // Calculate output size to reserve memory
    encoded.reserve(((input_size + 2) / 3) * 4);

    while (input_idx < input_size) {
        value = (value << 8) + data[input_idx++];
        i += 8;
        while (i >= 6) {
            encoded += base64_chars[(value >> (i - 6)) & 0x3F];
            i -= 6;
        }
    }

    if (i > 0) {
        encoded += base64_chars[(value << (6 - i)) & 0x3F];
    }

    while (encoded.size() % 4 != 0) {
        encoded += '=';
    }

    return encoded;
}

inline vector<uint8_t> base64Decode(string_view encoded) {
    static constexpr uint8_t lookup[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, // '=' handled as break char
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    vector<uint8_t> decoded;
    uint32_t buffer = 0;
    int bits_collected = 0;
    // Reserve rough estimate
    decoded.reserve(encoded.size() * 3 / 4);

    for (char c : encoded) {
        if (c == '=') break; // Padding signifies end

        uint8_t value = lookup[static_cast<uint8_t>(c)];
        // Check for invalid characters (value 64 is placeholder for invalid/padding)
        if (value == 64) throw DeserializationError("Invalid Base64 character");

        buffer = (buffer << 6) | value;
        bits_collected += 6;

        if (bits_collected >= 8) {
            bits_collected -= 8;
            decoded.push_back(static_cast<uint8_t>((buffer >> bits_collected) & 0xFF));
        }
    }
    // No need to handle padding explicitly if decoder stops at '='

    return decoded;
}


// --- YyjsonBuffer (Deserialization Wrapper) ---
class YyjsonBuffer : public Deserializer<YyjsonBuffer> {
private:
    // Holds the yyjson document structure
    yyjson_doc* doc_ = nullptr;
    // Points to the current value node within the doc_
    yyjson_val* current_val_ = nullptr;
    // True if this instance owns the yyjson_doc and should free it
    bool owns_doc_ = true;

    // Internal helper to parse data and set doc_/current_val_
    void parse_data(const char* data, size_t len) {
        doc_ = yyjson_read(data, len, 0);
        if (!doc_) {
            throw DeserializationError("Failed to parse JSON (yyjson_read failed)");
        }
        current_val_ = yyjson_doc_get_root(doc_);
        if (!current_val_) {
             yyjson_doc_free(doc_);
             doc_ = nullptr;
             throw DeserializationError("Failed to get root value from parsed JSON");
        }
        owns_doc_ = true; // This instance now owns the parsed doc
    }

public:
    // Default constructor: represents an empty JSON object {}
    YyjsonBuffer()
        : Deserializer<YyjsonBuffer>()
    {
        parse_data("{}", 2);
    }

    // Constructor from an existing value (view, doesn't own the doc)
    YyjsonBuffer(yyjson_val* val, yyjson_doc* doc_context)
        : doc_(doc_context), // Shares the document context
          current_val_(val),
          owns_doc_(false) // Does not own the shared document
    {
        if (!doc_ || !current_val_) {
            throw std::invalid_argument("YyjsonBuffer: Null value or document context provided for view constructor.");
        }
    }

    // Constructor from external data (span, copies data to parse)
    YyjsonBuffer(span<const uint8_t> data)
        : Deserializer<YyjsonBuffer>(data)
    {
        string json_str(reinterpret_cast<const char*>(data.data()), data.size());
        parse_data(json_str.c_str(), json_str.size());
    }

    // Constructor from external data (vector move)
    YyjsonBuffer(vector<uint8_t>&& buf) 
        : Deserializer<YyjsonBuffer>(std::move(buf))
    {
        parse_data(reinterpret_cast<const char*>(buf_.data()), buf_.size());
    }

    // Constructor from external data (vector copy)
    YyjsonBuffer(const vector<uint8_t>& buf) 
        : Deserializer<YyjsonBuffer>(buf)
    {
        parse_data(reinterpret_cast<const char*>(buf_.data()), buf_.size());
    }

    // Destructor
    ~YyjsonBuffer() {
        if (owns_doc_ && doc_) {
            yyjson_doc_free(doc_);
        }
    }

    // --- Rule of 5/6 ---
    // Delete copy constructor and assignment
    YyjsonBuffer(const YyjsonBuffer&) = delete;
    YyjsonBuffer& operator=(const YyjsonBuffer&) = delete;

    // Move constructor
    // YyjsonBuffer(YyjsonBuffer&& other) noexcept
    //     : owned_buf_(std::move(other.owned_buf_)),
    //       doc_(other.doc_),
    //       current_val_(other.current_val_),
    //       owns_doc_(other.owns_doc_) {
    //     // Nullify the moved-from object's pointers to prevent double-free
    //     other.doc_ = nullptr;
    //     other.current_val_ = nullptr;
    //     other.owns_doc_ = false; // It no longer owns anything
    // }

    // // Move assignment
    // YyjsonBuffer& operator=(YyjsonBuffer&& other) noexcept {
    //     if (this != &other) {
    //         // Free existing owned resources
    //         if (owns_doc_ && doc_) {
    //             yyjson_doc_free(doc_);
    //         }

    //         // Transfer resources from 'other'
    //         owned_buf_ = std::move(other.owned_buf_);
    //         doc_ = other.doc_;
    //         current_val_ = other.current_val_;
    //         owns_doc_ = other.owns_doc_;

    //         // Nullify 'other'
    //         other.doc_ = nullptr;
    //         other.current_val_ = nullptr;
    //         other.owns_doc_ = false;
    //     }
    //     return *this;
    // }

    YyjsonBuffer(YyjsonBuffer&& other) noexcept
        : Deserializer<YyjsonBuffer>(),
          doc_(other.doc_),
          current_val_(other.current_val_),
          owns_doc_(other.owns_doc_) 
    {
        buf_ = std::move(other.buf_);
        other.doc_ = nullptr;
        other.current_val_ = nullptr;
        other.owns_doc_ = false; // It no longer owns anything
    }

    // Move assignment
    YyjsonBuffer& operator=(YyjsonBuffer&& other) noexcept 
    {
        if (this != &other) {
            // Free existing owned resources
            if (owns_doc_ && doc_) {
                yyjson_doc_free(doc_);
            }

            // Transfer resources from 'other'
            buf_ = std::move(other.buf_);
            doc_ = other.doc_;
            current_val_ = other.current_val_;
            owns_doc_ = other.owns_doc_;

            // Nullify 'other'
            other.doc_ = nullptr;
            other.current_val_ = nullptr;
            other.owns_doc_ = false;
        }
        return *this;
    }

    string to_string() const override {
        if (!doc_ || !current_val_) {
            return "YyjsonBuffer (invalid state)";
        }
        yyjson_write_flag flg = YYJSON_WRITE_PRETTY; // Use pretty print for debug
        size_t len = 0;
        char* json_str = yyjson_val_write(current_val_, flg, &len);
        string json_dump = (json_str ? string(json_str, len) : "(failed to write)");
        if (json_str) {
            free(json_str);
        }

        string buf_info = "(no owned buffer)";
        if (!buf_.empty()) {
             buf_info = std::to_string(buf_.size()) + " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data()));
        }

        return "YyjsonBuffer " + buf_info + " : " + json_dump + "\n" + debug_string(*this);
    }

    // --- Deserializable Concept Implementation ---
    bool isNull() const { return current_val_ && yyjson_is_null(current_val_); }
    // Combined check for int/uint
    bool isInt() const { return current_val_ && yyjson_is_int(current_val_); }
    bool isUInt() const { return current_val_ && yyjson_is_uint(current_val_); }
    bool isFloat() const { return current_val_ && yyjson_is_real(current_val_); }
    bool isBool() const { return current_val_ && yyjson_is_bool(current_val_); }
    bool isString() const { return current_val_ && yyjson_is_str(current_val_); }
    // Represent Blob as Base64 string in JSON
    bool isBlob() const { return isString(); }
    bool isMap() const { return current_val_ && yyjson_is_obj(current_val_); }
    bool isArray() const { return current_val_ && yyjson_is_arr(current_val_); }

    // Helper to check type and throw
    template<typename T>
    void check_type(bool (checker)(yyjson_val*), const char* type_name) const {
        if (!current_val_ || !checker(current_val_)) {
             throw DeserializationError(string("Value is not a ") + type_name);
        }
    }

    // Need to handle potential overflow if source > target
    int8_t asInt8() const { check_type<int8_t>(yyjson_is_int, "signed integer"); return static_cast<int8_t>(yyjson_get_sint(current_val_)); }
    int16_t asInt16() const { check_type<int16_t>(yyjson_is_int, "signed integer"); return static_cast<int16_t>(yyjson_get_sint(current_val_)); }
    int32_t asInt32() const { check_type<int32_t>(yyjson_is_int, "signed integer"); return static_cast<int32_t>(yyjson_get_sint(current_val_)); }
    int64_t asInt64() const { check_type<int64_t>(yyjson_is_int, "signed integer"); return yyjson_get_sint(current_val_); }

    uint8_t asUInt8() const { check_type<uint8_t>(yyjson_is_uint, "unsigned integer"); return static_cast<uint8_t>(yyjson_get_uint(current_val_)); }
    uint16_t asUInt16() const { check_type<uint16_t>(yyjson_is_uint, "unsigned integer"); return static_cast<uint16_t>(yyjson_get_uint(current_val_)); }
    uint32_t asUInt32() const { check_type<uint32_t>(yyjson_is_uint, "unsigned integer"); return static_cast<uint32_t>(yyjson_get_uint(current_val_)); }
    uint64_t asUInt64() const { check_type<uint64_t>(yyjson_is_uint, "unsigned integer"); return yyjson_get_uint(current_val_); }

    float asFloat() const { check_type<float>(yyjson_is_real, "float"); return static_cast<float>(yyjson_get_real(current_val_)); }
    double asDouble() const { check_type<double>(yyjson_is_real, "float"); return yyjson_get_real(current_val_); }

    bool asBool() const { check_type<bool>(yyjson_is_bool, "boolean"); return yyjson_get_bool(current_val_); }

    string asString() const {
        check_type<string>(yyjson_is_str, "string");
        return string(yyjson_get_str(current_val_), yyjson_get_len(current_val_));
    }

    string_view asStringView() const {
        check_type<string_view>(yyjson_is_str, "string");
        return string_view(yyjson_get_str(current_val_), yyjson_get_len(current_val_));
    }

    vector<uint8_t> asBlob() const {
        check_type<vector<uint8_t>>(yyjson_is_str, "string (for blob)");
        // Assumes Base64 encoding
        return base64Decode(asStringView());
    }

    set<string_view> mapKeys() const {
        check_type<set<string_view>>(yyjson_is_obj, "map/object");
        set<string_view> keys;
        yyjson_obj_iter iter;
        yyjson_obj_iter_init(current_val_, &iter);
        yyjson_val *key; // This will point to the key value

        // Use the correct function name here:
        while ((key = yyjson_obj_iter_next(&iter))) {
             // Key must be a string in JSON
            if (yyjson_is_str(key)) { // Good practice to double-check key is string
                keys.insert(string_view(yyjson_get_str(key), yyjson_get_len(key)));
            }
            // You don't need the value for mapKeys, but if you did, you'd get it via:
            // yyjson_val *val = yyjson_obj_iter_get_val(key);
        }
        return keys;
    }

    // operator[] for map access
    YyjsonBuffer operator[] (const string_view key) const {
        check_type<YyjsonBuffer>(yyjson_is_obj, "map/object");
        // yyjson_obj_get requires null-terminated key
        string key_str(key);
        yyjson_val* val = yyjson_obj_get(current_val_, key_str.c_str());
        if (!val) {
            // Key not found - return a buffer representing null? Or throw?
            // Let's be consistent with nlohmann (which creates null on access)
            // but here we are const, so we should throw or return a null view.
            // Returning a view of a non-existent value is tricky. Let's throw.
            throw DeserializationError("Key not found in JSON object: " + string(key));
        }
        // Return a *view* into the same document
        return YyjsonBuffer(val, doc_);
    }

    size_t arraySize() const {
        check_type<size_t>(yyjson_is_arr, "array");
        return yyjson_arr_size(current_val_);
    }

    // operator[] for array access
    YyjsonBuffer operator[] (size_t index) const {
        check_type<YyjsonBuffer>(yyjson_is_arr, "array");
        yyjson_val* val = yyjson_arr_get(current_val_, index);
        if (!val) {
             // Index out of bounds
             throw DeserializationError("Array index out of bounds: " + std::to_string(index));
        }
         // Return a *view* into the same document
        return YyjsonBuffer(val, doc_);
    }
};


// --- YyjsonRootSerializer (Serialization Setup) ---
class YyjsonRootSerializer {
public:
    yyjson_mut_doc* doc;
    yyjson_mut_val* root_val; // The single root value being built

    YyjsonRootSerializer() : doc(yyjson_mut_doc_new(nullptr)), root_val(nullptr) {
        if (!doc) {
            throw SerializationError("Failed to create yyjson mutable document");
        }
        // Initialize root_val to null, it will be replaced by the first serialize call
        root_val = yyjson_mut_null(doc);
    }

    ~YyjsonRootSerializer() {
        if (doc) {
            yyjson_mut_doc_free(doc);
        }
    }

    // No copying/moving needed for this temporary builder
    YyjsonRootSerializer(const YyjsonRootSerializer&) = delete;
    YyjsonRootSerializer& operator=(const YyjsonRootSerializer&) = delete;

    ZBuffer finish() {

        if (!doc) {
             throw SerializationError("Cannot finish serialization, document is invalid.");
        }

        // Ensure root is set
        if (!root_val) {
            root_val = yyjson_mut_null(doc);
        }
        yyjson_mut_doc_set_root(doc, root_val);

        // const char *json = yyjson_mut_write(doc, 0, NULL);
        // size_t size = strlen(json);
        // return YyjsonBuffer(std::move(json), size);

        // --- Optimization Step 1: Create immutable doc directly ---
        yyjson_doc* immutable_doc = yyjson_mut_doc_imut_copy(doc, nullptr); // Pass allocator if needed
        if (!immutable_doc) {
            // Maybe grab error code from mut_doc if available?
            throw SerializationError("Failed to copy mutable doc to immutable (yyjson_mut_doc_imut_copy failed)");
        }

        // --- Optimization Step 2: Write *once* to get the buffer ---
        // We can write either the mutable or the immutable doc here.
        // Writing the immutable might be slightly safer if the mutable doc gets modified later,
        // but performance should be similar. Let's write the immutable one.
        yyjson_write_flag flg = YYJSON_WRITE_NOFLAG; // No pretty print for final buffer
        size_t len = 0;
        char* json_str_ptr = yyjson_write(immutable_doc, flg, &len);
        if (!json_str_ptr) {
            yyjson_doc_free(immutable_doc); // Clean up immutable doc on error
            throw SerializationError("Failed to write immutable JSON document (yyjson_write failed)");
        }

        // Copy the result into our vector buffer
        vector<uint8_t> final_buf(json_str_ptr, json_str_ptr + len);
        free(json_str_ptr); // Free the string allocated by yyjson

        // --- Step 3: Return buffer owning the *already created* immutable doc ---
        // No re-parsing needed!
        return ZBuffer(std::move(final_buf)); //, immutable_doc); // Pass ownership
    }
};


// --- YyjsonSerializer (Serialization Logic) ---
class YyjsonSerializer: public Serializer<YyjsonSerializer> {
private:
    yyjson_mut_doc* doc_;           // The document context
    yyjson_mut_val* parent_obj_;    // Pointer to parent if current context is an object's value
    yyjson_mut_val* parent_arr_;    // Pointer to parent if current context is an array element
    yyjson_mut_val* key_for_obj_;   // Pointer to the key if adding to an object (owned by parent's add call)
    YyjsonRootSerializer* root_ctx_; // Pointer to root if this is the top-level serializer

    // Internal helper to add the final value based on context
    void set_value(yyjson_mut_val* val_to_set) {
        if (!val_to_set) return; // Should not happen

        if (parent_obj_ && key_for_obj_) {
            // We were created via serializerForKey, add to the parent object
            yyjson_mut_obj_add(parent_obj_, key_for_obj_, val_to_set);
            // Key is consumed by obj_add, nullify it here? No, this serializer instance is done.
        } else if (parent_arr_) {
            // We were created for a vector element, append to the parent array
            yyjson_mut_arr_append(parent_arr_, val_to_set);
        } else if (root_ctx_) {
            // We are the root serializer, set the root value
            root_ctx_->root_val = val_to_set;
        } else {
            // Should not happen - indicates an invalid serializer state
            throw SerializationError("YyjsonSerializer: Invalid context for setting value.");
        }
    }

public:
    using Serializer<YyjsonSerializer>::serialize; // Make base overloads visible

    // Constructor for the root serializer
    YyjsonSerializer(YyjsonRootSerializer& rs)
        : Serializer<YyjsonSerializer>(),
          doc_(rs.doc),
          parent_obj_(nullptr),
          parent_arr_(nullptr),
          key_for_obj_(nullptr),
          root_ctx_(&rs) {}

    // Internal constructor for nested serializers
    YyjsonSerializer(yyjson_mut_doc* doc, yyjson_mut_val* parent_obj, yyjson_mut_val* parent_arr, yyjson_mut_val* key_for_obj)
        : Serializer<YyjsonSerializer>(),
          doc_(doc),
          parent_obj_(parent_obj),
          parent_arr_(parent_arr),
          key_for_obj_(key_for_obj),
          root_ctx_(nullptr) // Not the root
        {}

    // --- Primitive Serialization ---
    void serialize(std::nullptr_t) { set_value(yyjson_mut_null(doc_)); }
    void serialize(bool val) { set_value(yyjson_mut_bool(doc_, val)); }
    void serialize(double val) { set_value(yyjson_mut_real(doc_, val)); }
    void serialize(float val) { set_value(yyjson_mut_real(doc_, static_cast<double>(val))); } // Use double for real
    void serialize(int64_t val) { set_value(yyjson_mut_sint(doc_, val)); }
    void serialize(int32_t val) { serialize(static_cast<int64_t>(val)); }
    void serialize(int16_t val) { serialize(static_cast<int64_t>(val)); }
    void serialize(int8_t val) { serialize(static_cast<int64_t>(val)); }
    void serialize(uint64_t val) { set_value(yyjson_mut_uint(doc_, val)); }
    void serialize(uint32_t val) { serialize(static_cast<uint64_t>(val)); }
    void serialize(uint16_t val) { serialize(static_cast<uint64_t>(val)); }
    void serialize(uint8_t val) { serialize(static_cast<uint64_t>(val)); }

    // --- String/Blob Serialization ---
    void serialize(const char* val) {
        if (!val) { set_value(yyjson_mut_null(doc_)); }
        else { set_value(yyjson_mut_strcpy(doc_, val)); }
    }
    void serialize(const string& val) {
        set_value(yyjson_mut_strncpy(doc_, val.data(), val.size()));
    }
     void serialize(string_view val) {
        set_value(yyjson_mut_strncpy(doc_, val.data(), val.size()));
    }
    // Specialization for string_view convertible (e.g. from initializer list)
    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&& val) {
        serialize(string_view(std::forward<T>(val)));
    }

    void serialize(const span<const uint8_t>& val) {
        // Encode blob as Base64 string
        std::string s = base64Encode(val);
        set_value(yyjson_mut_strncpy(doc_, s.data(), s.size()));
    }

    // --- Key/Value Serialization (for std::any maps) ---
    // Note: This is less efficient than serializeMap + serializerForKey
    void serialize(const string& key, const any& value) {
         if (!parent_obj_) {
             throw SerializationError("Cannot serialize key/value pair outside of an object context.");
         }
         // Create the key value (yyjson requires keys to be added first)
         yyjson_mut_val* key_val = yyjson_mut_strncpy(doc_, key.data(), key.size());
         // Create a placeholder serializer for the value
         YyjsonSerializer value_serializer(doc_, parent_obj_, nullptr, key_val);
         // Serialize the std::any value using the placeholder serializer
         // This will eventually call set_value(actual_value) inside value_serializer,
         // which will add the key_val and the actual_value to parent_obj_.
         value_serializer.serializeAny(value);
    }

    // --- Composite Serialization ---

    // Returns a serializer configured to add its result to the current object context with the given key.
    YyjsonSerializer serializerForKey(const string_view& key) {
        if (parent_arr_ || root_ctx_) { // Cannot add key to root or array
            throw SerializationError("serializerForKey can only be used within serializeMap context.");
        }
        if (!parent_obj_) { // Should be inside serializeMap
             throw SerializationError("YyjsonSerializer: Invalid context for serializerForKey.");
        }
         // Create the key value - yyjson owns this memory now
         yyjson_mut_val* key_val = yyjson_mut_strncpy(doc_, key.data(), key.size());
         // Return a new serializer instance configured to add using this key to our parent object
         return YyjsonSerializer(doc_, parent_obj_, nullptr, key_val);
    }

    // Handles functions returned by zmap/zvec etc.
    template <typename F>
    requires InvocableSerializer<F, YyjsonSerializer&>
    void serialize(F&& f) {
        // The function f is expected to call serializeMap or serializeVector
        // on the serializer passed to it (*this*).
        std::forward<F>(f)(*this);
    }

    // Serialize a map structure
    template <typename F>
    requires InvocableSerializer<F, YyjsonSerializer&>
    void serializeMap(F&& f) {
        // Create the mutable object node
        yyjson_mut_val* obj = yyjson_mut_obj(doc_);
        // Add this new object to the parent context (root, parent obj, or parent arr)
        set_value(obj);
        // Create a serializer whose context *is* this new object
        YyjsonSerializer map_content_serializer(doc_, obj, nullptr, nullptr);
        // Call the user's function to populate the object
        std::forward<F>(f)(map_content_serializer);
    }

    // Serialize a vector structure
    template <typename F>
    requires InvocableSerializer<F, YyjsonSerializer&>
    void serializeVector(F&& f) {
        // Create the mutable array node
        yyjson_mut_val* arr = yyjson_mut_arr(doc_);
        // Add this new array to the parent context
        set_value(arr);
        // Create a serializer whose context *is* this new array
        YyjsonSerializer vector_content_serializer(doc_, nullptr, arr, nullptr);
        // Call the user's function to populate the array
        // The function f will call serialize(...) on vector_content_serializer,
        // which will append elements to the 'arr' via set_value().
        std::forward<F>(f)(vector_content_serializer);
    }

    // Override for std::map<string, any> (less efficient than serializeMap)
    void serialize(const map<string, any>& m) {
        serializeMap([&m](YyjsonSerializer& s) {
            for (const auto& [key, value] : m) {
                // Use serializerForKey to get the context, then serializeAny
                s.serializerForKey(key).serializeAny(value);
            }
        });
    }

    // Override for std::vector<any> (less efficient than serializeVector)
    void serialize(const vector<any>& l) {
         serializeVector([&l](YyjsonSerializer& s) {
            for (const auto& v : l) {
                // Directly serialize elements using the vector context serializer
                s.serializeAny(v);
            }
        });
    }
    
};


// --- Yyjson Class (Ties everything together) ---
class Yyjson {
public:
    using BufferType = YyjsonBuffer;
    using Serializer = YyjsonSerializer;
    using RootSerializer = YyjsonRootSerializer;
    // Provide the expected SerializingFunction type definition
    using SerializingFunction = std::function<void(Serializer&)>;

    static inline constexpr const char* Name = "YYJSON";
};

} // namespace zerialize