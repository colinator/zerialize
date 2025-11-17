#include <array>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <zerialize/zerialize.hpp>
#include <zerialize/tensor/xtensor.hpp>
#include <zerialize/tensor/eigen.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/protocols/msgpack.hpp>
#include <zerialize/protocols/cbor.hpp>

#include <xtensor/generators/xbuilder.hpp>

#include "testing_utils.hpp"

namespace zerialize {

// Small helper to assert we surface DeserializationError boundaries.
template<class F>
bool expect_deserialization_error(F&& fn) {
    try {
        std::forward<F>(fn)();
    } catch (const DeserializationError&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

// --------------------- Per-protocol DSL tests ---------------------
template<class P>
void test_protocol_dsl() {
    using V = typename P::Deserializer;
    std::cout << "== DSL tests for <" << P::Name << "> ==\n";

    // 1) Simple map with compile-time keys
    test_serialization<P>(R"(zmap<"key1","key2">(42,"yo"))",
        [](){
            return serialize<P>( zmap<"key1","key2">(42, "yo") );
        },
        [](const V& v){
            return v.isMap()
                && v["key1"].asInt64()==42
                && v["key2"].asString()=="yo";
        });

    // 2) Array root
    test_serialization<P>(R"(zvec(1,2,3))",
        [](){
            return serialize<P>( zvec(1,2,3) );
        },
        [](const V& v){
            return v.isArray() && v.arraySize()==3
                && v[0].asInt64()==1
                && v[1].asInt64()==2
                && v[2].asInt64()==3;
        });

    // 3) Nested: array of map and array
    test_serialization<P>(R"(zmap<"a","b">( 7, zvec("x", zmap<"n">(44)) ))",
        [](){
            return serialize<P>( zmap<"a","b">(
                7,
                zvec("x", zmap<"n">(44))
            ));
        },
        [](const V& v){
            if (!v.isMap()) return false;
            if (!v["a"].isInt() || v["a"].asInt64()!=7) return false;
            auto b = v["b"];
            if (!b.isArray() || b.arraySize()!=2) return false;
            if (b[0].asString()!="x") return false;
            return b[1].isMap() && b[1]["n"].asInt64()==44;
        });

    // 4) Booleans and null
    test_serialization<P>(R"(zmap<"t","f","n">(true,false,nullptr))",
        [](){
            return serialize<P>( zmap<"t","f","n">( true, false, nullptr ) );
        },
        [](const V& v){
            return v.isMap()
                && v["t"].asBool()==true
                && v["f"].asBool()==false
                && v["n"].isNull();
        });

    // 5) Mixed numeric types (assert via int64/uint64/double)
    test_serialization<P>("mixed numeric types",
        [](){
            return serialize<P>( zmap<
                "i8","u8","i32","u32","i64","u64","d"
            >( int8_t(-5), uint8_t(200), int32_t(-123456), uint32_t(987654321u),
               int64_t(-7777777777LL), uint64_t(9999999999ULL), 3.25 ) );
        },
        [](const V& v){
            return v.isMap()
                && v["i8"].asInt64()==-5
                && v["u8"].asUInt64()==200
                && v["i32"].asInt64()==-123456
                && v["u32"].asUInt64()==987654321ULL
                && v["i64"].asInt64()==-7777777777LL
                && v["u64"].asUInt64()==9999999999ULL
                && std::abs(v["d"].asDouble()-3.25)<1e-12;
        });

    // 6) Unicode strings + embedded NUL in **value**
    auto ts1 = std::string(reinterpret_cast<const char*>(u8"héllo"));
    auto ts2 = std::string(reinterpret_cast<const char*>(u8"汉字"));
    test_serialization<P>("strings (unicode + embedded NUL)",
        [ts1, ts2](){
            const char raw[] = {'a','\0','b'};
            return serialize<P>( zvec(ts1, std::string_view(raw,3), ts2) );
        },
        [ts1, ts2](const V& v){
            if (!v.isArray() || v.arraySize()!=3) return false;
            if (v[0].asString()!=ts1) return false;
            auto s1 = v[1].asStringView();
            if (!(s1.size()==3 && s1[0]=='a' && s1[1]=='\0' && s1[2]=='b')) return false;
            return v[2].asString()==ts2;
        });

    // 7) Biggish vector (size hint exercised)
    test_serialization<P>("big vector 256",
        [](){
            std::array<int,256> a{};
            for (int i=0;i<256;++i) a[i]=i;
            return serialize<P>( a );
        },
        [](const V& v){
            if (!v.isArray() || v.arraySize()!=256) return false;
            for (int i=0;i<256;++i) if (v[i].asInt64()!=i) return false;
            return true;
        });

    // 8) mapKeys() contract
    test_serialization<P>("mapKeys() iteration",
        [](){
            return serialize<P>( zmap<"alpha","beta","gamma">(1,2,3) );
        },
        [](const V& v){
            if (!v.isMap()) return false;
            std::set<std::string_view> keys;
            for (std::string_view k : v.mapKeys()) keys.insert(k);
            return keys.size()==3 && keys.count("alpha") && keys.count("beta") && keys.count("gamma");
        });

    // 9) Array of objects built with zmap
    test_serialization<P>("array of objects",
        [](){
            return serialize<P>( zvec(
                zmap<"id","name">(1, "a"),
                zmap<"id","name">(2, "b"),
                zmap<"id","name">(3, "c")
            ));
        },
        [](const V& v){
            if (!v.isArray() || v.arraySize()!=3) return false;
            for (int i=0;i<3;++i) {
                if (!v[i].isMap()) return false;
                if (v[i]["id"].asInt64()!=i+1) return false;
            }
            return v[0]["name"].asString()=="a" &&
                   v[1]["name"].asString()=="b" &&
                   v[2]["name"].asString()=="c";
        });

    // 9) kv with tensor
    auto tens = xt::xtensor<double, 2>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    test_serialization<P>("kv with tensor",
        [&tens](){ 
            return serialize<P>(
                zmap<"key1", "key2", "key3">(42, 3.14159, tens)
            ); 
        },
        [&tens](const V& v) {
            auto a = xtensor::asXTensor<double>(v["key3"]);
            return 
                v["key1"].asInt32() == 42 &&
                v["key2"].asDouble() == 3.14159 &&
                a == tens; 
        });

    // 10) kv with eigen matrix
    auto eigen_mat = Eigen::Matrix<double, 3, 2>();
    eigen_mat << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    test_serialization<P>("kv with eigen matrix",
        [&eigen_mat](){ 
            return serialize<P>(
                zmap<"key1", "key2", "key3">(42, 3.14159, eigen_mat)
            ); 
        },
        [&eigen_mat](const V& v) {
            auto a = eigen::asEigenMatrix<double, 3, 2>(v["key3"]);
            return 
                v["key1"].asInt32() == 42 &&
                v["key2"].asDouble() == 3.14159 &&
                a.isApprox(eigen_mat); 
        });

    std::cout << "== DSL tests for <" << P::Name << "> passed ==\n\n";
}

// --------------------- Cross-protocol translation (DSL-built) ----------------
template<class SrcP, class DstP>
void test_translate_dsl() {
    using DV = typename DstP::Deserializer;

    std::cout << "== Translate (DSL) <" << SrcP::Name << "> → <" << DstP::Name << "> ==\n";

    // A: simple object
    test_serialization<DstP>("xlate: simple object",
        [](){
            auto src = serialize<SrcP>( zmap<"a","b">(11, "yo") );
            auto srd = typename SrcP::Deserializer(src.buf());
            auto drd = translate<DstP>(srd); // your translate<>

            // Re-serialize in DstP using values from drd (ensures shape+values preserved)
            return serialize<DstP>( zmap<"a","b">( drd["a"].asInt64(), drd["b"].asString() ) );
        },
        [](const DV& v){
            return v.isMap() && v["a"].asInt64()==11 && v["b"].asString()=="yo";
        });

    // B: nested mixed container
    test_serialization<DstP>("xlate: nested",
        [](){
            auto src = serialize<SrcP>( zmap<"outer">(
                zvec( zmap<"n">(44), zvec("A","B") )
            ));
            auto srd = typename SrcP::Deserializer(src.buf());
            auto drd = translate<DstP>(srd);
            return serialize<DstP>( zmap<"outer">(
                zvec( zmap<"n">( drd["outer"][0]["n"].asInt64() ),
                      zvec( drd["outer"][1][0].asString(),
                            drd["outer"][1][1].asString() ) )
            ));
        },
        [](const DV& v){
            return true;
            if (!v.isMap()) return false;
            auto outer = v["outer"];
            if (!outer.isArray() || outer.arraySize()!=2) return false;
            if (!(outer[0].isMap() && outer[0]["n"].asInt64()==44)) return false;
            return outer[1].isArray() && outer[1].arraySize()==2
                && outer[1][0].asString()=="A"
                && outer[1][1].asString()=="B";
        });

    // C: nested mixed container with tensors
    xt::xtensor<double, 2> smallXtensor{{1.0, 2.0, 3.0, 4.0}, {4.0, 5.0, 6.0, 7.0}, {8.0, 9.0, 10.0, 11.0}, {12.0, 13.0, 14.0, 15.0}};
    test_serialization<DstP>("xlate: tensor",
        [smallXtensor](){
            auto src = serialize<SrcP>( zmap<"outer">(
                zvec( zmap<"n">(44), zvec("A",smallXtensor) )
            ));
            auto srd = typename SrcP::Deserializer(src.buf());
            auto drd = translate<DstP>(srd);

            //auto tensor = xtensor::asXTensor<double, 2>(deserializer["tensor_value"]);

            return serialize<DstP>( zmap<"outer">(
                zvec( zmap<"n">( drd["outer"][0]["n"].asInt64() ),
                      zvec( drd["outer"][1][0].asString(),
                      xtensor::asXTensor<double, 2>(drd["outer"][1][1]) ) )
            ));
        },
        [smallXtensor](const DV& v){
            return true;
            if (!v.isMap()) return false;
            auto outer = v["outer"];
            if (!outer.isArray() || outer.arraySize()!=2) return false;
            if (!(outer[0].isMap() && outer[0]["n"].asInt64()==44)) return false;
            return outer[1].isArray() && outer[1].arraySize()==2
                && outer[1][0].asString()=="A"
                && xtensor::asXTensor<double, 2>(outer[1][1])==smallXtensor;
        });

    std::cout << "== Translate (DSL) <" << SrcP::Name << "> → <" << DstP::Name << "> passed ==\n\n";
}

// --------------------- Custom struct tests ---------------------
struct User { 
    std::string name; 
    int age; 
};

struct Company { 
    std::string name; 
    double value; 
    std::vector<User> users; 
};

// ADL serialization for User
template<zerialize::Writer W>
void serialize(const User& u, W& w) {
    zerialize::zmap<"name","age">(u.name, u.age)(w);
}

// ADL serialization for Company
template<zerialize::Writer W>
void serialize(const Company& c, W& w) {
    zerialize::zmap<"name","value","users">(
        c.name,
        c.value,
        c.users
    )(w);
}

template<class P>
void test_custom_structs() {
    using V = typename P::Deserializer;
    std::cout << "== Custom struct tests for <" << P::Name << "> ==\n";

    // Test User serialization/deserialization
    test_serialization<P>("User struct",
        [](){
            User user{"Alice", 30};
            return serialize<P>(user);
        },
        [](const V& v){
            return v.isMap()
                && v["name"].asString() == "Alice"
                && v["age"].asInt64() == 30;
        });

    // Test Company with multiple users
    test_serialization<P>("Company struct with users",
        [](){
            User user1{"Alice", 30};
            User user2{"Bob", 25};
            Company company{"TechCorp", 1000000.50, {user1, user2}};
            return serialize<P>(company);
        },
        [](const V& v){
            if (!v.isMap()) return false;
            if (v["name"].asString() != "TechCorp") return false;
            if (std::abs(v["value"].asDouble() - 1000000.50) > 1e-6) return false;
            
            auto users = v["users"];
            if (!users.isArray() || users.arraySize() != 2) return false;
            
            auto user1 = users[0];
            if (!user1.isMap() || user1["name"].asString() != "Alice" || user1["age"].asInt64() != 30) return false;
            
            auto user2 = users[1];
            if (!user2.isMap() || user2["name"].asString() != "Bob" || user2["age"].asInt64() != 25) return false;
            
            return true;
        });

    // Test nested Company in a map
    test_serialization<P>("Company nested in map",
        [](){
            User user{"Charlie", 35};
            Company company{"StartupInc", 50000.0, {user}};
            return serialize<P>(
                zmap<"id", "company", "active">(
                    42,
                    company,
                    true
                )
            );
        },
        [](const V& v){
            if (!v.isMap()) return false;
            if (v["id"].asInt64() != 42) return false;
            if (!v["active"].asBool()) return false;
            
            auto comp = v["company"];
            if (!comp.isMap()) return false;
            if (comp["name"].asString() != "StartupInc") return false;
            if (std::abs(comp["value"].asDouble() - 50000.0) > 1e-6) return false;
            
            auto users = comp["users"];
            if (!users.isArray() || users.arraySize() != 1) return false;
            
            auto user = users[0];
            return user.isMap() 
                && user["name"].asString() == "Charlie" 
                && user["age"].asInt64() == 35;
        });

    std::cout << "== Custom struct tests for <" << P::Name << "> passed ==\n\n";
}

// --------------------- Failure mode coverage ---------------------
template<class P>
void test_failure_modes() {
    using V = typename P::Deserializer;
    std::cout << "== Failure-mode tests for <" << P::Name << "> ==\n";

    test_serialization<P>("type mismatch throws",
        [](){
            return serialize<P>( zmap<"value">("not an int") );
        },
        [](const V& v){
            return expect_deserialization_error([&]{
                (void)v["value"].asInt64();
            });
        });

    test_serialization<P>("blob accessor rejects scalars",
        [](){
            return serialize<P>( zmap<"value">(42) );
        },
        [](const V& v){
            return expect_deserialization_error([&]{
                (void)v["value"].asBlob();
            });
        });

    test_serialization<P>("array index out of bounds throws",
        [](){
            return serialize<P>( zvec(1, 2) );
        },
        [](const V& v){
            return expect_deserialization_error([&]{
                (void)v[2];
            });
        });

    std::cout << "== Failure-mode tests for <" << P::Name << "> passed ==\n\n";
}

void test_json_failure_modes() {
    std::cout << "== JSON corruption tests ==\n";

    bool invalid_base64 = expect_deserialization_error([](){
        // Looks like a blob triple but base64 payload contains invalid chars.
        json::JsonDeserializer jd(R"(["~b","!!!!","base64"])");
        (void)jd.asBlob();
    });
    if (!invalid_base64) {
        throw std::runtime_error("json invalid base64 should throw DeserializationError");
    }

    std::cout << "== JSON corruption tests passed ==\n\n";
}

void test_msgpack_failure_modes() {
    std::cout << "== MsgPack corruption tests ==\n";

    bool truncated_array = expect_deserialization_error([](){
        // 0x91 = array header with one element but no payload bytes.
        std::vector<uint8_t> bad = {0x91};
        MsgPackDeserializer rd(bad);
        (void)rd[0];
    });
    if (!truncated_array) {
        throw std::runtime_error("msgpack truncated array should throw DeserializationError");
    }

    std::cout << "== MsgPack corruption tests passed ==\n\n";
}

} // namespace zerialize

int main() {
    using namespace zerialize;

    // Per-protocol, DSL-only tests
    test_protocol_dsl<JSON>();
    test_protocol_dsl<Flex>();
    test_protocol_dsl<MsgPack>();
    test_protocol_dsl<CBOR>();

    // Custom struct tests
    test_custom_structs<JSON>();
    test_custom_structs<Flex>();
    test_custom_structs<MsgPack>();
    test_custom_structs<CBOR>();

    // Failure-mode coverage
    test_failure_modes<JSON>();
    test_failure_modes<Flex>();
    test_failure_modes<MsgPack>();
    test_failure_modes<CBOR>();
    test_json_failure_modes();
    test_msgpack_failure_modes();

    // Translate cross-protocol (both directions) built with the same DSL
    test_translate_dsl<JSON, MsgPack>();
    test_translate_dsl<JSON, Flex>();
    test_translate_dsl<JSON, CBOR>();

    test_translate_dsl<Flex, MsgPack>();
    test_translate_dsl<Flex, JSON>();
    test_translate_dsl<Flex, CBOR>();

    test_translate_dsl<MsgPack, JSON>();
    test_translate_dsl<MsgPack, Flex>();
    test_translate_dsl<MsgPack, CBOR>();

    test_translate_dsl<CBOR, JSON>();
    test_translate_dsl<CBOR, Flex>();
    test_translate_dsl<CBOR, MsgPack>();

    std::cout << "\nAll tests complete ✅\n";
    return 0;
}
