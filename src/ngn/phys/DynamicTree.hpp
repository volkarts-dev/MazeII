// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "Shapes.hpp"
#include "Functions.hpp"
#include <entt/fwd.hpp>

namespace ngn {

class WorldObject;

class TreeNode
{
public:
    static constexpr uint32_t NullNode = std::numeric_limits<uint32_t>::max();

    bool isLeaf() const { return right == NullNode; }

    union
    {
        uint32_t parent;
        uint32_t nextFree{NullNode};
    };

    uint32_t left{NullNode};
    uint32_t right{NullNode};

    AABB aabb{};
    entt::entity entity{};

    uint16_t height{0};
    bool updated{};
};

class DynamicTree
{
public:
    DynamicTree(entt::registry* registry);

    bool initialize();

    uint32_t addObject(const AABB& aabb, entt::entity entity);
    bool updateObject(uint32_t ndoeId, const AABB& aabb);
    void removeObject(uint32_t nodeId);

    template<typename Callback>
    void walkTree(const Callback& callback) const;

    template<typename Callback>
    void query(const Shape& shape, const Callback& callback) const
    {
        return query(calculateAABB(shape), callback);
    }

    template<typename Callback>
    void query(const AABB& aabb, const Callback& callback) const;

    const TreeNode& node(uint32_t index) const
    {
        assert(index < nodes_.size());
        return nodes_[index];
    }

private:
    void insertLeaf(uint32_t index);
    void removeLeaf(uint32_t index);
    void updateLeaf(uint32_t index);
    void syncHierarchy(uint32_t index);
    uint32_t balance(uint32_t index);

    AABB enlargeAABB(AABB aabb) const;

    uint32_t capacity() const { return static_cast<uint32_t>(nodes_.size()); }
    void checkCapacity();
    uint32_t allocateNode();
    void deallocateNode(uint32_t index);

private:
    entt::registry* registry_;

    std::vector<TreeNode> nodes_;

    uint32_t rootIndex_;
    uint32_t firstFreeIndex_;
    uint32_t firstLeafIndex_;
};

// **********************************************

template<typename Callback>
void DynamicTree::walkTree(const Callback& callback) const
{
    // TODO do not use vector with dynamic allocations for this
    std::vector<uint32_t> stack;

    stack.emplace_back(rootIndex_);

    while (!stack.empty())
    {
        const auto index = stack.back();
        stack.pop_back();

        const TreeNode& node = nodes_[index];

        callback(node);

        if (!node.isLeaf())
        {
            stack.emplace_back(node.left);
            stack.emplace_back(node.right);
        }
    }
}

template<typename Callback>
void DynamicTree::query(const AABB& aabb, const Callback& callback) const
{
// TODO do not use vector with dynamic allocations for this
    std::vector<uint32_t> stack;

    stack.emplace_back(rootIndex_);

    while (!stack.empty())
    {
        const auto index = stack.back();
        stack.pop_back();

        const TreeNode& node = nodes_[index];

        if (intersects(aabb, node.aabb))
        {
            if (node.isLeaf())
            {
                if (!callback(node.entity, node.aabb))
                    return;
            }
            else
            {
                stack.emplace_back(node.left);
                stack.emplace_back(node.right);
            }
        }
    }
}

} // namespace ngn
