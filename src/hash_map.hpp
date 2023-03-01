// Hopscotch Hashing
// Based on http://mcg.cs.tau.ac.il/papers/disc2008-hopscotch.pdf

#pragma once

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <list>
#include <stdexcept>
#include <vector>

template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>>
class HashMap {
private:
    using ObjectType = std::pair<const KeyType, ValueType>;
    using SizeType = size_t;
    using DifferenceType = ptrdiff_t;

public:
    using iterator = typename std::list<ObjectType>::iterator;              // NOLINT
    using const_iterator = typename std::list<ObjectType>::const_iterator;  // NOLINT

    explicit HashMap(const Hash hasher = Hash()) : neighbourhood_size_(min_neighbourhood_size_), hasher_(hasher) {
        objects_.clear();
        buckets_.assign(neighbourhood_size_, Bucket{});
    }

    template <typename IteratorType>
    HashMap(IteratorType begin, IteratorType end, const Hash hasher = Hash()) : HashMap(hasher) {
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }

    HashMap(std::initializer_list<ObjectType> list, const Hash hasher = Hash())
        : HashMap(list.begin(), list.end(), hasher) {
    }

    HashMap(const HashMap& other) : HashMap(other.hasher_) {
        Reallocate(other.buckets_.size(), other.neighbourhood_size_);
        for (auto it = other.objects_.begin(); it != other.objects_.end(); ++it) {
            insert(*it);
        }
    }

    HashMap(HashMap&& other) : HashMap() {
        Swap(other);
    }

    HashMap& operator=(const HashMap& other) & {
        HashMap map(other);
        Swap(map);
        return *this;
    }
    HashMap& operator=(HashMap&& other) & {
        clear();
        Swap(other);
        return *this;
    }

    SizeType size() const {  // NOLINT
        return objects_.size();
    }

    bool empty() const {  // NOLINT
        return objects_.empty();
    }

    Hash hash_function() const {  // NOLINT
        return hasher_;
    }

    iterator insert(const ObjectType& value) {  // NOLINT
        BucketIterator it = FindObject(value.first);
        if (it != buckets_.end()) {
            return it->object_it_;
        }
        objects_.push_front(value);
        it = InsertObject(objects_.begin(), objects_.cbegin());
        if (it == buckets_.end()) {
            HandleCollision();
            it = FindObject(value.first);
        }
        return it->object_it_;
    }

    iterator insert(ObjectType&& value) {  // NOLINT
        BucketIterator it = FindObject(value.first);
        if (it != buckets_.end()) {
            return it->object_it_;
        }
        objects_.push_front(std::move(value));
        it = InsertObject(objects_.begin(), objects_.cbegin());
        if (it == buckets_.end()) {
            HandleCollision();
            it = FindObject(objects_.begin()->first);
        }
        return it->object_it_;
    }

    void erase(const KeyType& key) {  // NOLINT
        BucketIterator it = FindObject(key);
        if (it == buckets_.end()) {
            return;
        }
        objects_.erase(it->object_const_it_);
        EraseObject(it);
    }

    iterator begin() {  // NOLINT
        return objects_.begin();
    }

    iterator end() {  // NOLINT
        return objects_.end();
    }

    const_iterator begin() const {  // NOLINT
        return objects_.cbegin();
    }

    const_iterator end() const {  // NOLINT
        return objects_.cend();
    }

    iterator find(const KeyType& key) {  // NOLINT
        BucketIterator it = FindObject(key);
        if (it == buckets_.end()) {
            return end();
        }
        return it->object_it_;
    }

    const_iterator find(const KeyType& key) const {  // NOLINT
        ConstBucketIterator it = FindObject(key);
        if (it == buckets_.cend()) {
            return end();
        }
        return it->object_const_it_;
    }

    ValueType& operator[](const KeyType& key) {
        iterator it = insert({key, ValueType{}});
        return it->second;
    }

    ValueType& operator[](KeyType&& key) {
        iterator it = insert({std::move(key), ValueType{}});
        return it->second;
    }

    const ValueType& at(const KeyType& key) const {  // NOLINT
        ConstBucketIterator it = FindObject(key);
        if (it == buckets_.cend()) {
            throw std::out_of_range("404 Not found");
        }
        return it->object_const_it_->second;
    }

    void clear() {  // NOLINT
        neighbourhood_size_ = min_neighbourhood_size_;
        objects_.clear();
        buckets_.assign(min_neighbourhood_size_, Bucket{});
    }

private:
    static const SizeType NULL_DELTA = std::numeric_limits<SizeType>::max();
    static const SizeType EMPTY_BUCKET = std::numeric_limits<SizeType>::max();
    const SizeType size_modifier_ = 3;
    const SizeType neighbourhood_modifier_ = 3;
    const SizeType min_neighbourhood_size_ = 4;

