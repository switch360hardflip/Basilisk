
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

#include <basilisk-mod.h>
#include <bsmod_cache.h>
#include <basilisk.h>

#define BASILISK_TITLE_BAR_HEIGHT 32
#define TITLE_BAR_BUTTON_PADDING_X 8
#define TITLE_BAR_BUTTON_PADDING_Y -4

BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, icon)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, close_caption)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, maximize_caption)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, minimize_caption)

BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_close)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_minimize)
BSGFX_CACHE_ATLAS_QUERY(BSMOD_ATLASES, BSMOD_ATLAS_UI, title_bar_maximize)

BSGFX_CACHE_COLOR_MATERIAL(title_bar_background, BS_RGBA(83, 83, 83, 255))
BSGFX_CACHE_COLOR_MATERIAL(close_button_background_color, BS_RGBA(226, 42, 39, 255))
BSGFX_CACHE_COLOR_MATERIAL(default_button_background_color, BS_RGBA(93, 93, 93, 255))
BSGFX_CACHE_COLOR_MATERIAL(transparent_color, BS_RGBA(0, 0, 0, 0))

static bsgfx_Font* basilisk_title_bar_font;

typedef enum {
    TITLE_BAR_BUTTON_FILE,
    TITLE_BAR_BUTTON_MINIMIZE,
    TITLE_BAR_BUTTON_MAXIMIZE,
    TITLE_BAR_BUTTON_CLOSE,

    TITLE_BAR_BUTTON_COUNT
} TitleBarButtonId;

typedef struct {
    bs_Range instance_range;
    bool hovering;
    bool hover_once;
    bool hover_release;
} TitleBarButton;

static bool hovering_any_title_bar_button;
static bool title_bar_has_changes = true;

static TitleBarButton title_bar_buttons[TITLE_BAR_BUTTON_COUNT];

bs_NonClientArea onClientAreaTick(bs_Context* context, bs_ivec2 pt) {
    #ifdef _WIN32
    RECT rc;
    GetWindowRect(context->hwnd, &rc);

    int y = pt.y - rc.top;

    if (hovering_any_title_bar_button)
        return BS_NON_CLIENT_AREA_CAPTION_BUTTON;

    if (y >= 0 && y < BASILISK_TITLE_BAR_HEIGHT)
        return BS_NON_CLIENT_AREA_CAPTION;
    #endif

    return BS_CLIENT_AREA;
}

static bs_Range basilisk_instantiateButtonBackgroundUI(bsgfx_UIElement* element, bsgfx_Material* material, bs_vec3 position, bs_vec2 size) {
    bsgfx_UISolid button = {
        .position = position,
        .size = size,
        .material_id = material->id,
    };

    bsgfx_solidUIElement(button, element);
   //element->position.x -= element->size.x;
    return bsgfx_instantiateSolidUIElement(button, element);
}

static void basilisk_instantiateTitleBarButtonUI(
    TitleBarButtonId id, 
    bsgfx_AtlasCache* icon_cache, 
    bsgfx_Material* material, 
    bs_vec3 position, 
    bs_vec2 title_bar_size, 
    int width
) {
    TitleBarButton* button = title_bar_buttons + id;
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;

    button->instance_range = basilisk_instantiateButtonBackgroundUI(element, material, position, BS_V2(width, BASILISK_TITLE_BAR_HEIGHT));

    position.z++;
    bsgfx_UIIcon close_button_icon = {
        .position = position,
        .cache = icon_cache,
        .subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI],
        .align = { width, BASILISK_TITLE_BAR_HEIGHT },
    };

    bsgfx_atlasIconUIElement(close_button_icon, element);
    bsgfx_instantiateAtlasIconUIElement(close_button_icon, element);
}

static void basilisk_instantiateTitleBarTextButtonUI(
    TitleBarButtonId id,
    const char* text,
    bsgfx_Material* material, 
    bs_vec3 position, 
    bs_vec2 title_bar_size
) {
    TitleBarButton* button = title_bar_buttons + id;

    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;

    position.z++;
    position.x += TITLE_BAR_BUTTON_PADDING_X;
    bsgfx_UIText text_ui = {
        .position = position,
        .font = basilisk_title_bar_font,
        .as_ascii = text,
        .px_size = 13,
        .align = { 0, BASILISK_TITLE_BAR_HEIGHT },
    };

    bsgfx_instantiateTextUI(text_ui, element);
    element->position = position;
    element->position.x -= TITLE_BAR_BUTTON_PADDING_X;
    element->position.z--;

    float height = BASILISK_TITLE_BAR_HEIGHT;
    height += TITLE_BAR_BUTTON_PADDING_Y * 2.0;
    element->position.y -= TITLE_BAR_BUTTON_PADDING_Y;

    button->instance_range = basilisk_instantiateButtonBackgroundUI(element, material, element->position, BS_V2(element->size.x + TITLE_BAR_BUTTON_PADDING_X * 2, height));
}

