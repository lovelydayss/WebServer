#include <cassert>
#include <cstddef>
#include <cstdint> // for std::int64_t and std::uint64_t
#include <cstdio>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory> // for std::allocator
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits> // for SFINAE (Substitution Failure Is Not An Error)
#include <vector>

namespace basic {

// 兼容 std::enable_if_t 的语法
template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

// 兼容 std::make_unique 的语法
template <typename T, typename... Args>
typename std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 分支预测优化
#ifdef _ENABLE_LIKELY_
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely
#define unlikely
#endif

} // namespace basic

namespace mJson {

using namespace basic;

// no binary/discarded
enum class value_type : std::uint8_t {
	NULL_TYPE = 1,   ///< null value
	OBJECT,          ///< object (unordered set of name/value pairs)
	ARRAY,           ///< array (ordered collection of values)
	STRING,          ///< string value
	BOOLEAN,         ///< boolean value
	NUMBER_INTEGER,  ///< number value (signed integer)
	NUMBER_UNSIGNED, ///< number value (unsigned integer)
	NUMBER_FLOAT,    ///< number value (floating-point)
	BINARY,          ///< binary array (ordered collection of bytes)
	DISCARDED        ///< discarded by the parser callback function
};

static std::vector<std::string> valueTypeString = {
    "",
    "NULL_TYPE",
    "OBJECT",
    "ARRAY",
    "STRING",
    "BOOLEAN",
    "NUMBER_INTEGER",
    "NUMBER_UNSIGNED",
    "NUMBER_FLOAT",
    // "BINARY",
    // "DISCARDED"
};

enum class error_type : std::uint8_t {
	LEPT_PARSE_OK,                ///< 成功解析
	LEPT_PARSE_EXPECT_VALUE,      ///< 空的 Json
	LEPT_PARSE_INVALID_VALUE,     ///< 非合法字面值
	LEPT_PARSE_ROOT_NOT_SINGULAR, ///< 空白值后还有其他字符
	LEPT_PARSE_NUMBER_TOO_BIG, ///< 数值过大双精度浮点数无法表示
	LEPT_PARSE_MISS_QUOTATION_MARK,          ///< 缺少字符串封闭标记
	LEPT_PARSE_INVALID_STRING_ESCAPE,        ///< 非法转义标志
	LEPT_PARSE_INVALID_STRING_CHAR,          ///< 非法字符
	LEPT_PARSE_INVALID_UNICODE_HEX,          ///< 非法十六进制编码
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,    ///< 非法代理对范围
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, ///< 数组未闭合
	LEPT_PARSE_MISS_KEY,                     ///< 缺少键值
	LEPT_PARSE_MISS_COLON,                   ///< 缺少中间 :
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET,  ///< 对象未闭合

	INSERT_OBJECT_OK,
	MODIFY_OBJECT_OK,
	REMOVE_OBJECT_OK,
	OBJECT_INDEX_WRONG,
	LEPT_KEY_NOT_EXIST
};

static std::vector<std::string> errorTypeString = {
    "LEPT_PARSE_OK",
    "LEPT_PARSE_EXPECT_VALUE",
    "LEPT_PARSE_INVALID_VALUE",
    "LEPT_PARSE_ROOT_NOT_SINGULAR",
    "LEPT_PARSE_NUMBER_TOO_BIG",
    "LEPT_PARSE_MISS_QUOTATION_MARK",
    "LEPT_PARSE_INVALID_STRING_ESCAPE",
    "LEPT_PARSE_INVALID_STRING_CHAR",
    "LEPT_PARSE_INVALID_UNICODE_HEX",
    "LEPT_PARSE_INVALID_UNICODE_SURROGATE",
    "LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET",
    "LEPT_PARSE_MISS_KEY",
    "LEPT_PARSE_MISS_COLON",
    "LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET",

    "INSERT_OBJECT_OK",
    "MODIFY_OBJECT_OK",
    "REMOVE_OBJECT_OK",
    "OBJECT_INDEX_WRONG",
    "LEPT_KEY_NOT_EXIST"};

class json;
using jsonPtr = std::unique_ptr<json>;
using jsonConstPtr = const std::unique_ptr<json>;
using jsonRef = json&;
using jsonConstRef = const json&;

///< iterator
//class iterator;
//class const_iterator;

// class reverse_iterator;
// class const_reverse_iterator;

///< typedef
using value_t = value_type;
using object_t = std::map<std::string, jsonPtr>;
using array_t = std::vector<jsonPtr>;
using string_t = std::string;
using boolean_t = bool;
using number_float_t = double;
using number_integer_t = std::int32_t;
using number_unsigned_t = std::uint32_t;

using object_key_t = object_t::key_type;
using object_val_t = object_t::value_type;

using size_t = std::size_t;
using initializer_list_t = std::initializer_list<json>;

///< template to complete undored_map or map reload
// using json=basic_json<map>;
// using ordered_json = basic_json<ordered_map>;

class json {

public:
	class const_iterator {
	public:
		friend class MyVector;
		const_iterator(json* ptr = nullptr)
		    : ptr_(ptr) {}
		void operator++() { ++ptr_; }
		bool operator!=(const const_iterator& it) { return ptr_ != it.ptr_; }
		// 返回值被const修饰，只能读，不能修改
		const json& operator*() const { return *ptr_; }
		const json* operator->() const { return ptr_; }