    std::list<ObjectType> objects_;

    struct Bucket {
        iterator object_it_;
        const_iterator object_const_it_;
        SizeType first_delta_ = NULL_DELTA;
        SizeType next_delta_ = NULL_DELTA;
        SizeType prev_delta_ = NULL_DELTA;
        SizeType object_bucket_ = EMPTY_BUCKET;
    };

    using BucketIterator = typename std::vector<Bucket>::iterator;
    using ConstBucketIterator = typename std::vector<Bucket>::const_iterator;

    std::vector<Bucket> buckets_;

    SizeType neighbourhood_size_;  // Вряд ли станет больше 36, а если станет, то никакая таблица не прожует

    Hash hasher_;

    void HandleCollision() {
        try {
            bool handle = false;
            if (objects_.size() >= buckets_.size()) {
                handle = Reallocate(buckets_.size() * size_modifier_, neighbourhood_size_);
            }
            while (!handle) {
                handle = Reallocate((neighbourhood_size_ * neighbourhood_modifier_ >= buckets_.size() ||
                                             objects_.size() >= buckets_.size()
                                         ? buckets_.size() * size_modifier_
                                         : buckets_.size()),
                                    neighbourhood_size_ * neighbourhood_modifier_);
            }
        } catch (const std::bad_alloc& e) {
            throw e;
        }
    }

    bool Reallocate(SizeType new_capacity, SizeType new_neighbourhood_size) {
        if (new_capacity <= buckets_.size() && new_neighbourhood_size <= neighbourhood_size_) {
            return false;
        }
        try {
            buckets_.assign(new_capacity, Bucket{});
        } catch (const std::bad_alloc& e) {
            throw e;
        }
        neighbourhood_size_ = new_neighbourhood_size;
        iterator it = objects_.begin();
        const_iterator const_it = objects_.cbegin();
        for (; it != objects_.end() && const_it != objects_.cend(); ++it, ++const_it) {
            BucketIterator bucket_it = InsertObject(it, const_it);
            if (bucket_it == buckets_.end()) {
                return false;
            }
        }
        return true;
    }

    BucketIterator GetStartBucket(const KeyType& key) {
        return buckets_.begin() + hasher_(key) % buckets_.size();
    }

    ConstBucketIterator GetStartBucket(const KeyType& key) const {
        return buckets_.cbegin() + hasher_(key) % buckets_.size();
    }

    BucketIterator InsertObject(iterator object_it, const_iterator object_const_it) {
        BucketIterator start_bucket = GetStartBucket(object_const_it->first);
        BucketIterator free_bucket = start_bucket;
        while (free_bucket != buckets_.end() && free_bucket->object_bucket_ != EMPTY_BUCKET) {
            ++free_bucket;
        }
        if (free_bucket == buckets_.end()) {
            return buckets_.end();
        }
        while (neighbourhood_size_ <= static_cast<SizeType>(free_bucket - start_bucket)) {
            BucketIterator fit_bucket = free_bucket;
            while (fit_bucket != buckets_.begin() &&
                   neighbourhood_size_ > static_cast<SizeType>(free_bucket - fit_bucket) + 1) {
                if (neighbourhood_size_ >
                    static_cast<SizeType>(free_bucket - (buckets_.begin() + (fit_bucket - 1)->object_bucket_))) {
                    break;
                }
                --fit_bucket;
            }
            if (fit_bucket == buckets_.begin() ||
                static_cast<SizeType>(free_bucket - fit_bucket) + 1 >= neighbourhood_size_) {
                return buckets_.end();
            }
            --fit_bucket;
            if (fit_bucket->prev_delta_ != NULL_DELTA) {
                (fit_bucket - fit_bucket->prev_delta_)->next_delta_ += free_bucket - fit_bucket;
                fit_bucket->prev_delta_ += free_bucket - fit_bucket;
                std::swap(fit_bucket->prev_delta_, free_bucket->prev_delta_);
            }
            if (fit_bucket->next_delta_ != NULL_DELTA) {
                (fit_bucket + fit_bucket->next_delta_)->prev_delta_ -= free_bucket - fit_bucket;
                fit_bucket->next_delta_ -= free_bucket - fit_bucket;
                std::swap(fit_bucket->next_delta_, free_bucket->next_delta_);
            }
            if (fit_bucket == buckets_.begin() + fit_bucket->object_bucket_ +
                                  (buckets_.begin() + fit_bucket->object_bucket_)->first_delta_) {
                (buckets_.begin() + fit_bucket->object_bucket_)->first_delta_ =
                    free_bucket - (buckets_.begin() + fit_bucket->object_bucket_);
            }
            std::swap(fit_bucket->object_it_, free_bucket->object_it_);
            std::swap(fit_bucket->object_const_it_, free_bucket->object_const_it_);
            std::swap(fit_bucket->object_bucket_, free_bucket->object_bucket_);
            free_bucket = fit_bucket;
        }
        if (free_bucket < start_bucket) {
            return buckets_.end();
        }
        free_bucket->object_it_ = object_it;
        free_bucket->object_const_it_ = object_const_it;
        free_bucket->object_bucket_ = start_bucket - buckets_.begin();
        BucketIterator previous_bucket = free_bucket;
        while (previous_bucket != start_bucket &&
               (previous_bucket - 1)->object_bucket_ != free_bucket->object_bucket_) {
            --previous_bucket;
        }
        if (previous_bucket != start_bucket) {
            --previous_bucket;
            if (previous_bucket->next_delta_ != NULL_DELTA) {
                BucketIterator next_bucket = previous_bucket + previous_bucket->next_delta_;
                next_bucket->prev_delta_ = next_bucket - free_bucket;
                free_bucket->next_delta_ = next_bucket - free_bucket;
            }
            previous_bucket->next_delta_ = free_bucket - previous_bucket;
            free_bucket->prev_delta_ = free_bucket - previous_bucket;
        } else {
            if (start_bucket->first_delta_ != NULL_DELTA) {
                BucketIterator next_bucket = start_bucket + start_bucket->first_delta_;
                next_bucket->prev_delta_ = next_bucket - free_bucket;
                free_bucket->next_delta_ = next_bucket - free_bucket;
            }
            start_bucket->first_delta_ = free_bucket - start_bucket;
        }
        return free_bucket;
    }

