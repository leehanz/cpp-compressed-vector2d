#pragma once

#include <assert.h>
#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <array>
#include <sstream>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

namespace std
{

    /**
     * std::vector<std::vector<T>> is rather inefficient because each of the inner vectors
     * contains separately allocated heap memory.
     *
     * The vector2d class is designed to save the runtime for memory allocation and deallocation
     * of nested std::vector, which stores its elements contiguously in a vector, and provides
     * interfaces like std::vector for the runtime-resizability.
     *
     * It has O(1) constant time complexity of random access, and it will return row_type and
     * row_type iterator (std::vector<row_type>::iterator) when we access or iterate it.
     */
    template <typename T>
    class vector2d
    {

    public:

        /**
         * The inner vector of vector2d is represented by row_type class.
         * It shouldn't be used without vector2d.
         */
        class row_type
        {

        public:

            class ctor_passkey
            {
            private:

                ctor_passkey() {}
                friend vector2d;
            };

            row_type() = delete;
            row_type(vector2d* container, ctor_passkey const&)
                : m_container(container)
                , m_begin_index(0)
                , m_size(0)
                , m_capacity(0)
            {}
            row_type(vector2d* container, size_t size, ctor_passkey const&)
                : m_container(container)
                , m_begin_index(0)
                , m_size(size)
                , m_capacity(size)
            {}
            row_type(vector2d* container, size_t istart, size_t size, ctor_passkey const&)
                : m_container(container)
                , m_begin_index(istart)
                , m_size(size)
                , m_capacity(size)
            {}

            size_t begin_index() const { return m_begin_index; }
            size_t end_index() const { return m_begin_index + m_size; }

            /* begin vector-like methods */

            // Iterators
            using iterator = typename std::vector<T>::iterator;
            using const_iterator = typename std::vector<T>::const_iterator;
            iterator begin() noexcept { return m_container->m_data.begin() + m_begin_index; }
            iterator end() noexcept { return m_container->m_data.begin() + m_begin_index + m_size; }
            const_iterator begin() const noexcept { return m_container->m_data.begin() + m_begin_index; }
            const_iterator end() const noexcept { return m_container->m_data.begin() + m_begin_index + m_size; }
            const_iterator cbegin() const noexcept { return m_container->m_data.begin() + m_begin_index; }
            const_iterator cend() const noexcept { return m_container->m_data.begin() + m_begin_index + m_size; }

            // Element access
            T& at(size_t index) { return m_container->m_data.at(m_begin_index + index); }
            T const& at(size_t index) const { return m_container->m_data.at(m_begin_index + index); }
            T& operator[](size_t index) { return m_container->m_data[m_begin_index + index]; }
            T const& operator[](size_t index) const { return m_container->m_data[m_begin_index + index]; }
            T& front() { return m_container->m_data.at(m_begin_index); }
            T const& front() const { return m_container->m_data.at(m_begin_index); }
            T& back() { return m_container->m_data.at(m_begin_index + m_size - 1); }
            T const& back() const { return m_container->m_data.at(m_begin_index + m_size - 1); }

            // Capacity
            size_t size() const noexcept { return m_size; }
            size_t max_size() const noexcept { return m_container->m_data.max_size(); }
            bool empty() const { return m_size == 0; }
            size_t capacity() const noexcept { return m_capacity; }
            void reserve(size_t size);
            void shrink_to_fit() { m_capacity = m_size; }

            // Modifiers
            void clear()
            {
                if (this->empty()) { return; }
                m_size = 0;
                // Destruct elements in this row.
                for (auto it = this->begin(); it != this->end(); ++it) { m_container->destroy(*it); }
            }

            template <class InputIt>
            iterator insert(const_iterator pos, InputIt first, InputIt last);
            iterator insert(iterator pos, const T& value);

            iterator erase(const_iterator first, const_iterator last);
            iterator erase(const_iterator pos);

            void push_back(T const& value);

            void pop_back()
            {
                // Destruct last element in this row.
                m_container->destroy(*(this->end() - 1));
                // Reduce the size but keep the capacity as before.
                this->update(this->begin_index(), this->size() - 1);
            }