	protected:
		json* ptr_;
	};

	class iterator : const_iterator {
	public:
		iterator(json* ptr = nullptr)
		    : const_iterator(ptr) {}
		// 返回类型是T&的普通引用，可读可写
		json& operator*() { return *const_iterator::ptr_; }
		json* operator->() { return const_iterator::ptr_; }
	};

	///< Constructor
	json(const value_t v);
	json(std::nullptr_t = nullptr);

	template <typename JsonValueType>
	json(const JsonValueType& val);

	template <typename JsonValueType>
	json(const string_t&, const JsonValueType& val);

	json(initializer_list_t init,
	     bool typeDeduction = true,
	     value_t manualType = value_t::ARRAY);

	json(size_t cnt, jsonConstRef val);

	json(iterator first, iterator last);
	json(const_iterator first, const_iterator last);

	json(jsonConstRef other) noexcept;
	json(json&& other) noexcept;

	json(const string_t utf8JsonString) noexcept;

	///< Destructor
	~json() noexcept;

	///< swap
	void swap(jsonRef other) noexcept;
	void swap(jsonRef left, jsonRef right) noexcept;

	void swap(array_t& other);
	void swap(object_t& other);
	void swap(string_t& other);

	// void swap(binary_t& other);
	// void swap(typename binary_t::container_type& other);

	///< parse
	static json parse(jsonRef j, const string_t s);

	///< stringify
	std::string to_string(jsonConstRef j);

	///< at
	jsonRef at(size_t idx);
	jsonConstRef at(size_t idx) const;
	jsonRef at(const object_key_t& key);
	jsonConstRef at(const object_key_t& key) const;
	jsonRef at(jsonConstPtr& ptr);
	jsonConstRef at(jsonConstPtr& ptr) const;

	template <typename KeyType>
	jsonRef at(KeyType&& key);
	template <typename KeyType>
	jsonConstRef at(KeyType&& key) const;

	///< accept
	template <typename InputType>
	static bool accept(InputType&& i, const bool ignoreComments = false);

	template <typename IteratorType>
	static bool accept(IteratorType first,
	                   IteratorType last,
	                   const bool ignoreComments = false);

	///< front
	jsonRef front();
	jsonConstRef front() const;

	///< back
	jsonRef back();
	jsonConstRef back() const;

	///< begin & cbegin
	iterator begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;

	///< end & cend
	iterator end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;

	///< find
	iterator find(const object_key_t& key);
	const_iterator find(const object_key_t& key) const;

	template <typename KeyType>
	iterator find(KeyType&& key);
	template <typename KeyType>
	const_iterator find(KeyType&& key) const;

	///< flatten
	json flatten() const;

	///< emplace & emplace_back
	template <class... Args>
	std::pair<iterator, bool> emplace(Args&&... args);

	template <class... Args>
	jsonRef emplace_back(Args&&... args);

	///< push_back
	void push_back(const json& val);
	void push_back(json&& val);

	void push_back(const object_val_t& val);
	void push_back(initializer_list_t init);

	///< insert
	iterator insert(const_iterator pos, const json& val);
	iterator insert(const_iterator pos, json&& val);

	iterator insert(const_iterator pos, size_t cnt, const json& val);

	iterator
	insert(const_iterator pos, const_iterator first, const_iterator last);

	iterator insert(const_iterator pos, initializer_list_t ilist);

	void insert(const_iterator first, const_iterator last);

	///< earse
	iterator erase(iterator pos);
	const_iterator erase(const_iterator pos);

	iterator erase(iterator first, iterator last);
	const_iterator erase(const_iterator first, const_iterator last);

	size_t erase(const string_t& key);
	void erase(const size_t idx);

	template <typename KeyType>
	size_t erase(KeyType&& key);

	///< update
	void update(jsonConstRef j, bool mergeObjects = false);
	void update(const_iterator first,
	            const_iterator last,
	            bool mergeObjects = false);

	///< get
	template <typename JsonValueType>
	JsonValueType get() const;

	template <typename JsonValuePtrType>
	JsonValuePtrType get_ptr();

	template <typename JsonValuePtrType>
	constexpr const JsonValuePtrType get_ptr() const noexcept;

	template <typename JsonValueRefType>
	JsonValueRefType get_ref();

	template <typename JsonValueRefType>
	const JsonValueRefType get_ref() const;

	///< items

	///< empty
	bool empty() const noexcept {

		assert(is_primitive() || is_structured());

		if (unlikely(is_null()))
			return true;
		else if (is_primitive())
			return false;
		else
			return is_array() ? array_.empty() : map_.empty();
	}

