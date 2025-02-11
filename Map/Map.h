#ifndef IS_BTREE_MAP
#define IS_BTREE_MAP

#include <cstddef>
#include <iterator>

// Implementation of map using AA Tree
// The comparator must satisfy strict weak ordering relation
// TODO try to implement custom Allocator
template <typename Key, typename T, typename Compare>
class Map {
private:
    struct Node;
    class Iterator;
    class ConstIterator;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = Iterator;
    using const_iterator = ConstIterator;

    Map()
        : root_(nullptr),
          b_iter_(Iterator(nullptr)),
          e_iter_(Iterator(nullptr)),
          size_(0),
          comparator_(Compare()) {}

    Map(const Map& other)
        : root_(other.root_ == nullptr ? nullptr : new Node(*other.root_)),
          b_iter_(Iterator(beginNode(root_))),
          e_iter_(nullptr),
          size_(other.size_),
          comparator_(Compare()) {}

    Map& operator=(const Map& other) {
        if (this != &other) {
            delete root_;
            root_ = nullptr;
            size_ = 0;
            b_iter_ = Iterator(nullptr);
            if (other.root_ != nullptr) {
                root_ = new Node(*other.root_);
                b_iter_ = Iterator(beginNode(root_));
                size_ = other.size_;
            }
        }
        return *this;
    }

    Map(Map&& other) noexcept
        : root_(other.root_),
          b_iter_(other.b_iter_),
          e_iter_(nullptr),
          size_(other.size_),
          comparator_(Compare()) {
        other.root_ = nullptr;
        other.b_iter_ = Iterator(nullptr);
        other.size_ = 0;
    }

    Map& operator=(Map&& other) noexcept {
        if (this != &other) {
            root_ = other.root_;
            b_iter_ = other.b_iter_;
            size_ = other.size_;
            other.root_ = nullptr;
            other.b_iter_ = Iterator(nullptr);
            other.size_ = 0;
        }
        return *this;
    }

    ~Map() {
        delete root_;
        root_ = nullptr;
        b_iter_ = Iterator(nullptr);
        size_ = 0;
    }

    // Iterators
    [[nodiscard]] iterator begin() noexcept {
        return b_iter_;
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return ConstIterator(b_iter_);
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return ConstIterator(b_iter_);
    }

    [[nodiscard]] iterator end() noexcept {
        return e_iter_;
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return ConstIterator(e_iter_);
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return ConstIterator(e_iter_);
    }