            void resize(size_t size) { this->resize(size, T()); }

            void resize(size_t size, T const& value);

            // TODO: implement the following interfaces if needed.
            // template <class... Args> iterator emplace(const_iterator pos, Args &&... args);
            // template <class... Args> void emplace_back(Args &&... args);
            // void shrink_to_fit();
            // void swap(std::vector<T> & other);

            /* end vector-like methods */

            void update(size_t ibegin, size_t size)
            {
                m_begin_index = ibegin;
                m_size = size;
                m_capacity = std::max(m_size, m_capacity);
            }

            void reset(vector2d* container) { m_container = container; }

        private:

            vector2d* m_container = nullptr;

            size_t m_begin_index = 0;
            size_t m_size = 0;
            size_t m_capacity = 0;

        }; /* end class row_type */

        /* begin construction methods */
        vector2d(const size_t& nrow)
        {
            if (nrow == 0)
            {
                std::ostringstream ms;
                ms << "ollib::vector2d(): input nrow " << nrow << " cannot be zero";
                throw std::out_of_range(ms.str());
            }
            m_rows.assign(nrow, row_type(this, 0, 0, typename row_type::ctor_passkey()));
        }

        vector2d(const size_t& nrow, const size_t& ncol)
        {
            if (nrow == 0 || ncol == 0)
            {
                std::ostringstream ms;
                ms << "ollib::vector2d(): input nrow " << nrow << " or ncol " << ncol << " cannot be zero";
                throw std::out_of_range(ms.str());
            }
            m_rows.reserve(nrow);
            m_data.reserve(nrow * ncol);

            std::vector<T> columns(ncol);
            for (size_t i = 0; i < nrow; ++i) { push_back(columns); }
        }
        /* end construction methods */

        vector2d() = default;
        vector2d(vector2d const& other)
        {
            m_data = other.m_data;
            m_rows = other.m_rows;
            for (auto& row : m_rows) { row.reset(this); }
        }
        vector2d(vector2d&& other)
        {
            m_data = std::move(other.m_data);
            m_rows = std::move(other.m_rows);
            for (auto& row : m_rows) { row.reset(this); }
        }
        vector2d& operator=(vector2d const& other)
        {
            m_data = other.m_data;
            m_rows = other.m_rows;
            for (auto& row : m_rows) { row.reset(this); }
        }
        vector2d& operator=(vector2d&& other)
        {
            m_data = std::move(other.m_data);
            m_rows = std::move(other.m_rows);
            for (auto& row : m_rows) { row.reset(this); }
        }

        /* begin vector-like methods */

        // Iterators
        using row_iterator = typename std::vector<row_type>::iterator;
        using const_row_iterator = typename std::vector<row_type>::const_iterator;
        row_iterator begin() noexcept { return m_rows.begin(); }
        row_iterator end() noexcept { return m_rows.end(); }
        const_row_iterator begin() const noexcept { return m_rows.begin(); }
        const_row_iterator end() const noexcept { return m_rows.end(); }
        const_row_iterator cbegin() const noexcept { return m_rows.begin(); }
        const_row_iterator cend() const noexcept { return m_rows.end(); }

        // Element access
        row_type& at(size_t index) { return m_rows.at(index); }
        row_type const& at(size_t index) const { return m_rows.at(index); }
        row_type& operator[](size_t index) { return m_rows[index]; }
        row_type const& operator[](size_t index) const { return m_rows[index]; }
        row_type& front() { return m_rows.front(); }
        row_type const& front() const { return m_rows.front(); }
        row_type& back() { return m_rows.back(); }
        row_type const& back() const { return m_rows.back(); }

        // Capacity
        size_t size() const noexcept { return m_rows.size(); }
        size_t max_size() const noexcept { return m_rows.max_size(); }
        size_t empty() const { return m_rows.empty(); }
        size_t capacity() const noexcept { return m_rows.capacity(); }
        void reserve(size_t size) { m_rows.reserve(size); }