    BucketIterator FindObject(const KeyType& key) {
        BucketIterator start_bucket = GetStartBucket(key);
        if (start_bucket->first_delta_ == NULL_DELTA) {
            return buckets_.end();
        }
        start_bucket += start_bucket->first_delta_;
        while (!(start_bucket->object_const_it_->first == key) && start_bucket->next_delta_ != NULL_DELTA) {
            start_bucket += start_bucket->next_delta_;
        }
        if (!(start_bucket->object_const_it_->first == key)) {
            return buckets_.end();
        }
        return start_bucket;
    }

    ConstBucketIterator FindObject(const KeyType& key) const {
        ConstBucketIterator start_bucket = GetStartBucket(key);
        if (start_bucket->first_delta_ == NULL_DELTA) {
            return buckets_.cend();
        }
        start_bucket += start_bucket->first_delta_;
        while (!(start_bucket->object_const_it_->first == key) && start_bucket->next_delta_ != NULL_DELTA) {
            start_bucket += start_bucket->next_delta_;
        }
        if (!(start_bucket->object_const_it_->first == key)) {
            return buckets_.cend();
        }
        return start_bucket;
    }

    void EraseObject(BucketIterator object_bucket) {
        if (object_bucket->prev_delta_ != NULL_DELTA) {
            BucketIterator previous_bucket = object_bucket - object_bucket->prev_delta_;
            if (object_bucket->next_delta_ != NULL_DELTA) {
                BucketIterator next_bucket = object_bucket + object_bucket->next_delta_;
                previous_bucket->next_delta_ = next_bucket - previous_bucket;
                next_bucket->prev_delta_ = next_bucket - previous_bucket;
            } else {
                previous_bucket->next_delta_ = NULL_DELTA;
            }
        } else {
            BucketIterator start_bucket = buckets_.begin() + object_bucket->object_bucket_;
            if (object_bucket->next_delta_ != NULL_DELTA) {
                BucketIterator next_bucket = object_bucket + object_bucket->next_delta_;
                start_bucket->first_delta_ = next_bucket - start_bucket;
                next_bucket->prev_delta_ = NULL_DELTA;
            } else {
                start_bucket->first_delta_ = NULL_DELTA;
            }
        }
        object_bucket->next_delta_ = NULL_DELTA;
        object_bucket->prev_delta_ = NULL_DELTA;
        object_bucket->object_bucket_ = EMPTY_BUCKET;
    }

    void Swap(HashMap& other) {
        std::swap(objects_, other.objects_);
        std::swap(buckets_, other.buckets_);
        std::swap(neighbourhood_size_, other.neighbourhood_size_);
        std::swap(hasher_, other.hasher_);
    }
};