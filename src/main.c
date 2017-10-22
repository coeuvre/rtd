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

typedef struct GameNodeTreeWalker {
    GameNode *current;
    int size;
    GameNode *stack[MAX_TREE_HEIGHT];
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

    GameNode *parent;

    GameNode *prev;
    GameNode *next;

    int childrenCount;
    GameNode *firstChild;
    GameNode *lastChild;

    // TODO(coeuvre): Use hashmap?
    void *components[COMPONENT_NAME_COUNT];
};

GameNode *CreateGameNode(GameContext *c) {
    GameNode *node = malloc(sizeof(GameNode));
    node->c = c;
    node->parent = NULL;
    node->prev = NULL;
    node->next = NULL;
    node->childrenCount = 0;
    node->firstChild = NULL;
    node->lastChild = NULL;

    for (int i = 0; i < COMPONENT_NAME_COUNT; ++i) {
        node->components[i] = NULL;
    }

    return node;
}

void AppendGameNodeChild(GameNode *parent, GameNode *child) {
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

T2 GetGameNodeTransform(GameNode *node) {
    if (node->components[COMPONENT_NAME_TransformComponent] == NULL) {
        return IdentityT2();
    }

    T2 parentTransform;
    if (node->parent) {
        parentTransform = GetGameNodeTransform(node->parent);
    } else {
        parentTransform = IdentityT2();
    }

    TransformComponent *transformComponent = node->components[COMPONENT_NAME_TransformComponent];
    return DotT2(transformComponent->transform, parentTransform);
}

GameNodeTreeWalker *SetupGameNodeTreeWalker(GameContext *c) {
    GameNodeTreeWalker *walker = &c->gameNodeTreeWalker;
    walker->current = c->rootNode;
    walker->size = 0;
    return walker;
}

int HasNextGameNode(GameNodeTreeWalker *walker) {
    return walker->current != NULL;
}

void WalkToNextGameNode(GameNodeTreeWalker *walker) {
    assert(HasNextGameNode(walker));

    for (GameNode *child = walker->current->lastChild; child != NULL; child = child->prev) {
        assert(walker->size < MAX_TREE_HEIGHT);
        walker->stack[walker->size++] = child;
    }

    if (walker->size > 0) {
        walker->current = walker->stack[--walker->size];
    } else {
        walker->current = NULL;
    }
}

GameNode *GetCurrentGameNode(GameNodeTreeWalker *walker) {
    return walker->current;
}

static void OnFixedUpdateBackground(GameNode *node, void *data, float delta) {
    TransformComponent *transform = node->components[COMPONENT_NAME_TransformComponent];
    transform->transform = TranslateT2(MakeV2(-10.0f * delta, 0.0f), transform->transform);
}

static GameNode *LoadBackgroundGameNode(GameContext *c) {
    GameNode *node = CreateGameNode(c);

    ScriptComponent *script = malloc(sizeof(ScriptComponent));
    node->components[COMPONENT_NAME_ScriptComponent] = script;
    script->data = NULL;
    script->onReady = NULL;
    script->onFixedUpdate = OnFixedUpdateBackground;

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    node->components[COMPONENT_NAME_TransformComponent] = transform;
    transform->transform = MakeT2FromScale(MakeV2(2.0f, 2.0f));

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    node->components[COMPONENT_NAME_SpriteComponent] = sprite;
    sprite->texturePath = "assets/sprites/background_day.png";
    sprite->isRegionEnabled = 0;
    sprite->region = ZeroBBox2();

    return node;
}

static void LoadGameNodes(GameContext *c) {
    GameNode *mainNode = CreateGameNode(c);

    GameNode *backgroundNode = LoadBackgroundGameNode(c);
    AppendGameNodeChild(mainNode, backgroundNode);

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
    ScriptComponent *script = node->components[COMPONENT_NAME_ScriptComponent];
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
        GameNode *node = GetCurrentGameNode(walker);
        DoScriptFixedUpdate(node, delta);
    }
}

static void RenderSprite(GameNode *node) {
    RenderContext *rc = node->c->rc;

    SpriteComponent *sprite = node->components[COMPONENT_NAME_SpriteComponent];
    if (sprite == NULL) {
        return;
    }

    T2 transform = GetGameNodeTransform(node);

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

    ClearDrawing(rc);

    for (GameNodeTreeWalker *walker = SetupGameNodeTreeWalker(c); HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        GameNode *node = GetCurrentGameNode(walker);
        RenderSprite(node);
    }

    float fontSize = 20.0f;
    float ascent = GetFontAscent(rc, c->font, fontSize);
    float lineHeight = GetFontLineHeight(rc, c->font, fontSize);
    float y = rc->height - ascent;
#define BUF_SIZE 128
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "FPS: %d", c->fpsCounter.fps);
    DrawLineText(rc, c->font, fontSize, 0.0f, y, buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
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