        // Modifiers
        void clear()
        {
            for (auto& row : m_rows) { row.clear(); }
            m_data.clear();
            m_rows.clear();
        }

        row_iterator insert(const_row_iterator pos, row_iterator first, row_iterator last);
        row_iterator insert(row_iterator pos, row_type const& row);

        row_iterator erase(const_row_iterator first, const_row_iterator last);
        row_iterator erase(const_row_iterator pos);

        void push_back(std::initializer_list<T> const& arr);

        void push_back(std::vector<T> const& arr);

        void pop_back()
        {
            m_rows.back().clear();
            m_rows.pop_back();
        }

        void resize(size_t size)
        {
            // Resize with empty Row.
            m_rows.resize(size, row_type(this, typename row_type::ctor_passkey()));
        }

        void resize(size_t size, row_type const& row);

        // TODO: implement the following interfaces if needed.
        // template <class... Args> row_iterator emplace(const_row_iterator pos, Args &&... args);
        // template <class... Args> void emplace_back(Args &&... args);
        // void shrink_to_fit();
        // void swap(vector2d<T> & other);

        /* end vector-like methods */

        size_t nelement()
        {
            auto acc_func = [](size_t accumulator, row_type const& r)
            { return accumulator + r.size(); };
            return std::accumulate(m_rows.begin(), m_rows.end(), 0, acc_func);
        }

        void compact()
        {
            std::vector<T> data;
            data.reserve(nelement());

            for (auto& row : m_rows)
            {
                if (row.empty()) { continue; }
                for (auto it = row.begin(); it != row.end(); ++it)
                {
                    data.emplace_back(*it);
                }
                size_t const size = row.size();
                row.shrink_to_fit();
                row.update(data.size() - size, size);
            }

            m_data.swap(data);
        }

        void print()
        {
            std::cout << "vector2d: " << std::endl;
            std::cout << "nrow: " << this->size() << std::endl;
            std::cout << "nelement: " << this->m_data.size() << std::endl;
            for (size_t n = 0; n < m_rows.size(); ++n)
            {
                std::cout << "row[" << n << "]: " <<  "[";
                for (size_t i = m_rows[n].begin_index(); i < m_rows[n].end_index(); ++i)
                {
                    std::cout << " " << m_data[i];
                }
                std::cout << "] size:" << m_rows[n].size() << " capacity:" << m_rows[n].capacity() << " begin:" << m_rows[n].begin_index()
                    << std::endl;
            }

            for (auto& data : m_data)
            {
                std::cout << data << " ";
            }
        }

    private:

        void append(size_t count, T const& value);
        void move_desc(size_t ibegin, size_t iend, size_t new_ibegin);
        void move_asc(size_t ibegin, size_t iend, size_t new_ibegin);

        // calling destructor of value type
        void destroy(size_t index) { this->destroy(m_data[index]); }
        void destroy(T& element) { m_data.get_allocator().destroy(&element); }

        std::vector<row_type> m_rows;
        std::vector<T> m_data;

    }; /* end class vector2d */

    template <class T>
    void vector2d<T>::row_type::reserve(size_t size)
    {
        if (size <= m_capacity) { return; }

        if (this->end() != m_container->m_data.end())
        {
            m_container->append(size, T());
            m_container->move_desc(this->begin_index(),
                this->begin_index() + this->capacity() - 1,
                m_container->m_data.size() - size);
            // TODO: unused memory could be the capacity of previous/next Row.

            this->update(m_container->m_data.size() - size, this->size());
        }
        else
        {
            m_container->append(size - this->size(), T());
        }
        m_capacity = size;
    }

