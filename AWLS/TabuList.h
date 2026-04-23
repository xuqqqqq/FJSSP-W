//
// Created by qiming on 25-4-20.
//

#ifndef FJSP_TABULIST_H
#define FJSP_TABULIST_H
#include "NeighborhoodMove.h"
#include "Schedule.h"

#ifdef TABU_WITH_POS
/**
 * Represents a tabu list item containing:
 * - Operation ID
 * - Machine position (0-based index)
 */
struct TabuItem
{
    int op_id; // Operation identifier
    int machine_pos; // Position on machine (0-based)

    bool operator==(const TabuItem& other) const { return op_id == other.op_id && machine_pos == other.machine_pos; }
};

/**
 * Hash function specialization for TabuItem
 */
template <>
struct std::hash<TabuItem>
{
    std::size_t operator()(const TabuItem& item) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(item.op_id);
        std::size_t h2 = std::hash<int>{}(item.machine_pos);
        return h1 ^ (h2 << 1);
    }
};

/**
 * Tabu list implementation that prevents cycling by recording forbidden moves
 */
struct TabuList
{
    std::vector<std::unordered_map<TabuItem, unsigned long long>> tabu_list;

    /**
     * Constructor
     * @param machine_num Number of machines in the problem
     */
    explicit TabuList(int machine_num = 0) { tabu_list.resize(machine_num); }

    /**
     * Add an item to the tabu list
     * @param item Tabu item to add
     * @param value The iteration until which this move is tabu
     * @param machine_id Machine identifier
     */
    void add_item(const TabuItem& item, unsigned long long value, int machine_id = 0)
    {
        tabu_list[machine_id][item] = value;
    }

    /**
     * Check if an item exists in the tabu list
     * @return True if the item is currently tabu
     */
    [[nodiscard]] bool has_item(const TabuItem& item, int machine_id = 0) const
    {
        if (machine_id >= 0 && machine_id < tabu_list.size())
        {
            return tabu_list[machine_id].contains(item);
        }
        return false;
    }

    /**
     * Get the expiration iteration for a tabu item
     * @return The iteration when tabu status expires (0 if not tabu)
     */
    [[nodiscard]] unsigned long long get_item_value(const TabuItem& item, int machine_id = 0) const
    {
        if (has_item(item, machine_id))
        {
            return tabu_list[machine_id].at(item);
        }
        return 0;
    }

    /**
     * Add an entire sequence of operations to the tabu list
     * @param machine_id Machine identifier
     * @param start_pos Starting position on machine
     * @param sequence Vector of operation IDs
     * @param tabu_time Iteration until which these moves are tabu
     */
    void add_sequence(int machine_id, int start_pos, const std::vector<int>& sequence, unsigned long long tabu_time)
    {
        for (const int op : sequence)
        {
            tabu_list[machine_id][{op, start_pos++}] = tabu_time;
        }
    }

    /**
     * Check if a sequence is currently tabu
     * @return True if any operation in the sequence is still tabu
     */
    [[nodiscard]] bool is_tabu(int machine_id, int start_pos, const std::vector<int>& sequence,
        unsigned long long iteration) const
    {
        for (const int op : sequence)
        {
            if (get_item_value({ op, start_pos++ }, machine_id) < iteration)
            {
                return false;
            }
        }
        return true;
    }
};
#else
struct TabuItem
{
    std::vector<int> operations; // 禁忌的操作顺序

    // 重载相等运算符，用于哈希表比较
    bool operator==(const TabuItem& other) const { return operations == other.operations; }

    TabuItem() = default;
    explicit TabuItem(const std::vector<int>& operations) : operations(operations) {}
};

#define HASH1

