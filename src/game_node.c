#include "game_node.h"

#include "assert.h"
#include "string.h"

extern GameNode *CreateGameNode(GameContext *c, const char *name) {
    GameNode *node = malloc(sizeof(GameNode));
    memset(node, 0, sizeof(GameNode));
    node->name = name;

    return node;
}

extern void AppendGameNodeChild(GameNode *parent, GameNode *child) {
    assert(child->parent == NULL);

    // TODO(coeuvre): check whether child's name is unique

    child->parent = parent;

    if (parent->lastChild != NULL) {
        parent->lastChild->next = child;
    } else {
        parent->firstChild = child;
    }
    child->prev = parent->lastChild;
    child->next = NULL;
    parent->lastChild = child;

    ++parent->childrenCount;
}


extern T2 GetGameNodeWorldTransform(GameNode *node) {
    if (GetGameNodeComponent(node, TransformComponent) == NULL) {
        return IdentityT2();
    }

    T2 parentTransform;
    if (node->parent) {
        parentTransform = GetGameNodeWorldTransform(node->parent);
    } else {
        parentTransform = IdentityT2();
    }

    TransformComponent *transform = GetGameNodeComponent(node, TransformComponent);
    return DotT2(transform->transform, parentTransform);
}

extern void WalkToNextGameNode(GameNodeTreeWalker *walker) {
    assert(HasNextGameNode(walker));

    for (GameNode *child = walker->node->lastChild; child != NULL; child = child->prev) {
        assert(walker->size < MAX_TREE_HEIGHT);
        GameNodeTreeWalkerStackEntry *entry = &walker->stack[walker->size++];
        entry->node = child;
        entry->level = walker->level + 1;
    }

    if (walker->size > 0) {
        GameNodeTreeWalkerStackEntry *entry = &walker->stack[--walker->size];
        walker->node = entry->node;
        walker->level = entry->level;
    } else {
        walker->node = NULL;
        walker->level = 0;
    }
}