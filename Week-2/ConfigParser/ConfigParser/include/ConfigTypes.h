#pragma once

// Types used by the configuration parser
// Line and column numbers used for diagnostics are ONE-BASED (1..N).

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <functional>

// Tokenizer tokens
enum class TokenType
{
	BlankLine,
	Comment,
	SectionHeader,
	Key,
	Equals,
	Value
};

struct Token
{
	TokenType type;
	std::string value;
	int lineNumber = 0;   // one-based
	int columnNumber = 0; // one-based
};

// Error reported by tokenizer/parser
struct Error
{
	int lineNumber = 0;
	int columnNumber = 0;
	std::string message;
};

// Scalar and array value types
using IntScalar = std::int64_t;
using BoolScalar = bool;
using StringScalar = std::string;
using ScalarValue = std::variant<BoolScalar, IntScalar, StringScalar>;

using IntArray = std::vector<IntScalar>;
using BoolArray = std::vector<BoolScalar>;
using StringArray = std::vector<StringScalar>;
using ArrayValue = std::variant<IntArray, BoolArray, StringArray>;

using Value = std::variant<ScalarValue, ArrayValue>;

// Section holds key->Value mappings. Storage is private; lookups return optional
// reference wrappers to const Value to prevent external mutation.
class Section
{
public:
	Section() = default;

	using iterator = std::map<std::string, Value>::iterator;
	using const_iterator = std::map<std::string, Value>::const_iterator;

	// Insert a key/value. Returns pair(iterator, inserted)
	std::pair<iterator, bool> emplace(const std::string& key, const Value& value)
	{
		return data_.emplace(key, value);
	}

	// Query without exposing mutable storage
	std::optional<std::reference_wrapper<const Value>> Get(const std::string& key) const
	{
		auto it = data_.find(key);
		if (it == data_.end())
			return std::nullopt;
		return std::cref(it->second);
	}

	bool Contains(const std::string& key) const
	{
		return data_.find(key) != data_.end();
	}

	std::size_t size() const { return data_.size(); }

	// For parser convenience (internal use) provide mutable operator[]
	// Note: this returns a mutable reference so callers inside the parser can
	// populate sections. External users should prefer Get() which returns const.
	Value& operator[](const std::string& key)
	{
		return data_[key];
	}

	iterator begin() { return data_.begin(); }
	iterator end() { return data_.end(); }
	const_iterator begin() const { return data_.begin(); }
	const_iterator end() const { return data_.end(); }
	const_iterator cbegin() const { return data_.cbegin(); }
	const_iterator cend() const { return data_.cend(); }

private:
	std::map<std::string, Value> data_;
};

// Document holds named sections. Storage is private; lookups return optional
// reference wrappers to const Section or const Value.
class Document
{
public:
	Document() = default;

	using iterator = std::map<std::string, Section>::iterator;
	using const_iterator = std::map<std::string, Section>::const_iterator;

	// Emplace a section (used by the parser). Returns pair(iterator, inserted)
	std::pair<iterator, bool> emplace(const std::string& name, Section&& section)
	{
		return sections_.emplace(name, std::move(section));
	}

	// Mutable access for parser to populate sections: creates section if missing
	Section& operator[](const std::string& name)
	{
		return sections_[name];
	}

	// Const lookup for consumers
	std::optional<std::reference_wrapper<const Section>> GetSection(const std::string& name) const
	{
		auto it = sections_.find(name);
		if (it == sections_.end())
			return std::nullopt;
		return std::cref(it->second);
	}

	// Get a value from a named section/key
	std::optional<std::reference_wrapper<const Value>> GetValue(const std::string& section,
																const std::string& key) const
	{
		auto it = sections_.find(section);
		if (it == sections_.end())
			return std::nullopt;
		return it->second.Get(key);
	}

	std::size_t size() const { return sections_.size(); }

	iterator begin() { return sections_.begin(); }
	iterator end() { return sections_.end(); }
	const_iterator begin() const { return sections_.begin(); }
	const_iterator end() const { return sections_.end(); }
	const_iterator cbegin() const { return sections_.cbegin(); }
	const_iterator cend() const { return sections_.cend(); }

private:
	std::map<std::string, Section> sections_;
};