    /**
     * There are basically 3 situation while inserting element in our case:
     * 1. The capacity is enough: We move the elements after the insertion pos backwardly and fill the range.
     * 2. The capacity is not enough and the row is not the last one: We append required spaces (nelement of row + insertion size) to the end,
     *    we move the elements before insertion pos backwardly, then fill the range, finally move the remaining of elements to the back.
     * 3. The capacity is not enough and the row is the last one: We append required spaces (insertion size) to the end, then
     *    move elements after the insertion pos backwardly and fill the range.
     *
     * Time complexity: O(m), where m = size of the row plus reallocation if required.
     */
    template <class T>
    template <class InputIt>
    typename std::vector<T>::iterator vector2d<T>::row_type::insert(const_iterator pos, InputIt first, InputIt last)
    {
        size_t const diff = pos - this->begin();
        if (diff > this->size() || diff < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::row_type::insert(): input pos " << diff << " is out of range.";
            throw std::out_of_range(ms.str());
        }

        size_t const range = last - first;
        if (range < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::row_type::insert(): input range is invalid.";
            throw std::out_of_range(ms.str());
        }

        // Return pos if first==last.
        if (range == 0) { return this->begin() + diff; }

        if (this->size() + range <= this->capacity())
        {
            m_container->move_desc(this->begin_index() + diff,
                this->end_index() - 1,
                this->begin_index() + diff + range);

            for (auto it = first; it != last; ++it)
            {
                size_t pos = it - first;
                m_container->m_data[this->begin_index() + diff + pos] = *it;
            }

            this->update(this->begin_index(), this->size() + range);
        }
        // When this->size() + range > this->capacity()
        else
        {
            // When the row is not at the end.
            if (this->end() != m_container->m_data.end())
            {
                m_container->append(this->size() + range, T());
                m_container->move_desc(this->begin_index(),
                    this->begin_index() + diff - 1,
                    m_container->m_data.size() - this->size() - range);

                for (auto it = first; it != last; ++it)
                {
                    size_t pos = it - first;
                    m_container->m_data[m_container->m_data.size() - this->size() - range + diff + pos] = *it;
                }

                m_container->move_desc(this->begin_index() + diff,
                    this->end_index() - 1,
                    m_container->m_data.size() - this->size() + diff);

                // TODO: unused memory could be the capacity of previous/next Row.

                this->update(m_container->m_data.size() - this->size() - range, this->size() + range);
            }
            // When the row is at the end.
            else
            {
                m_container->append(range, T());
                m_container->move_desc(this->begin_index() + diff,
                    this->end_index() - 1,
                    this->begin_index() + diff + range);

                for (auto it = first; it != last; ++it)
                {
                    size_t pos = it - first;
                    m_container->m_data[this->begin_index() + diff + pos] = *it;
                }

                this->update(this->begin_index(), this->size() + range);
            }
        }

        return this->begin() + diff;
    }

    template <class T>
    typename std::vector<T>::iterator vector2d<T>::row_type::insert(iterator pos, const T& value)
    {
        std::array<T, 1> tmp{ value };
        return this->insert(pos, tmp.begin(), tmp.end());
    }

    /**
     * In erase(), we only have to move the elements after the last erase pos forwardly, and leave the space
     * there to be capacity of this row.
     *
     * Time complexity: O(m), where m = size of the row.
     */
    template <class T>
    typename std::vector<T>::iterator vector2d<T>::row_type::erase(const_iterator first, const_iterator last)
    {
        size_t const fdiff = first - this->begin();
        if (fdiff > this->size() || fdiff < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::row_type::erase(): input fisrt " << fdiff << " is out of range.";
            throw std::out_of_range(ms.str());
        }

        size_t const ldiff = last - this->begin();
        if (ldiff > this->size() || ldiff < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::row_type::erase(): input last " << ldiff << " is out of range.";
            throw std::out_of_range(ms.str());
        }

        size_t const range = last - first;
        if (range < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::row_type::erase(): input range is invalid.";
            throw std::out_of_range(ms.str());
        }

        // Return last if first==last.
        if (range == 0) { return this->begin() + ldiff; }

        // Destruct elements in [first, last)
        for (auto it = this->begin() + fdiff; it != this->begin() + ldiff; ++it) { m_container->destroy(*it); }

        m_container->move_asc(this->begin_index() + ldiff,
            this->end_index(),
            this->begin_index() + ldiff - range);

        this->update(this->begin_index(), this->size() - range);

        return this->begin() + fdiff;
    }

