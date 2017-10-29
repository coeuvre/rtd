#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 576
#define WINDOW_HEIGHT 768

#include "game.h"

static void OnFixedUpdateBackground(GameNode *node, void *data, float delta) {
//    TransformComponent *transform = GetGameNodeComponent(node, TransformComponent);
//    transform->transform = TranslateT2(MakeV2(-40.0f * delta, 0.0f), transform->transform);
}

static GameNode *CreateBackgroundGameNode(GameContext *c, const char *name) {
    GameNode *node = CreateGameNode(c, name);

    ScriptComponent *script = malloc(sizeof(ScriptComponent));
    script->data = NULL;
    script->onReady = NULL;
    script->onFixedUpdate = OnFixedUpdateBackground;
    SetGameNodeComponent(node, ScriptComponent, script);

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    transform->translation = ZeroV2();
    transform->rotation = 0.0f;
    transform->scale = OneV2();
    SetGameNodeComponent(node, TransformComponent, transform);

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    sprite->texturePath = "assets/sprites/background_day.png";
    sprite->region = OneBBox2();
    sprite->anchor = MakeV2(0.0f, 0.0f);
    SetGameNodeComponent(node, SpriteComponent, sprite);

    return node;
}

static void OnFixedUpdateBird(GameNode *node, void *data, float delta) {
    TransformComponent *transform = GetGameNodeComponent(node, TransformComponent);
    transform->rotation += 1.0f * delta;
}

static GameNode *CreateBirdGameNode(GameContext *c) {
    GameNode *node = CreateGameNode(c, "Bird");

    ScriptComponent *script = malloc(sizeof(ScriptComponent));
    script->data = NULL;
    script->onReady = NULL;
    script->onFixedUpdate = OnFixedUpdateBird;
    SetGameNodeComponent(node, ScriptComponent, script);

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    transform->translation = MakeV2(28.0f, 200.0f);
    transform->rotation = 0.0f;
    transform->scale = OneV2();
    SetGameNodeComponent(node, TransformComponent, transform);

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    sprite->texturePath = "assets/sprites/bird_blue_0.png";
    sprite->region = OneBBox2();
    sprite->anchor = MakeV2(0.5f, 0.5f);
    SetGameNodeComponent(node, SpriteComponent, sprite);

    return node;
}

static GameNode *CreateGroundGameNode(GameContext *c, const char *name) {
    GameNode *node = CreateGameNode(c, name);

    TransformComponent *transform = malloc(sizeof(TransformComponent));
    transform->translation = ZeroV2();
    transform->rotation = 0.0f;
    transform->scale = OneV2();
    SetGameNodeComponent(node, TransformComponent, transform);

    SpriteComponent *sprite = malloc(sizeof(SpriteComponent));
    sprite->texturePath = "assets/sprites/ground.png";
    sprite->region = OneBBox2();
    sprite->anchor = ZeroV2();
    SetGameNodeComponent(node, SpriteComponent, sprite);

    return node;
}

static void LoadGameNodes(GameContext *c) {
    GameNode *mainNode = CreateGameNode(c, "Main");

    GameNode *background = CreateBackgroundGameNode(c, "Background 1");
    AppendGameNodeChild(mainNode, background);

    GameNode *bird = CreateBirdGameNode(c);
    AppendGameNodeChild(mainNode, bird);

    GameNode *ground = CreateGroundGameNode(c, "Ground 1");
    AppendGameNodeChild(mainNode, ground);

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
    for (GameNodeTreeWalker *walker = BeginWalkGameNodeTree(&c->gameNodeTreeWalker, c->rootNode); HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        DoScriptFixedUpdate(walker->node, delta);
    }
}

static void RenderSprite(RenderContext *rc, GameNode *node) {
    SpriteComponent *sprite = GetGameNodeComponent(node, SpriteComponent);
    if (sprite == NULL) {
        return;
    }

    T2 transform = GetGameNodeWorldTransform(node);

    Texture *texture = LoadTexture(rc, sprite->texturePath);
    if (texture == NULL) {
        return;
    }

    V2 texSize = MakeV2((F) texture->width, (F) texture->height);
    BBox2 src = MakeBBox2(HadamardMulV2(sprite->region.min, texSize), HadamardMulV2(sprite->region.max, texSize));

    BBox2 dst = src;

    V2 offset = HadamardMulV2(sprite->anchor, GetBBox2Size(src));
    transform = DotT2(transform, MakeT2FromTranslation(NegV2(offset)));

    DrawTexture(rc, transform, dst, texture, src, OneV4());

    DestroyTexture(rc, &texture);
}

static void Render(GameContext *c) {
    RenderContext *rc = c->rc;
    int lastDrawCallCount = rc->drawCallCount;

    ClearDrawing(rc);

    SetCameraTransform(rc, MakeT2(MakeV2(144.0f, 128.0f), 0.0f, MakeV2(2.0f, 2.0f)));

    for (GameNodeTreeWalker *walker = BeginWalkGameNodeTree(&c->gameNodeTreeWalker, c->rootNode);
         HasNextGameNode(walker); WalkToNextGameNode(walker)) {
        RenderSprite(rc, walker->node);
    }

    DrawRect(rc, IdentityT2(), MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(144.0f, 256.0f)),
             0.0f, 1.0f, ZeroV4(), MakeV4(1.0f, 1.0f, 0.0f, 1.0f));

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
    for (GameNodeTreeWalker *walker = BeginWalkGameNodeTree(&c->gameNodeTreeWalker, c->rootNode);
         HasNextGameNode(walker); WalkToNextGameNode(walker)) {
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

    GameContext *context = malloc(sizeof(GameContext));

    SetupGame(context);

    return RunMainLoop(context);
}