    // Capacity
    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] size_type getSize() const noexcept {
        return size_;
    }

    // Modifiers

    // Return a pair consisting of an iterator to the inserted element (or to
    // the element that prevented the insertion) and a bool value set to true if
    // and only if the insertion took place.
    std::pair<iterator, bool> insert(const_reference value) {
        Node* parent = findParent(value.first);
        if ((parent != nullptr) && (parent->value->first == value.first)) {
            return std::pair<iterator, bool>{Iterator(parent), false};
        }
        Node* new_node =
            new Node({new value_type(value), nullptr, nullptr, parent, 1});
        ++size_;
        if (parent == nullptr) {
            root_ = new_node;
            b_iter_ = Iterator(root_);
        } else {
            if (comparator_(value.first, b_iter_->first)) {
                b_iter_ = Iterator(new_node);
            }
            Node* rebalance_node = nullptr;
            if (comparator_(value.first, parent->value->first)) {
                parent->left = new_node;
                rebalance_node = parent;
            } else {
                parent->right = new_node;
                rebalance_node = parent->parent;
            }
            int unchanged_nodes = 0;
            bool is_tree_changed = false;
            while ((rebalance_node != nullptr) && (unchanged_nodes < 3)) {
                is_tree_changed = rebalance_node != skew(rebalance_node);
                is_tree_changed =
                    is_tree_changed || rebalance_node != split(rebalance_node);
                if (!is_tree_changed) {
                    ++unchanged_nodes;
                } else {
                    unchanged_nodes = 0;
                }
                rebalance_node = rebalance_node->parent;
            }
        }
        return std::pair<iterator, bool>{Iterator(new_node), true};
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        Node* parent = findParent(value.first);
        if ((parent != nullptr) && (parent->value->first == value.first)) {
            return std::pair<iterator, bool>{Iterator(parent), false};
        }
        Node* new_node = new Node(
            {new value_type(std::move(value)), nullptr, nullptr, parent, 1});
        ++size_;
        if (parent == nullptr) {
            root_ = new_node;
            b_iter_ = Iterator(root_);
        } else {
            if (comparator_(value.first, b_iter_->first)) {
                b_iter_ = Iterator(new_node);
            }
            Node* rebalance_node = nullptr;
            if (comparator_(value.first, parent->value->first)) {
                parent->left = new_node;
                rebalance_node = parent;
            } else {
                parent->right = new_node;
                rebalance_node = parent->parent;
            }
            int unchanged_nodes = 0;
            bool is_tree_changed = false;
            while ((rebalance_node != nullptr) && (unchanged_nodes < 3)) {
                is_tree_changed = rebalance_node != skew(rebalance_node);
                is_tree_changed =
                    is_tree_changed || rebalance_node != split(rebalance_node);
                if (!is_tree_changed) {
                    ++unchanged_nodes;
                } else {
                    unchanged_nodes = 0;
                }
                rebalance_node = rebalance_node->parent;
            }
        }
        return std::pair<iterator, bool>{Iterator(new_node), true};
    }

    // TODO change function signature
    void erase(const key_type& erased_key) noexcept {
        if (root_ == nullptr) {
            return;
        }
        Node* current_node = findNode(erased_key);
        if (current_node != nullptr) {
            Node* rebalance_node = nullptr;
            if (current_node->left == nullptr &&
                current_node->right == nullptr) {
                rebalance_node = trivialNodeErase(current_node, nullptr);
            } else if (current_node->left != nullptr &&
                       current_node->right == nullptr) {
                rebalance_node =
                    trivialNodeErase(current_node, current_node->left);
            } else if (current_node->left == nullptr &&
                       current_node->right != nullptr) {
                rebalance_node =
                    trivialNodeErase(current_node, current_node->right);
            } else {
                Node* next_node = next(current_node);
                Node* right_child = next_node->right;
                rebalance_node = next_node->parent;
                if (right_child != nullptr) {
                    right_child->parent = rebalance_node;
                }
                if (rebalance_node->left == next_node) {
                    rebalance_node->left = right_child;
                } else {
                    rebalance_node->right = right_child;
                }
                std::swap(current_node->value, next_node->value);
                current_node->value->second =
                    std::move(next_node->value->second);
                --size_;
                next_node->left = nullptr;
                next_node->right = nullptr;
                next_node->parent = nullptr;
                delete next_node;
            }
            bool is_level_changed = true;
            while ((rebalance_node != nullptr) && is_level_changed) {
                size_type init_level = rebalance_node->level;
                decreaseNodeLevel(rebalance_node);
                is_level_changed = init_level != rebalance_node->level;
                if (is_level_changed) {
                    rebalance_node = skew(rebalance_node);
                    if (rebalance_node->right != nullptr) {
                        skew(rebalance_node->right);
                        if (rebalance_node->right->right != nullptr) {
                            skew(rebalance_node->right->right);
                        }
                    }
                    rebalance_node = split(rebalance_node);
                    if (rebalance_node->right != nullptr) {
                        split(rebalance_node->right);
                    }
                }
                rebalance_node = rebalance_node->parent;
            }
        }
    }

    // Lookup
    iterator find(const key_type& search_key) noexcept {
        return Iterator(findNode(search_key));
    }

    const_iterator find(const key_type& search_key) const noexcept {
        return ConstIterator(findNode(search_key));
    }

    [[nodiscard]] bool contains(const key_type& key) const noexcept {
        return findNode(key) != nullptr;
    }

