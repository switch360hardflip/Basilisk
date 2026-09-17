
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

static struct {
    bs_Range file_button;
} instances;

bs_NonClientArea onClientAreaTick(bs_Context* context, bs_ivec2 pt) {
    #ifdef _WIN32
    RECT rc;
    GetWindowRect(context->hwnd, &rc);

    int y = pt.y - rc.top;

//    if (basilisk_hovering_title_bar_buttons)
//        return BS_NON_CLIENT_AREA_CAPTION_BUTTON;

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

static bs_Range basilisk_instantiateTitleBarButtonUI(bsgfx_AtlasCache* icon_cache, bsgfx_Material* material, bs_vec3 position, bs_vec2 title_bar_size, int width) {
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;

    bs_Range range = basilisk_instantiateButtonBackgroundUI(element, material, position, BS_V2(width, BASILISK_TITLE_BAR_HEIGHT));

    position.z++;
    bsgfx_UIIcon close_button_icon = {
        .position = position,
        .cache = icon_cache,
        .subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI],
        .align = { width, BASILISK_TITLE_BAR_HEIGHT },
    };

    bsgfx_atlasIconUIElement(close_button_icon, element);
    bsgfx_instantiateAtlasIconUIElement(close_button_icon, element);
    
    return range;
}

static bs_Range basilisk_instantiateTitleBarTextButtonUI(const char* text, bsgfx_Material* material, bs_vec3 position, bs_vec2 title_bar_size) {
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
    return basilisk_instantiateButtonBackgroundUI(element, material, element->position, BS_V2(element->size.x + TITLE_BAR_BUTTON_PADDING_X * 2, height));
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

    bsgfx_Material* close_button_background_material = $close_button_background_color();
    bsgfx_Material* default_button_background_material = $default_button_background_color();
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
    bsgfx_instantiateSolidUI((bsgfx_UISolid) {
        .position = position,
        .size = title_bar_size,
        .material_id = $title_bar_background()->id,
    }, element);
    position.z++;

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

    instances.file_button = basilisk_instantiateTitleBarTextButtonUI("File", transparent_material, position, title_bar_size);

    //bs_Context* ctx = contextFromMenuType(CONTEXT_MENU_FILE);

    //if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON) || bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON_NON_CLIENT) || 
    //    bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON) || bs_inputDownOnce(BS_RIGHT_MOUSE_BUTTON_NON_CLIENT))
    //{
    //    if (hovering)
    //        toggleContextMenu(BS_IV2(position.x + TITLE_BAR_BUTTON_PADDING_X / 2, position.y - TITLE_BAR_BUTTON_PADDING_Y * 2), CONTEXT_MENU_FILE);
    //}

    position.x = title_bar_size.x;



    const int close_button_width = 32;
    const int maximize_button_width = 32;
    const int minimize_button_width = 32;


   /**
    Close button
    */
    position.x -= close_button_width;
    bs_Range range = basilisk_instantiateTitleBarButtonUI(close_caption, close_button_background_material, position, title_bar_size, close_button_width);

   /**
    Maximize button
    */
    position.x -= maximize_button_width;
    basilisk_instantiateTitleBarButtonUI(maximize_caption, default_button_background_material, position, title_bar_size, maximize_button_width);

   /**
    Minimize button
    */
    position.x -= minimize_button_width;
    basilisk_instantiateTitleBarButtonUI(minimize_caption, default_button_background_material, position, title_bar_size, minimize_button_width);
}

static bool buttonTest(bs_Range instance_range, bsgfx_Material* hovering_material) {
    bsgfx_Material* transparent_material = $transparent_color();

    bool hovering = bsgfx_hoveringQuadInstance(bsgfx_subtypes()[BSGFX_SUBTYPE_UI_COLOR], instance_range.offset);
    bsgfx_InstanceHeader* header = bsgfx_instanceHeader(bsgfx_subtypes()[BSGFX_SUBTYPE_UI_COLOR], instance_range.offset);

    if (hovering) {
        if (header->material == hovering_material->id)
            return false;

        header->material = hovering_material->id;
        return true;
    }
    else {
        if (header->material == transparent_material->id)
            return false;

        header->material = transparent_material->id;
        return true;
    }

    return false;
}

void onTitleBarTick() {
    bs_Context* context = bs_scope()->context;

    bsgfx_computeContextCamera();
    bsmod_onTick();

    //basilisk_instantiateBaseUI();
    bool was_hidden = context->hidden;

    bs_Renderer* renderer = bs_fetch(BASILISK_RENDERERS, BASILISK_RENDERER_MAIN)->renderer;
    bs_Queue* queue = bs_fetch(BSGFX_QUEUES, BSGFX_QUEUE_GRAPHICS)->queue;
    //bs_Renderer* renderer = bs_fetch(BASILISK_RENDERERS, BASILISK_RENDERER_TITLE_BAR)->renderer;
    //bs_Queue* queue = bs_fetch(BASILISK_QUEUES, BASILISK_QUEUE_TITLE_BAR)->queue;

    bs_RGBA clear_color = BS_RGBA(75, 75, 75, 255);

    bsgfx_Material* transparent_material = $transparent_color();
    bsgfx_Material* default_button_background_material = $default_button_background_color();

    static bool has_changes = true;

    static bool hovering = false;
    bool was_hovering = hovering;

    if (buttonTest(instances.file_button, default_button_background_material))
        has_changes = true;

    if (has_changes) {
        has_changes = false;

    }
    basilisk_pipeline(queue, renderer, clear_color);
}
