#ifndef COLLECTIONUTIL_H
#define COLLECTIONUTIL_H

#include <algorithm>
#include <vector>

namespace OccQtCore::CollectionUtil
{
    template <typename T>
    bool contains(const std::vector<T>& values, const T& value)
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    template <typename T>
    void addUnique(std::vector<T>& values, const T& value)
    {
        if (!contains(values, value))
        {
            values.push_back(value);
        }
    }

    template <typename T>
    void sortUnique(std::vector<T>& values)
    {
        std::sort(values.begin(), values.end());

        values.erase(
            std::unique(values.begin(), values.end()),
            values.end());
    }

    template <typename T, typename Predicate>
    const T* findPtr(
        const std::vector<T>& values,
        Predicate predicate)
    {
        const auto it =
            std::find_if(
                values.begin(),
                values.end(),
                predicate);

        if (it == values.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    template <typename T, typename Predicate>
    T* findPtr(
        std::vector<T>& values,
        Predicate predicate)
    {
        const auto it =
            std::find_if(
                values.begin(),
                values.end(),
                predicate);

        if (it == values.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    template <typename T, typename Predicate>
    void removeIf(
        std::vector<T>& values,
        Predicate predicate)
    {
        const auto it =
            std::remove_if(
                values.begin(),
                values.end(),
                predicate);

        values.erase(
            it,
            values.end());
    }

    template <typename T, typename Predicate>
    std::vector<T> takeIf(
        std::vector<T>& values,
        Predicate predicate)
    {
        std::vector<T> takenValues;

        auto it = values.begin();

        while (it != values.end())
        {
            if (predicate(*it))
            {
                takenValues.push_back(*it);
                it = values.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return takenValues;
    }
}

#endif // COLLECTIONUTIL_H
