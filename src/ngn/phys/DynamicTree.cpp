// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#include "DynamicTree.hpp"

namespace ngn {

DynamicTree::DynamicTree(entt::registry* registry) :
    registry_{registry},
    rootIndex_(TreeNode::NullNode),
    firstFreeIndex_(TreeNode::NullNode),
    firstLeafIndex_(TreeNode::NullNode)
{
    checkCapacity();
}

bool DynamicTree::initialize()
{
    return true;
}

uint32_t DynamicTree::addObject(const AABB& aabb, entt::entity entity)
{
    const auto index = allocateNode();
    TreeNode& node = nodes_[index];

    node.aabb = enlargeAABB(aabb);
    node.entity = entity;

    insertLeaf(index);

    return index;
}

bool DynamicTree::updateObject(uint32_t treeNode, const AABB& aabb)
{
    assert(treeNode < nodes_.size());

    TreeNode& node = nodes_[treeNode];

    if (contains(node.aabb, aabb))
        return false;

    node.aabb = enlargeAABB(aabb);

    updateLeaf(treeNode);

    return true;
}

void DynamicTree::removeObject(uint32_t treeNode)
{
    assert(treeNode < nodes_.size());

    removeLeaf(treeNode);
    deallocateNode(treeNode);
}

void DynamicTree::insertLeaf(uint32_t index)
{
    auto* newNode = &nodes_[index];

    assert(newNode->parent == TreeNode::NullNode);
    assert(newNode->left == TreeNode::NullNode);
    assert(newNode->right == TreeNode::NullNode);

    if (rootIndex_ == TreeNode::NullNode)
    {
        rootIndex_ = index;
        return;
    }

    auto treeNodeIndex = rootIndex_;
    while (!nodes_[treeNodeIndex].isLeaf())
    {
        const TreeNode& treeNode = nodes_[treeNodeIndex];

        const TreeNode& leftNode = nodes_[treeNode.left];
        const TreeNode& rightNode = nodes_[treeNode.right];

        AABB combinedAABB = combine(treeNode.aabb, newNode->aabb);

        float newParentNodeCost = 2.0f * area(combinedAABB);
        float minimumPushDownCost = 2.0f * (area(combinedAABB) - area(treeNode.aabb));

        float costLeft{};
        float costRight{};

        if (leftNode.isLeaf())
        {
            costLeft = area(combine(newNode->aabb, leftNode.aabb)) + minimumPushDownCost;
        }
        else
        {
            AABB newLeftAABB = combine(newNode->aabb, leftNode.aabb);
            costLeft = (area(newLeftAABB) - area(leftNode.aabb)) + minimumPushDownCost;
        }

        if (rightNode.isLeaf())
        {
            costRight = area(combine(newNode->aabb, rightNode.aabb)) + minimumPushDownCost;
        }
        else
        {
            AABB newRightAABB = combine(newNode->aabb, rightNode.aabb);
            costRight = (area(newRightAABB) - area(rightNode.aabb)) + minimumPushDownCost;
        }

        // if the cost of creating a new parent node here is less than descending in either direction then
        // we know we need to create a new parent node, errrr, here and attach the leaf to that
        if (newParentNodeCost < costLeft && newParentNodeCost < costRight)
            break;

        // otherwise descend in the cheapest direction
        if (costLeft < costRight)
            treeNodeIndex = treeNode.left;
        else
            treeNodeIndex = treeNode.right;
    }

    uint32_t leafSiblingIndex = treeNodeIndex;
    uint32_t newParentIndex = allocateNode();

    // reload all references as allocateNode() can recreate/reallocate the nodes_ array
    auto* leafSibling = &nodes_[leafSiblingIndex];
    auto* newParent = &nodes_[newParentIndex];
    newNode = &nodes_[index];

    uint32_t oldParentIndex = leafSibling->parent;
    newParent->parent = oldParentIndex;
    newParent->aabb = combine(newNode->aabb, leafSibling->aabb);
    newParent->left = leafSiblingIndex;
    newParent->right = index;
    newNode->parent = newParentIndex;
    leafSibling->parent = newParentIndex;

    if (oldParentIndex == TreeNode::NullNode)
    {
        rootIndex_ = newParentIndex;
    }
    else
    {
        TreeNode& oldParent = nodes_[oldParentIndex];
        if (oldParent.left == leafSiblingIndex)
            oldParent.left = newParentIndex;
        else
            oldParent.right = newParentIndex;
    }

    syncHierarchy(newNode->parent);
}

void DynamicTree::removeLeaf(uint32_t index)
{
    // if the leaf is the root then we can just clear the root pointer and return
    if (index == rootIndex_)
    {
        rootIndex_ = TreeNode::NullNode;
        return;
    }

    TreeNode& leafNode = nodes_[index];
    uint32_t parentNodeIndex = leafNode.parent;
    const TreeNode& parentNode = nodes_[parentNodeIndex];
    uint32_t grandParentNodeIndex = parentNode.parent;
    uint32_t siblingNodeIndex = parentNode.left == index ? parentNode.right : parentNode.left;
    assert(siblingNodeIndex != TreeNode::NullNode); // we must have a sibling
    TreeNode& siblingNode = nodes_[siblingNodeIndex];

    if (grandParentNodeIndex != TreeNode::NullNode)
    {
        TreeNode& grandParentNode = nodes_[grandParentNodeIndex];
        if (grandParentNode.left == parentNodeIndex)
        {
            grandParentNode.left = siblingNodeIndex;
        }
        else
        {
            grandParentNode.right = siblingNodeIndex;
        }
        siblingNode.parent = grandParentNodeIndex;
        deallocateNode(parentNodeIndex);

        syncHierarchy(grandParentNodeIndex);
    }
    else
    {
        // if we have no grandparent then the parent is the root and so our sibling becomes the root and has it's parent removed
        rootIndex_ = siblingNodeIndex;
        siblingNode.parent = TreeNode::NullNode;
        deallocateNode(parentNodeIndex);
    }

    leafNode.parent = TreeNode::NullNode;
}

void DynamicTree::updateLeaf(uint32_t index)
{
    removeLeaf(index);
    insertLeaf(index);
}

void DynamicTree::syncHierarchy(uint32_t index)
{
    while (index != TreeNode::NullNode)
    {
        index = balance(index);

        uint32_t left = nodes_[index].left;
        uint32_t right = nodes_[index].right;

        nodes_[index].height = 1 + glm::max(nodes_[left].height, nodes_[right].height);
        nodes_[index].aabb = combine(nodes_[left].aabb, nodes_[right].aabb);

        index = nodes_[index].parent;
    }
}

uint32_t DynamicTree::balance(uint32_t index)
{
    assert(index != TreeNode::NullNode);

    TreeNode& A = nodes_[index];
    if (A.isLeaf() || A.height < 2)
    {
        return index;
    }

    uint32_t iB = A.left;
    uint32_t iC = A.right;
    assert(iB < capacity());
    assert(iC < capacity());

    TreeNode& B = nodes_[iB];
    TreeNode& C = nodes_[iC];

    int16_t balance = static_cast<int16_t>(C.height - B.height);

    // Rotate C up
    if (balance > 1)
    {
        uint32_t iF = C.left;
        uint32_t iG = C.right;
        assert(iF < capacity());
        assert(iG < capacity());
        TreeNode& F = nodes_[iF];
        TreeNode& G = nodes_[iG];

        // Swap A and C
        C.left = index;
        C.parent = A.parent;
        A.parent = iC;

        // A's old parent should point to C
        if (C.parent != TreeNode::NullNode)
        {
            if (nodes_[C.parent].left == index)
            {
                nodes_[C.parent].left = iC;
            }
            else
            {
                assert(nodes_[C.parent].right == index);
                nodes_[C.parent].right = iC;
            }
        }
        else
        {
            rootIndex_ = iC;
        }

        // Rotate
        if (F.height > G.height)
        {
            C.right = iF;
            A.right = iG;
            G.parent = index;
            A.aabb = combine(B.aabb, G.aabb);
            C.aabb = combine(A.aabb, F.aabb);

            A.height = 1 + glm::max(B.height, G.height);
            C.height = 1 + glm::max(A.height, F.height);
        }
        else
        {
            C.right = iG;
            A.right = iF;
            F.parent = index;
            A.aabb = combine(B.aabb, F.aabb);
            C.aabb = combine(A.aabb, G.aabb);

            A.height = 1 + glm::max(B.height, F.height);
            C.height = 1 + glm::max(A.height, G.height);
        }

        return iC;
    }

    // Rotate B up
    if (balance < -1)
    {
        uint32_t iD = B.left;
        uint32_t iE = B.right;
        assert(iD < capacity());
        assert(iE < capacity());
        TreeNode& D = nodes_[iD];
        TreeNode& E = nodes_[iE];

        // Swap A and B
        B.left = index;
        B.parent = A.parent;
        A.parent = iB;

        // A's old parent should point to B
        if (B.parent != TreeNode::NullNode)
        {
            if (nodes_[B.parent].left == index)
            {
                nodes_[B.parent].left = iB;
            }
            else
            {
                assert(nodes_[B.parent].right == index);
                nodes_[B.parent].right = iB;
            }
        }
        else
        {
            rootIndex_ = iB;
        }

        // Rotate
        if (D.height > E.height)
        {
            B.right = iD;
            A.left = iE;
            E.parent = index;
            A.aabb = combine(C.aabb, E.aabb);
            B.aabb = combine(A.aabb, D.aabb);

            A.height = 1 + glm::max(C.height, E.height);
            B.height = 1 + glm::max(A.height, D.height);
        }
        else
        {
            B.right = iE;
            A.left = iD;
            D.parent = index;
            A.aabb = combine(C.aabb, D.aabb);
            B.aabb = combine(A.aabb, E.aabb);

            A.height = 1 + glm::max(C.height, D.height);
            B.height = 1 + glm::max(A.height, E.height);
        }

        return iB;
    }

    return index;
}

AABB DynamicTree::enlargeAABB(AABB aabb) const
{
    // aabb.extend(glm::vec2{glm::max(aabb.width(), aabb.height()) * 0.1f});
    aabb.extend(glm::vec2{10.f});
    return aabb;
}

void DynamicTree::checkCapacity()
{
    if (firstFreeIndex_ == TreeNode::NullNode)
    {
        size_t oldSize = nodes_.size();

        nodes_.resize(glm::max(1UL, oldSize * 2));

        size_t li = nodes_.size() - 1;
        for (size_t i = oldSize; i < li; ++i)
        {
            nodes_[i].nextFree = static_cast<uint32_t>(i) + 1;
        }
        nodes_[li].nextFree = TreeNode::NullNode;

        firstFreeIndex_ = static_cast<uint32_t>(oldSize);
    }
}

uint32_t DynamicTree::allocateNode()
{
    checkCapacity();

    uint32_t newIdx = firstFreeIndex_;

    TreeNode& node = nodes_[newIdx];

    firstFreeIndex_ = node.nextFree;

    node.parent = TreeNode::NullNode;
    node.left = TreeNode::NullNode;
    node.right = TreeNode::NullNode;
    node.height = 0;

    return newIdx;
}

void DynamicTree::deallocateNode(uint32_t index)
{
    TreeNode& node = nodes_[index];

    node.nextFree = firstFreeIndex_;
    firstFreeIndex_ = index;
}

} // namespace ngn
