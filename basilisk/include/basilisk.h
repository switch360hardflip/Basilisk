
 /**
  MIT License

  Copyright (c) 2026 switch360hardflip <switch360hardflip@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  */

#ifndef BASILISK_H
#define BASILISK_H

#include <basilisk-gfx.h>

typedef struct {
    int sources[BS_OBJECT_TYPE_COUNT];
    int package_id;
    bs_U32 main_thread_id;
    bs_Context* context;
} Basilisk;

typedef struct {
    bsgfx_Font* selawik;
} Fonts;

extern Basilisk basilisk;
extern Fonts _fonts_;

#define BASILISK_TITLE_BAR_HEIGHT 32

#define BASILISK_CONTEXT_IDS(X)                                 \
    X(BASILISK_CONTEXT_TITLE_BAR)                               \
    X(BASILISK_CONTEXT_NEW_PROJECT)                          \
    X(BASILISK_CONTEXT_MENU_0)                                  \
    X(BASILISK_CONTEXT_MENU_1)                                  \
    X(BASILISK_CONTEXTS_COUNT)

#define BASILISK_IMAGE_IDS(X)                                   \
    X(BASILISK_IMAGE_MAIN_OUTPUT_DEPTH)                         \
    X(BASILISK_IMAGE_MAIN_OUTPUT_COLOR)                         \
    X(BASILISK_IMAGES_COUNT)

#define BASILISK_SAMPLER_IDS(X)                                 \
	X(BASILISK_SAMPLERS_COUNT)

#define BASILISK_BUFFER_IDS(X)                                  \
	X(BASILISK_BUFFERS_COUNT)

#define BASILISK_BATCH_IDS(X)                                   \
    X(BASILISK_BATCHES_COUNT)

#define BASILISK_RENDERER_IDS(X)                                \
    X(BASILISK_RENDERER_MAIN)                                   \
    X(BASILISK_RENDERER_TITLE_BAR)                              \
    X(BASILISK_RENDERER_CONTEXT_MENU_0)                         \
    X(BASILISK_RENDERER_CONTEXT_MENU_1)                         \
    X(BASILISK_RENDERERS_COUNT)

#define BASILISK_QUEUE_IDS(X)                                   \
    X(BASILISK_QUEUE_TITLE_BAR)                                 \
    X(BASILISK_QUEUES_COUNT)

#define BASILISK_RAY_TRACER_IDS(X)                              \
	X(BASILISK_RAY_TRACERS_COUNT)

#define BASILISK_ATLAS_IDS(X)                                   \
    X(BASILISK_ATLASES_COUNT)

#define BASILISK_FONT_IDS(X)                                    \
	X(BASILISK_FONTS_COUNT)

BS_GENERATE_ENUM(BASILISK_CONTEXT_IDS);
BS_GENERATE_ENUM(BASILISK_IMAGE_IDS);
BS_GENERATE_ENUM(BASILISK_SAMPLER_IDS);
BS_GENERATE_ENUM(BASILISK_BUFFER_IDS);
BS_GENERATE_ENUM(BASILISK_BATCH_IDS);
BS_GENERATE_ENUM(BASILISK_RENDERER_IDS);
BS_GENERATE_ENUM(BASILISK_QUEUE_IDS);
BS_GENERATE_ENUM(BASILISK_RAY_TRACER_IDS);
BS_GENERATE_ENUM(BASILISK_ATLAS_IDS);
BS_GENERATE_ENUM(BASILISK_FONT_IDS);

#define BASILISK_CONTEXTS basilisk.sources[BS_OBJECT_CONTEXT]
#define BASILISK_IMAGES basilisk.sources[BS_OBJECT_IMAGE]
#define BASILISK_SAMPLERS basilisk.sources[BS_OBJECT_SAMPLER]
#define BASILISK_BUFFERS basilisk.sources[BS_OBJECT_BUFFER]
#define BASILISK_BATCHES basilisk.sources[BS_OBJECT_BATCH]
#define BASILISK_RENDERERS basilisk.sources[BS_OBJECT_RENDERER]
#define BASILISK_QUEUES basilisk.sources[BS_OBJECT_QUEUE]
#define BASILISK_RAY_TRACERS basilisk.sources[BS_OBJECT_RAY_TRACER]
#define BASILISK_ATLASES basilisk.sources[BS_OBJECT_ATLAS]
#define BASILISK_FONTS basilisk.sources[BS_OBJECT_FONT]

#define BORDER_PADDING 2

typedef struct {
    bs_vec3 position;
    bs_Range background_instance_range;
    bool hovering;
} Button;

typedef enum {
    CONTEXT_MENU_UNDEFINED,

    CONTEXT_MENU_FILE,
    CONTEXT_MENU_OPEN_RECENT,
    CONTEXT_MENU_TEST,
    CONTEXT_MENU_TEST2,

    CONTEXT_MENU_COUNT
} ContextMenuType;

typedef void (*OnContextMenuClick)();