private:
    Node* root_;
    iterator b_iter_;
    iterator e_iter_;
    size_type size_;
    key_compare comparator_;

    struct Node {
        Map::pointer value;
        Node* left;
        Node* right;
        Node* parent;
        Map::size_type level;

        Node(Map::pointer value, Node* left, Node* right, Node* parent,
             Map::size_type level)
            : value(value),
              left(left),
              right(right),
              parent(parent),
              level(level) {}

        Node(const Node& other)
            : value(new value_type(*(other.value))),
              left(nullptr),
              right(nullptr),
              parent(nullptr),
              level(other.level) {
            if (other.left != nullptr) {
                left = new Node(*other.left);
                left->parent = this;
            }
            if (other.right != nullptr) {
                right = new Node(*other.right);
                right->parent = this;
            }
        }

        ~Node() {
            delete value;
            value = nullptr;
            delete left;
            delete right;
            left = nullptr;
            right = nullptr;
            parent = nullptr;
            level = 0;
        }
    };

    class Iterator {
    private:
        friend Map::ConstIterator;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = Map::difference_type;
        using value_type = Map::value_type;
        using reference = Map::reference;
        using pointer = Map::pointer;

        explicit Iterator(Node* ptr) noexcept
            : ptr_(ptr){};

        Iterator(const Iterator& other) noexcept
            : ptr_(other.ptr_){};

        Iterator& operator=(const Iterator& other) & noexcept {
            if (this != &other) {
                ptr_ = other.ptr_;
            }
            return *this;
        }

        ~Iterator(){};

        [[nodiscard]] reference operator*() const noexcept {
            return *(ptr_->value);
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return ptr_->value;
        }

        Iterator& operator++() {
            ptr_ = Map::next(ptr_);
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--() {
            ptr_ = Map::prev(ptr_);
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(const Iterator& right) const noexcept {
            return ptr_ == right.ptr_;
        }

        [[nodiscard]] bool operator!=(const Iterator& right) const noexcept {
            return ptr_ != right.ptr_;
        }

    private:
        Node* ptr_;
    };

    class ConstIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = Map::difference_type;
        using value_type = Map::value_type;
        using reference = Map::const_reference;
        using pointer = Map::const_pointer;

        explicit ConstIterator(Node* ptr) noexcept
            : ptr_(ptr){};

        explicit ConstIterator(const Iterator& other) noexcept
            : ptr_(other.ptr_){};

        ConstIterator(const ConstIterator& other) noexcept
            : ptr_(other.ptr_){};

        ConstIterator& operator=(const ConstIterator& other) & noexcept {
            if (this != &other) {
                ptr_ = other.ptr_;
            }
            return *this;
        }

        ~ConstIterator(){};

        [[nodiscard]] reference operator*() const noexcept {
            return *(ptr_->value);
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return ptr_->value;
        }

        ConstIterator& operator++() {
            ptr_ = Map::next(ptr_);
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        ConstIterator& operator--() {
            ptr_ = Map::prev(ptr_);
            return *this;
        }

        ConstIterator operator--(int) {
            ConstIterator tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(
            const ConstIterator& right) const noexcept {
            return ptr_ == right.ptr_;
        }

        [[nodiscard]] bool operator!=(
            const ConstIterator& right) const noexcept {
            return ptr_ != right.ptr_;
        }

    private:
        Node* ptr_;
    };

    Node* skew(Node* node) noexcept {
        if ((node->left == nullptr) || (node->level != node->left->level)) {
            return node;
        }
        Node* left_node = node->left;
        node->left = left_node->right;
        if (left_node->right != nullptr) {
            left_node->right->parent = node;
        }
        left_node->right = node;
        left_node->parent = node->parent;
        if (node->parent != nullptr) {
            if (node->parent->left == node) {
                node->parent->left = left_node;
            } else {
                node->parent->right = left_node;
            }
        }
        node->parent = left_node;
        if (root_ == node) {
            root_ = left_node;
        }
        return left_node;
    }

    Node* split(Node* node) noexcept {
        if (node->right == nullptr || node->right->right == nullptr ||
            node->level != node->right->right->level) {
            return node;
        }
        Node* right_node = node->right;
        node->right = right_node->left;
        if (right_node->left != nullptr) {
            right_node->left->parent = node;
        }
        right_node->left = node;
        right_node->parent = node->parent;
        if (node->parent != nullptr) {
            if (node->parent->left == node) {
                node->parent->left = right_node;
            } else {
                node->parent->right = right_node;
            }
        }
        node->parent = right_node;
        if (root_ == node) {
            root_ = right_node;
        }
        ++right_node->level;
        return right_node;
    }

    // Deleting a node in case of less than two children
    // return parent of erased node
    [[nodiscard]] Node* trivialNodeErase(Node* node_to_erase,
                                         Node* child) noexcept {
        if (child != nullptr) {
            child->parent = node_to_erase->parent;
        }
        Node* parent = nullptr;
        if (root_ != node_to_erase) {
            parent = node_to_erase->parent;
            if (parent->left == node_to_erase) {
                parent->left = child;
            } else {
                parent->right = child;
            }
        } else {
            root_ = child;
        }
        if (b_iter_ == Iterator(node_to_erase)) {
            ++b_iter_;
        }
        --size_;
        node_to_erase->left = nullptr;
        node_to_erase->right = nullptr;
        node_to_erase->parent = nullptr;
        delete node_to_erase;
        return parent;
    }

    void static decreaseNodeLevel(Node* node) noexcept {
        size_type left_diff = node->left != nullptr
                                  ? node->level - node->left->level
                                  : node->level;
        size_type right_diff = node->right != nullptr
                                   ? node->level - node->right->level
                                   : node->level;
        if (left_diff > 1 || right_diff > 1) {
            if (node->right != nullptr && node->right->level == node->level) {
                --node->right->level;
            }
            --node->level;
        }
    }

    [[nodiscard]] static Node* next(const Node* node) noexcept {
        if (node->right != nullptr) {
            Node* current_node = node->right;
            while (current_node->left != nullptr) {
                current_node = current_node->left;
            }
            return current_node;
        }
        Node* parent = node->parent;
        while (parent != nullptr && (parent->right == node)) {
            node = parent;
            parent = node->parent;
        }
        return parent;
    }

    [[nodiscard]] static Node* prev(const Node* node) noexcept {
        if (node->left != nullptr) {
            Node* current_node = node->left;
            while (current_node->right != nullptr) {
                current_node = current_node->right;
            }
            return current_node;
        }
        Node* parent = node->parent;
        while (parent != nullptr && (parent->left == node)) {
            node = parent;
            parent = node->parent;
        }
        return parent;
    }

    // return a pointer to the node containing the key, return nullptr if such a
    // key does not exist
    // TODO add noexcept condition for Compare class
    [[nodiscard]] Node* findNode(const key_type& search_key) const noexcept {
        Node* node = root_;
        while (node != nullptr) {
            const key_type key = node->value->first;
            if (!(comparator_(key, search_key)) &&
                !(comparator_(search_key, key))) {
                break;
            }
            if (comparator_(search_key, key)) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return node;
    }

    // return a pointer to the node containing the key, otherwise return the
    // parent of the new inserted node (return nullptr if map is empty)
    [[nodiscard]] Node* findParent(const key_type& search_key) const noexcept {
        Node* parent = nullptr;
        Node* node = root_;
        while (node != nullptr) {
            parent = node;
            const key_type key = node->value->first;
            if (!(comparator_(key, search_key)) &&
                !(comparator_(search_key, key))) {
                break;
            }
            if (comparator_(search_key, key)) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        if (node != nullptr || size_ == 0) {
            return node;
        }
        return parent;
    }

    // Pointer to leftmost node
    [[nodiscard]] static Node* beginNode(Node* node) noexcept {
        if (node == nullptr) {
            return nullptr;
        }
        while (node->left != nullptr) {
            node = node->left;
        }
        return node;
    }
};

#endif  // IS_BTREE_MAP