	///< clear
	void clear() noexcept {

		assert(is_primitive() || is_structured());

		if (unlikely(is_null()))
			return;

		switch (type_) {
		case value_type::BOOLEAN: boolean_ = false; return;
		case value_type::NUMBER_FLOAT: double_ = 0.0; return;
		case value_type::NUMBER_INTEGER: integer_ = 0; return;
		case value_type::NUMBER_UNSIGNED: uinteger_ = 0; return;
		case value_type::STRING: str_ = ""; return;
		case value_type::BINARY: return;
		case value_type::ARRAY: array_.clear(); return;
		case value_type::OBJECT: map_.clear(); return;
		default: return;
		}
	}

	///< contains
	bool contains(const object_key_t& key) const { return count(key) != 0; }

	// bool contains(jsonConstPtr& ptr) const;

	// template <typename KeyType>
	// bool contains(KeyType&& key) const;

	///< count
	///< there don't allow element duplication
	///< just 0-1
	size_t count(const object_key_t& key) const {
		assert(is_object());
		return map_.find(key) != map_.end() ? 1 : 0;
	}

	// template <typename KeyType>
	// size_t count(KeyType&& key) const;

	///< size
	size_t size() const noexcept {

		if (unlikely(is_null()))
			return 0;
		else if (is_primitive())
			return 1;
		else
			return is_array() ? array_.size() : map_.size();
	}

	///< type
	constexpr value_t type() const noexcept { return type_; }
	string_t type_name() const { return errorTypeString[(size_t)type_]; }

	///< is.....
	constexpr bool is_primitive() const noexcept {
		return is_string() || is_number() || is_boolean() || is_null() ||
		       is_binary();
	}
	constexpr bool is_structured() const noexcept {
		return is_array() || is_object();
	}
	constexpr bool is_null() const noexcept {
		return type_ == value_type::NULL_TYPE;
	}
	constexpr bool is_object() const noexcept {
		return type_ == value_type::OBJECT;
	}
	constexpr bool is_array() const noexcept {
		return type_ == value_type::ARRAY;
	}
	constexpr bool is_string() const noexcept {
		return type_ == value_type::STRING;
	}
	constexpr bool is_boolean() const noexcept {
		return type_ == value_type::BOOLEAN;
	}
	constexpr bool is_number() const noexcept {
		return is_number_float() || is_number_integer() || is_number_unsigned();
	}
	constexpr bool is_number_float() const noexcept {
		return type_ == value_type::NUMBER_FLOAT;
	}
	constexpr bool is_number_integer() const noexcept {
		return type_ == value_type::NUMBER_INTEGER;
	}
	constexpr bool is_number_unsigned() const noexcept {
		return type_ == value_type::NUMBER_UNSIGNED;
	}
	constexpr bool is_binary() const noexcept {
		return type_ == value_type::BINARY;
	}
	constexpr bool is_discarded() const noexcept {
		return type_ == value_type::DISCARDED;
	}

	///< max_size
	size_t max_size() const noexcept {

		if (unlikely(is_null()))
			return 0;
		else if (is_primitive())
			return 1;
		else
			return is_array() ? array_.max_size() : map_.max_size();
	}

	///< array & object
	static json array(initializer_list_t init = {});
	static json object(initializer_list_t init = {});

	///< operator []
	jsonRef operator[](size_t idx);
	jsonConstRef operator[](size_t idx) const;
	jsonRef operator[](const object_key_t& key);
	jsonConstRef operator[](const object_key_t& key) const;

	jsonRef operator[](jsonConstPtr& ptr);
	jsonConstRef operator[](jsonConstPtr& ptr) const;

	///< operator =
	jsonRef operator=(jsonConstRef other) noexcept;

	///< operator +=
	jsonRef operator+=(jsonConstRef val);
	jsonRef operator+=(json&& val);

	jsonRef operator+=(const object_val_t& val);
	jsonRef operator+=(initializer_list_t init);

	///< operator == != > < >= <= <=>
	///< ......
	bool operator==(jsonConstRef rhs) const noexcept;
	bool operator!=(jsonConstRef rhs) const noexcept;
	// bool operator>(jsonConstRef rhs) const noexcept;
	// bool operator<(jsonConstRef rhs) const noexcept;
	// bool operator>=(jsonConstRef rhs) const noexcept;
	// bool operator<=(jsonConstRef rhs) const noexcept;

	bool equal(jsonConstRef rhs);

	///< operator<< >>
	friend std::ostream& operator<<(std::ostream& o, const json& j);
	friend std::ostream& operator<<(std::ostream& o, jsonConstPtr& ptr);

	friend std::istream& operator>>(std::istream& i, json& j);

private:
	value_t type_;
	union {
		object_t map_;               ///< object value
		array_t array_;              ///< array value
		string_t str_;               ///< string value
		                             ///<
		number_integer_t integer_;   ///< int value
		number_unsigned_t uinteger_; ///< unsigned int value
		number_float_t double_;      ///< float value

		boolean_t boolean_;
	};
};

} // namespace mJson