void basilisk_instantiateTitleBarUI() {
    basilisk_title_bar_font = _fonts_.selawik;

    if (!basilisk_title_bar_font)
        return;

    bs_ivec2 resolution = bs_resolution(bs_scope()->context);
    bs_vec2 title_bar_size = { resolution.x, BASILISK_TITLE_BAR_HEIGHT };

    bsgfx_AtlasCache* icon_atlas_cache = $BSMOD_ATLAS_UI_icon();
    bsgfx_AtlasCache* close_caption = $BSMOD_ATLAS_UI_title_bar_close();
    bsgfx_AtlasCache* maximize_caption = $BSMOD_ATLAS_UI_title_bar_maximize();
    bsgfx_AtlasCache* minimize_caption = $BSMOD_ATLAS_UI_title_bar_minimize();

    bsgfx_Material* transparent_material = $transparent_color();

    bs_vec3 position;
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;
    bool hovering;

    // title icon
    //position = bsgfx_seekTopLeftUI(title_bar_size);
    position = BS_V3(0, resolution.y - title_bar_size.y, 0);
   /**
    Background
    */
    /*
    bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = position,
        .size = title_bar_size,
        .material_id = $title_bar_background()->id,
    }, element);
    position.z++;
    */

   /**
    Icon
    */
    bsgfx_instantiateAtlasIconUI((bsgfx_UIIcon) {
        .position = position,
        .cache = icon_atlas_cache,
        .subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI],
        .align = { 32, BASILISK_TITLE_BAR_HEIGHT },
    }, element);

   /**
    File button
    */
    position.x += element->size.x;
    position.x += 16.0;

    basilisk_instantiateTitleBarTextButtonUI(TITLE_BAR_BUTTON_FILE, "File", transparent_material, position, title_bar_size);

    //bs_Context* ctx = contextFromMenuType(CONTEXT_MENU_FILE);

    //if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON) || bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON_NON_CLIENT) || 
    //    bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON) || bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON_NON_CLIENT))
    //{
    //    if (hovering)
    //        toggleContextMenu(BS_IV2(position.x + TITLE_BAR_BUTTON_PADDING_X / 2, position.y - TITLE_BAR_BUTTON_PADDING_Y * 2), CONTEXT_MENU_FILE);
    //}

    position.x = title_bar_size.x;

    const int close_button_width = 48;
    const int maximize_button_width = 48;
    const int minimize_button_width = 48;

    position.x -= close_button_width;
    basilisk_instantiateTitleBarButtonUI(TITLE_BAR_BUTTON_CLOSE, close_caption, transparent_material, position, title_bar_size, close_button_width);

    position.x -= maximize_button_width;
    basilisk_instantiateTitleBarButtonUI(TITLE_BAR_BUTTON_MAXIMIZE, maximize_caption, transparent_material, position, title_bar_size, maximize_button_width);

    position.x -= minimize_button_width;
    basilisk_instantiateTitleBarButtonUI(TITLE_BAR_BUTTON_MINIMIZE, minimize_caption, transparent_material, position, title_bar_size, minimize_button_width);
}

static void buttonTest(TitleBarButtonId id, bsgfx_Material* hovering_material) {
    TitleBarButton* button = title_bar_buttons + id;

    bsgfx_Material* transparent_material = $transparent_color();
    bsgfx_InstanceSubtype* subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI_COLOR];

    button->hovering = bsgfx_hoveringQuadInstance(subtype, button->instance_range.offset);
    button->hover_once = false;
    button->hover_release = false;
    bsgfx_InstanceHeader* header = bsgfx_instanceHeader(subtype, button->instance_range.offset);

    if (button->hovering) {
        if (header->material == hovering_material->id)
            return false;

        button->hover_once = title_bar_has_changes = true;
        header->material = hovering_material->id;
    }
    else {
        if (header->material == transparent_material->id)
            return false;

        button->hover_release = title_bar_has_changes = true;
        header->material = transparent_material->id;
    }
}

bool onTitleBarTick() {
    bs_Context* context = bs_scope()->context;

    bsgfx_computeContextCamera();
    bsmod_onTick();

    bool was_hidden = context->hidden;

    //bs_Renderer* renderer = bs_fetch(BASILISK_RENDERERS, BASILISK_RENDERER_TITLE_BAR)->renderer;
    //bs_Queue* queue = bs_fetch(BASILISK_QUEUES, BASILISK_QUEUE_TITLE_BAR)->queue;

    bsgfx_Material* transparent_material = $transparent_color();
    bsgfx_Material* default_button_background_material = $default_button_background_color();
    bsgfx_Material* close_button_background_material = $close_button_background_color();

    static bool hovering = false;
    bool was_hovering = hovering;

    buttonTest(TITLE_BAR_BUTTON_FILE, default_button_background_material);
    buttonTest(TITLE_BAR_BUTTON_MINIMIZE, default_button_background_material);
    buttonTest(TITLE_BAR_BUTTON_MAXIMIZE, default_button_background_material);
    buttonTest(TITLE_BAR_BUTTON_CLOSE, close_button_background_material);

    if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON)) {
        if (title_bar_buttons[TITLE_BAR_BUTTON_FILE].hovering) {
            bs_vec3 position = { 0 };
            position.x += TITLE_BAR_BUTTON_PADDING_X / 2;
            position.y -= TITLE_BAR_BUTTON_PADDING_Y / 2;
            toggleContextMenu(BS_IV2(position.x, position.y), CONTEXT_MENU_FILE);
        }
    }

    hovering_any_title_bar_button = false;
    for (int i = 0; i < TITLE_BAR_BUTTON_COUNT; i++) {
        if (title_bar_buttons[i].hovering) {
            hovering_any_title_bar_button = true;
            break;
        }
    }

    if (title_bar_has_changes || bs_scope()->resizing) {
        title_bar_has_changes = false;

        return true;
    }

    return false;
}