    template <class T>
    typename std::vector<T>::iterator vector2d<T>::row_type::erase(const_iterator pos)
    {
        size_t const diff = pos - begin();
        return this->erase(begin() + diff, begin() + diff + 1);
    }

    /**
     * The idea of push_back is similar to insert(), it's worth noting that we call emplace_back
     * when the row is empty or is already the last one.
     *
     * Time complexity: O(m), where m = size of the row.
     */
    template <class T>
    void vector2d<T>::row_type::push_back(T const& value)
    {
        if (this->size() < this->capacity())
        {
            m_container->m_data[this->end_index()] = value;
            this->update(this->begin_index(), this->size() + 1);
        }
        else
        {
            // When this->size() == this->capacity()
            if (!this->empty() && this->end_index() != m_container->m_data.size())
            {
                // Append elements of row to the end of m_data.
                m_container->append(this->size() + 1, T());
                // Move the original elements to new positions.
                m_container->move_desc(this->begin_index(),
                    this->end_index() - 1,
                    m_container->m_data.size() - this->size() - 1);

                // TODO: unused memory could be the capacity of previous/next Row.

                // Fill the new element.
                m_container->m_data[m_container->m_data.size() - 1] = value;
            }
            else
            {
                m_container->m_data.emplace_back(value);
            }

            this->update(m_container->m_data.size() - this->size() - 1, this->size() + 1);
        }
    }

    /**
     * There are 3 situations in resize():
     * 1. Request smaller size: We only shrink the size and leave the remaining space as the capacity of this row.
     * 2. Request bigger size and the capacity is enough: Fill the range of value in the capacity.
     * 3. Request bigger size and the capacity is not enough, we append the size of elements to the end, then
     *    move the original elements and fill the remaining elements with value.
     *
     * Time complexity: O(m), where m = size of the row plus reallocation if required.
     */
    template <class T>
    void vector2d<T>::row_type::resize(size_t size, T const& value)
    {
        if (size <= this->size())
        {
            // Reduce the size but keep the capacity as before.
            this->update(this->begin_index(), size);
            return;
        }

        if (size <= this->capacity())
        {
            for (size_t i = this->end_index(); i < this->end_index() + size - this->size(); ++i)
            {
                m_container->m_data[i] = value;
            }
            this->update(this->begin_index(), size);
        }
        else
        {
            m_container->append(size, value);

            if (!this->empty())
            {
                m_container->move_desc(this->begin_index(),
                    this->begin_index() + this->size(),
                    m_container->m_data.size() - size);
                // TODO: unused memory in the original position could be the capacity of previous/next row.
                for (size_t i = m_container->m_data.size() - size + this->size(); i < m_container->m_data.size(); ++i)
                {
                    m_container->m_data[i] = value;
                }
            }

            this->update(m_container->m_data.size() - size, size);
        }
    }

    template <class T>
    void vector2d<T>::push_back(std::initializer_list<T> const& arr)
    {
        auto nelement = arr.end() - arr.begin();
        // Add new row
        m_rows.emplace_back(this, m_data.size(), nelement, typename row_type::ctor_passkey());
        // Insert range data to m_data
        m_data.reserve(m_data.size() + nelement);

        for (auto it = arr.begin(); it != arr.end(); ++it)
        {
            m_data.emplace_back(*it);
        }
    }

    template <class T>
    void vector2d<T>::push_back(std::vector<T> const& arr)
    {
        auto nelement = arr.end() - arr.begin();
        // Add new row
        m_rows.emplace_back(this, m_data.size(), nelement, typename row_type::ctor_passkey());
        // Insert range data to m_data
        m_data.reserve(m_data.size() + nelement);

        for (auto it = arr.begin(); it != arr.end(); ++it)
        {
            m_data.emplace_back(*it);
        }
    }

