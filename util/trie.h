#include <iostream>
#include <optional>

const int CHARSET_SIZE = 256;

template<typename T>
struct TrieNode
{
	TrieNode(std::optional<T> prop) :
		prop(prop), cnt(0)
	{
		for (auto &i : ch) i = nullptr;
	}

	TrieNode *ch[CHARSET_SIZE];
	std::optional<T> prop;
	int cnt;
};

template<typename T>
class Trie
{
public:
	Trie() {}

	~Trie()
	{
		destroyRecursive(root);
	}

	void insert(const std::string &str, T prop)
	{
		TrieNode<T> **k = &root;
		for (auto i : str)
		{
			if ((*k)->ch[i] == nullptr)
			{
				(*k)->ch[i] = new TrieNode<T>(std::nullopt);
			}
			(*k)->cnt++;
			k = &(*k)->ch[i];
		}
		(*k)->prop = prop;
		(*k)->cnt++;
	}

	std::optional<T> find(const std::string &str)
	{
		TrieNode<T> *k = root;
		for (auto i : str)
		{
			if (k->ch[i] == nullptr) return std::nullopt;
			k = k->ch[i];
		}
		return k->prop;
	}

	void remove(const std::string &str)
	{
		if (!find(str).has_value()) return;
		remove(root, str, 0);
	}

	void dfs()
	{
		dfs(root, "");
		std::cout << "\n";
	}

	void dfs(TrieNode<T> *k, std::string str)
	{
		if (k->prop.has_value()) std::cout << str << "\n";
		for (int i = 0; i < CHARSET_SIZE; i++)
		{
			if (k->ch[i] != nullptr)
			{
				std::cout << char(i) << " " << k->ch[i]->cnt << "\n";
				dfs(k->ch[i], str + char(i));
			}
		}
	}

private:
	void remove(TrieNode<T> *&k, const std::string &str, int index)
	{
		if (index == str.length()) k->prop = std::nullopt;
		else if (k->ch[str[index]] != nullptr) remove(k->ch[str[index]], str, index + 1);

		k->cnt--;
		if (k->cnt == 0)
		{
			delete k;
			k = nullptr;
		}
	}

	void destroyRecursive(TrieNode<T> *&k)
	{
		for (int i = 0; i < CHARSET_SIZE; i++)
		{
			if (k->ch[i] != nullptr) destroyRecursive(k->ch[i]);
		}
		delete k;
	}

private:
	TrieNode<T> *root = new TrieNode<T>(std::nullopt);
};
