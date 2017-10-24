#ifndef RTD_GAME_NODE_H
#define RTD_GAME_NODE_H

#include <stdlib.h>

#include "cgmath.h"

typedef struct GameContext GameContext;
typedef struct GameNode GameNode;

typedef void OnReadyFn(GameNode *node, void *data);
typedef void OnFixedUpdateFn(GameNode *node, void *data, float delta);

typedef struct ScriptComponent {
    void *data;
    OnReadyFn *onReady;
    OnFixedUpdateFn *onFixedUpdate;
} ScriptComponent;

typedef struct TransformComponent {
    T2 transform;
} TransformComponent;

typedef struct SpriteComponent {
    const char *texturePath;
    int isRegionEnabled;
    BBox2 region;
} SpriteComponent;

typedef enum ComponentName {
    COMPONENT_NAME_ScriptComponent,
    COMPONENT_NAME_TransformComponent,
    COMPONENT_NAME_SpriteComponent,

    COMPONENT_NAME_COUNT,
} ComponentName;

#define MAX_TREE_HEIGHT 32

typedef struct GameNodeTreeWalkerStackEntry {
    GameNode *node;
    int level;
} GameNodeTreeWalkerStackEntry;

typedef struct GameNodeTreeWalker {
    GameNode *node;
    int level;
    int size;
    GameNodeTreeWalkerStackEntry stack[MAX_TREE_HEIGHT];
} GameNodeTreeWalker;

struct GameNode {
    const char *name;

    GameNode *parent;

    GameNode *prev;
    GameNode *next;

    int childrenCount;
    GameNode *firstChild;
    GameNode *lastChild;

    // TODO(coeuvre): Use hashmap?
    void *components[COMPONENT_NAME_COUNT];
};

#define SetGameNodeComponent(node, component, ptr) ((node)->components[COMPONENT_NAME_##component] = (ptr))
#define GetGameNodeComponent(node, component) ((component *) ((node)->components[COMPONENT_NAME_##component]))

extern GameNode *CreateGameNode(GameContext *c, const char *name);
extern void AppendGameNodeChild(GameNode *parent, GameNode *child);
extern T2 GetGameNodeWorldTransform(GameNode *node);
extern void WalkToNextGameNode(GameNodeTreeWalker *walker);

static inline GameNodeTreeWalker *BeginWalkGameNodeTree(GameNodeTreeWalker *walker, GameNode *node) {
    walker->node = node;
    walker->level = 1;
    walker->size = 0;
    return walker;
}

static inline int HasNextGameNode(GameNodeTreeWalker *walker) {
    return walker->node != NULL;
}


#endif // RTD_GAME_NODE_H