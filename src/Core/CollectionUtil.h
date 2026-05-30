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
}

#endif // COLLECTIONUTIL_H