typedef struct {
    const char* left_text;
    const char* right_text;
    ContextMenuType hover_menu_type;

    Button button;

    OnContextMenuClick on_click;
} ContextMenuElement;

typedef struct {
    bsgfx_InstanceSubtype* ui;
    bsgfx_InstanceSubtype* ui_solid;
    bsgfx_InstanceSubtype* text;
} CommonSubtypes;


void instantiateNewProjectMenu(bs_Context* context);
void iniNewProjectMenu();
void onCreateProjectFromContextMenu();

bs_Object* createHiResRenderer(bs_Context* context, int id);
void createRenderers();

void renderContextMenu(bs_RendererScope* scope);
void instantiateContextMenuUI(ContextMenuType menu_type);

typedef enum {
    TITLE_BAR_BUTTON_FILE,
    TITLE_BAR_BUTTON_MINIMIZE,
    TITLE_BAR_BUTTON_MAXIMIZE,
    TITLE_BAR_BUTTON_CLOSE,

    TITLE_BAR_BUTTON_COUNT
} TitleBarButtonId;

typedef struct {
    bs_Range instance_range;
    bs_vec3 position;
    bool hovering;
    bool hover_once;
    bool hover_release;
} TitleBarButton;

bs_Range instantiateButtonBackgroundUI(
    const CommonSubtypes* subtypes,
    bsgfx_UIElement* element,
    bsgfx_Material* material,
    bs_vec3 position,
    bs_vec2 size
);

void instantiateTitleBarButtonUI(
    const CommonSubtypes* subtypes,
    TitleBarButton buttons[TITLE_BAR_BUTTON_COUNT],
    TitleBarButtonId id,
    bsgfx_AtlasCache* icon_cache,
    bsgfx_Material* material,
    bs_vec3 position,
    bs_vec2 title_bar_size,
    int width
);

void instantiateTitleBarTextButtonUI(
    const CommonSubtypes* subtypes,
    TitleBarButton buttons[TITLE_BAR_BUTTON_COUNT],
    TitleBarButtonId id,
    const char* text,
    bsgfx_Material* material,
    bs_vec3 position,
    bs_vec2 title_bar_size
);

void instantiateTitleBarTextUI(
    const CommonSubtypes* subtypes,
    const char* text,
    bsgfx_Material* material,
    bs_vec3 position,
    bs_vec2 title_bar_size
);
void titleBarButtonTest(const CommonSubtypes* subtypes, TitleBarButton buttons[TITLE_BAR_BUTTON_COUNT], TitleBarButtonId id, bsgfx_Material* hovering_material);
void instantiateTitleBarUI(bs_Context* context);


extern bool _hovering_any_title_bar_button_;

void instantiateBaseUI(bs_Context* context, const CommonSubtypes* subtypes);
bs_NonClientArea onClientAreaTick(bs_Context* context, bs_ivec2 pt);

void onContextMenuTick(bs_Context* context, void* params);
void iniContextMenu(ContextMenuType type);

void openContextMenu(bs_ivec2 position, ContextMenuType type, bs_Context* context);
void toggleContextMenu(bs_ivec2 position, ContextMenuType type);

void onTitleBarInput(bs_Context* context, bs_ContextInputParams params);
void onTitleBarMotion(bs_Context* context, int x, int y);
void onTitleBarLeave(bs_Context* context);
bool onTitleBarTick();
void iniTitleBar();

void renderDither(bs_RendererScope* scope, bs_Queue* queue);
void renderUIStencil(bs_RendererScope* scope, bs_Queue* queue);
void renderUISolid(bs_RendererScope* scope, bs_Queue* queue, bsgfx_InstanceSubtype* subtype);
void renderUI(bs_RendererScope* scope, bs_Queue* queue, bsgfx_InstanceSubtype* subtype);
void renderRoundedQuads(bs_RendererScope* scope, bs_Queue* queue, bsgfx_InstanceSubtype* subtype);
void renderPrefabOutlines(bs_RendererScope* scope, bs_Queue* queue);
void renderFontSubtype(bs_RendererScope* scope, bs_Queue* queue, bsgfx_InstanceSubtype* subtype, int font_id, bs_Shader* fragment_shader);
void renderTiles(bs_RendererScope* scope, bs_Queue* queue);
void renderSelectedTile(bs_RendererScope* scope, bs_Queue* queue);
void renderUIPost(bs_RendererScope* scope, bs_Queue* queue);
void renderCones(bs_RendererScope* scope, bs_Queue* queue);
void renderPoints(bs_RendererScope* scope, bs_Queue* queue);
void renderLines(bs_RendererScope* scope, bs_Queue* queue);
void renderDepthlessLines(bs_RendererScope* scope, bs_Queue* queue);

void renderMainContext(bs_RendererScope* scope);
void pipeline(bs_Queue* queue, bs_Renderer* renderer, bs_SubpassFunction callbacks[], int callbacks_count);

void updateInstances();

#endif