    template <class T>
    typename std::vector<typename vector2d<T>::row_type>::iterator
        vector2d<T>::insert(const_row_iterator pos, row_iterator first, row_iterator last)
    {
        row_iterator rit;
        size_t const diff = pos - begin();
        for (auto it = last - 1; it != first - 1; --it) { rit = this->insert(begin() + diff, *it); }
        return rit;
    }

    template <class T>
    typename std::vector<typename vector2d<T>::row_type>::iterator
        vector2d<T>::insert(row_iterator pos, row_type const& row)
    {
        row_type tmp_row = row_type(this, typename row_type::ctor_passkey());
        tmp_row.update(m_data.size(), row.size());
        m_data.reserve(m_data.size() + row.size());

        for (auto it = row.begin(); it != row.end(); ++it)
        {
            m_data.emplace_back(*it);
        }

        row_iterator rit = m_rows.insert(pos, tmp_row);

        return rit;
    }

    // TODO: unused memory could be the capacity of previous/next Row.
    template <class T>
    typename std::vector<typename vector2d<T>::row_type>::iterator
        vector2d<T>::erase(const_row_iterator first, const_row_iterator last)
    {
        size_t const fdiff = first - this->begin();
        if (fdiff > this->size() || fdiff < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::erase(): input fisrt " << fdiff << " is out of range.";
            throw std::out_of_range(ms.str());
        }

        size_t const ldiff = last - this->begin();
        if (ldiff > this->size() || ldiff < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::erase(): input last " << ldiff << " is out of range.";
            throw std::out_of_range(ms.str());
        }

        size_t const range = last - first;
        if (range < 0)
        {
            std::ostringstream ms;
            ms << "ollib::vector2d::erase(): input range is invalid.";
            throw std::out_of_range(ms.str());
        }

        // Return last if first==last.
        if (range == 0) { return this->begin() + ldiff; }

        for (auto it = this->begin() + fdiff; it != this->begin() + ldiff; ++it) { it->clear(); }

        row_iterator rit = m_rows.erase(first, last);

        return rit;
    }

    template <class T>
    typename std::vector<typename vector2d<T>::row_type>::iterator
        vector2d<T>::erase(const_row_iterator pos)
    {
        size_t const diff = pos - begin();
        return this->erase(begin() + diff, begin() + diff + 1);
    }

    /**
     * There are 2 situations in resize() of vector2d:
     * 1. Request smaller size: We only resize the row vector, leave the momory deleted row as garbage.
     * 2. Request bigger size: We append n (size) * m (nelement of row) elements to m_data, and update the index of rows.
     */
    template <class T>
    void vector2d<T>::resize(size_t size, row_type const& row)
    {
        size_t const nrow = m_rows.size();
        row_type r = row;
        r.reset(this);
        m_rows.resize(size, r);

        if (size <= nrow) { return; }

        size_t const diff = size - nrow;
        size_t const extra = diff * row.size();
        size_t const total = m_data.size() + extra;

        // Append n (size) * m (row.size()) elements of value type to m_data.
        m_data.reserve(total);

        // Update new rows.
        size_t const begin_index = m_data.size();
        for (size_t i = nrow; i < size; ++i)
        {
            size_t index = begin_index + (i - nrow) * row.size();
            m_rows[i].update(index, row.size());

            for (auto it = row.begin(); it != row.end(); ++it)
            {
                m_data.emplace_back(*it);
            }
        }
    }

    template <class T>
    inline void vector2d<T>::append(size_t count, T const& value)
    {
        m_data.resize(m_data.size() + count, value);
    }

    template <class T>
    inline void vector2d<T>::move_desc(size_t ibegin, size_t iend, size_t new_ibegin)
    {
        for (int i = static_cast<int>(iend - ibegin); i >= 0; --i)
        {
            m_data[new_ibegin + i] = std::move(m_data[ibegin + i]);
        }
    }

    template <class T>
    inline void vector2d<T>::move_asc(size_t ibegin, size_t iend, size_t new_ibegin)
    {
        for (int i = 0; i < static_cast<int>(iend - ibegin); ++i)
        {
            m_data[new_ibegin + i] = std::move(m_data[ibegin + i]);
        }
    }

}  // namespace ollib
