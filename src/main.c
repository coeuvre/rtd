#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 288
#define WINDOW_HEIGHT 512

#include "cgmath.h"
#include "window.h"
#include "renderer.h"
#include "time.h"

typedef struct GameNode GameNode;

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

typedef struct GameContext {
    Window *window;
    RenderContext *rc;
    FPSCounter fpsCounter;
    int isRunning;

    GameNodeTreeWalker gameNodeTreeWalker;

    Font *font;

    GameNode *rootNode;
} GameContext;


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

struct GameNode {
    GameContext *c;

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

GameNode *CreateGameNode(GameContext *c, const char *name) {
    GameNode *node = malloc(sizeof(GameNode));
    memset(node, 0, sizeof(GameNode));

    node->c = c;
    node->name = name;

    return node;
}

void AppendGameNodeChild(GameNode *parent, GameNode *child) {
    assert(child->parent == NULL);

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

T2 GetGameNodeWorldTransform(GameNode *node) {
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

GameNodeTreeWalker *SetupGameNodeTreeWalker(GameContext *c) {
    GameNodeTreeWalker *walker = &c->gameNodeTreeWalker;
    walker->node = c->rootNode;
    walker->level = 1;
    walker->size = 0;
    return walker;
}

int HasNextGameNode(GameNodeTreeWalker *walker) {
    return walker->node != NULL;
}

void WalkToNextGameNode(GameNodeTreeWalker *walker) {
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

static void OnFixedUpdateBackground(GameNode *node, void *data, float delta) {
//    TransformComponent *transform = GetGameNodeComponent(node, TransformComponent);
//    transform->transform = TranslateT2(MakeV2(-40.0f * delta, 0.0f), transform->transform);
}

static GameNode *CreateBackgroundGameNode(GameContext *c) {
    GameNode *node = CreateGameNode(c, "Background");

    ScriptComponent *script = malloc(sizeof(ScriptComponent));
    script->data = NULL;
    script->onReady = NULL;
    script->onFixedUpdate = OnFixedUpdateBackground;
    SetGameNodeComponent(node, ScriptComponent, script);

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    transform->transform = IdentityT2();
    SetGameNodeComponent(node, TransformComponent, transform);

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    sprite->texturePath = "assets/sprites/background_day.png";
    sprite->isRegionEnabled = 0;
    sprite->region = ZeroBBox2();
    SetGameNodeComponent(node, SpriteComponent, sprite);

    return node;
}

static GameNode *CreateBirdGameNode(GameContext *c) {
    GameNode *node = CreateGameNode(c, "Bird");

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    transform->transform = IdentityT2();
    SetGameNodeComponent(node, TransformComponent, transform);

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    sprite->texturePath = "assets/sprites/bird_blue_0.png";
    sprite->isRegionEnabled = 0;
    sprite->region = ZeroBBox2();
    SetGameNodeComponent(node, SpriteComponent, sprite);

    return node;
}

static void LoadGameNodes(GameContext *c) {
    GameNode *mainNode = CreateGameNode(c, "Main");

    GameNode *backgroundGameNode = CreateBackgroundGameNode(c);
    AppendGameNodeChild(mainNode, backgroundGameNode);

    GameNode *birdGameNode = CreateBirdGameNode(c);
    AppendGameNodeChild(mainNode, birdGameNode);

    c->rootNode = mainNode;
}

static void SetupGame(GameContext *c) {
    SDL_Init(0);

    c->window = CreateGameWindow("Flappy Bird", WINDOW_WIDTH, WINDOW_HEIGHT);
    c->rc = CreateRenderContext(WINDOW_WIDTH, WINDOW_HEIGHT, c->window->pointToPixel);

    LoadGameNodes(c);

#ifdef PLATFORM_WIN32
    char *font = "C:/Windows/Fonts/Arial.ttf";
#else
    char *font = "/Library/Fonts/Arial.ttf";
#endif
    c->font = LoadFont(c->rc, font);
}

static void ProcessSystemEvent(GameContext *c) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                c->isRunning = 0;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    c->isRunning = 0;
                }

                break;
            }

            default: break;
        }
    }
}

static void DoScriptFixedUpdate(GameNode *node, float delta) {
    ScriptComponent *script = GetGameNodeComponent(node, ScriptComponent);
    if (script == NULL) {
        return;
    }

    OnFixedUpdateFn *onFixedUpdate = script->onFixedUpdate;
    if (onFixedUpdate == NULL) {
        return;
    }

    onFixedUpdate(node, script->data, delta);
}

static void Update(GameContext *c, float delta) {
    for (GameNodeTreeWalker *walker = SetupGameNodeTreeWalker(c); HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        DoScriptFixedUpdate(walker->node, delta);
    }
}

static void RenderSprite(GameNode *node) {
    RenderContext *rc = node->c->rc;

    SpriteComponent *sprite = GetGameNodeComponent(node, SpriteComponent);
    if (sprite == NULL) {
        return;
    }

    T2 transform = GetGameNodeWorldTransform(node);

    Texture *texture = LoadTexture(rc, sprite->texturePath);

    BBox2 src;
    if (sprite->isRegionEnabled) {
        src = sprite->region;
    } else {
        src = MakeBBox2FromTexture(texture);
    }

    BBox2 dst = src;

    DrawTexture(rc, transform, dst, texture, src, OneV4(), ZeroV4());

    DestroyTexture(rc, &texture);
}

static void Render(GameContext *c) {
    RenderContext *rc = c->rc;
    int lastDrawCallCount = rc->drawCallCount;

    SetCameraTransform(rc, MakeT2FromScale(MakeV2(2.0f, 2.0f)));

    ClearDrawing(rc);

    for (GameNodeTreeWalker *walker = SetupGameNodeTreeWalker(c); HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        RenderSprite(walker->node);
    }

    SetCameraTransform(rc, IdentityT2());

    float fontSize = 16.0f;
    float ascent = GetFontAscent(rc, c->font, fontSize);
    float lineHeight = GetFontLineHeight(rc, c->font, fontSize);
    float y = rc->height - ascent;
#define BUF_SIZE 128
    char buf[BUF_SIZE];

    snprintf(buf, BUF_SIZE, "FPS: %d", c->fpsCounter.fps);
    DrawLineText(rc, c->font, fontSize, 0.0f, y, buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
    y -= lineHeight;

    snprintf(buf, BUF_SIZE, "Draw calls: %d", lastDrawCallCount);
    DrawLineText(rc, c->font, fontSize, 0.0f, y, buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
    y -= lineHeight;

    // Draw game node tree hierarchy
    for (GameNodeTreeWalker *walker = SetupGameNodeTreeWalker(c); HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        GameNode *node = walker->node;
        float indent = (walker->level - 1) * fontSize;
        snprintf(buf, BUF_SIZE, "%s", node->name);
        DrawLineText(rc, c->font, fontSize, indent, y, buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
        y -= lineHeight;
    }
}

static int RunMainLoop(GameContext *c) {
    c->isRunning = 1;

    InitFPSCounter(&c->fpsCounter);
    Tick lastUpdate = GetCurrentTick();
    while (c->isRunning) {
        ProcessSystemEvent(c);

        Tick now = GetCurrentTick();
        float delta = TickToSecond(now - lastUpdate);
        lastUpdate = now;
        Update(c, delta);

        Render(c);
        SwapWindowBuffers(c->window);
        CountOneFrame(&c->fpsCounter);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {0};

    SetupGame(&context);

    return RunMainLoop(&context);
}