namespace std
{
#ifdef HASH1
    template <>
    struct hash<TabuItem>
    {
        size_t operator()(const TabuItem& item) const
        {
            size_t seed = 0;
            // 组合vector中每个元素的哈希值
            for (const int& op : item.operations)
            {
                // 使用标准的哈希组合技术
                seed ^= hash<int>()(op) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
#elif HASH2
    template <>
    struct hash<TabuItem>
    {
        size_t operator()(const TabuItem& item) const
        {
            // FNV-1a哈希算法
            const size_t prime = 0x100000001b3;
            size_t hash = 0xcbf29ce484222325;

            for (const int& op : item.operations)
            {
                hash ^= static_cast<size_t>(op);
                hash *= prime;
            }
            return hash;
        }
    };
#elif HASH3
    template <>
    struct hash<TabuItem>
    {
        size_t operator()(const TabuItem& item) const
        {
            const auto& ops = item.operations;
            size_t size = ops.size();

            if (size == 0)
                return 0;

            // 小向量直接使用乘法哈希
            if (size <= 4)
            {
                size_t h = ops[0];
                for (size_t i = 1; i < size; ++i)
                {
                    h = h * 31 + ops[i];
                }
                return h;
            }

            // 较大向量采样哈希
            size_t h = size;
            h ^= ops[0] + ops[size - 1];
            h ^= ops[size / 2];
            h ^= ops[size / 3];
            h ^= ops[2 * size / 3];
            return h;
        }
    };
#elif HASH4
    template <>
    struct hash<TabuItem>
    {
        size_t operator()(const TabuItem& item) const
        {
            // 如果vector内部数据连续，可以直接对内存块进行哈希
            if (item.operations.empty())
                return 0;

            // 指向数据的指针
            const int* data = item.operations.data();
            // 数据大小（字节数）
            size_t bytes = item.operations.size() * sizeof(int);

            // MurmurHash2
            const uint64_t m = 0xc6a4a7935bd1e995ULL;
            const int r = 47;
            uint64_t h = 0x8445d61a4e774912ULL ^ (bytes * m);

            // 每次处理8字节
            const uint64_t* chunks = reinterpret_cast<const uint64_t*>(data);
            const uint64_t* end = chunks + (bytes / 8);

            while (chunks != end)
            {
                uint64_t k = *chunks++;
                k *= m;
                k ^= k >> r;
                k *= m;
                h ^= k;
                h *= m;
            }

            // 处理剩余字节
            const unsigned char* tail = reinterpret_cast<const unsigned char*>(chunks);
            switch (bytes & 7)
            {
            case 7:
                h ^= uint64_t(tail[6]) << 48;
                [[fallthrough]];
            case 6:
                h ^= uint64_t(tail[5]) << 40;
                [[fallthrough]];
            case 5:
                h ^= uint64_t(tail[4]) << 32;
                [[fallthrough]];
            case 4:
                h ^= uint64_t(tail[3]) << 24;
                [[fallthrough]];
            case 3:
                h ^= uint64_t(tail[2]) << 16;
                [[fallthrough]];
            case 2:
                h ^= uint64_t(tail[1]) << 8;
                [[fallthrough]];
            case 1:
                h ^= uint64_t(tail[0]);
                h *= m;
            };

            h ^= h >> r;
            h *= m;
            h ^= h >> r;

            return static_cast<size_t>(h);
        }
    };
#endif
} // namespace std

struct TabuList
{
    std::vector<std::unordered_map<TabuItem, unsigned long long>> tabu_list;

    /**
     * Constructor
     * @param machine_num Number of machines in the problem
     */
    explicit TabuList(int machine_num = 0) { tabu_list.resize(machine_num); }

    [[nodiscard]] bool is_tabu(int machine_id, const std::vector<int>& sequence, unsigned long long iteration) const
    {

        const auto it = tabu_list[machine_id].find(TabuItem{ sequence });
        if (it != tabu_list[machine_id].end())
        {
            return it->second >= iteration;
        }
        return false;
    }
    void add_sequence(int machine_id, const std::vector<int>& sequence, unsigned long long iteration)
    {
        tabu_list[machine_id].insert_or_assign(TabuItem{ sequence }, iteration);
    }
};
#endif
#endif // FJSP_TABULIST_H

