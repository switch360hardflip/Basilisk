
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
#define BASILISK_CONTEXT_MENU_TEXT_INDENT 16
#define BASILISK_CONTEXT_MENU_BUTTON_HEIGHT 16
#define BASILISK_CONTEXT_MENU_WIDTH 300
#define BASILISK_CONTEXT_MENU_CLEAR_COLOR BS_RGBA(185, 185, 185, 255)

BSGFX_CACHE_COLOR_MATERIAL(context_menu_button_hover_color, BS_RGBA(0, 120, 215, 255))

static void instantiateContextMenuUI(bs_Context* context, ContextMenuElement elements[], int elements_count) {
    int window_height = elements_count * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;
    const int border_size = 1;

    bs_vec3 position = { 0.0 };

    bsgfx_UIElement element;

   /**
    Background
    */
    bsgfx_UISolid solid = {
        .material_id = $white_material()->id,
        .position = {
            position.x + border_size,
            position.y + border_size,
            position.z,
        },
        .size = { 
            BASILISK_CONTEXT_MENU_WIDTH - border_size * 2,
            window_height - border_size * 2,
        },
    };
    bsgfx_solidUIElement(solid, &element);
    bool hovering_menu = bsgfx_hoveringUIElement(&element);
    bsgfx_instantiateSolidUIElement(solid, &element);
    position.z++;

   /**
    Elements
    */
    position.y += window_height;

    for (int i = 0; i < elements_count; i++) {
        ContextMenuElement* context_menu_element = elements + i;
        bsgfx_UIElement element;

        bsgfx_Material* text_material = $black_material();

       /**
        Hovering button background
        */
        bsgfx_UISolid solid = {
            .position = position,
            .size = { BASILISK_CONTEXT_MENU_WIDTH, BASILISK_CONTEXT_MENU_BUTTON_HEIGHT },
        };

        bsgfx_solidUIElement(solid, &element);
        element.position.y -= element.size.y;
        position = element.position;

        bool hovering = bsgfx_hoveringUIElement(&element);
        bool child_open = false;

        if (context_menu_element->hover_menu_type != CONTEXT_MENU_UNDEFINED) {
          //  bs_Context* child_context = bs_fetch(BASILISK_CONTEXTS, _context_menu_types_[context_menu_element->hover_menu_type].context_id)->context;
          //  child_open = !child_context->hidden;
        }

        if (hovering || child_open) {
            solid.material_id = $context_menu_button_hover_color()->id;
            bsgfx_instantiateSolidUIElement(solid, &element);
            text_material = $white_material();
        }

        bs_vec3 revert_position = position;

       /**
        Left Text
        */
        position.z++;
        position.x += BASILISK_CONTEXT_MENU_TEXT_INDENT;
        bsgfx_UIText left_text = {
            .position = position,
            .as_ascii = context_menu_element->left_text,
            .font = _fonts_.selawik,
            .px_size = 13,
            .material_id = text_material->id,
            .align = { 0, BASILISK_CONTEXT_MENU_BUTTON_HEIGHT },
        };

        bsgfx_instantiateTextUI(left_text, &element);
        position.x -= BASILISK_CONTEXT_MENU_TEXT_INDENT;

       /**
        Right text
        */
        if (context_menu_element->right_text) {
            position.x += BASILISK_CONTEXT_MENU_WIDTH - BASILISK_CONTEXT_MENU_TEXT_INDENT;
            bsgfx_UIText right_text = {
                .position = position,
                .as_ascii = context_menu_element->right_text,
                .font = _fonts_.selawik,
                .px_size = 13,
                .material_id = text_material->id,
                .align = { 0, BASILISK_CONTEXT_MENU_BUTTON_HEIGHT },
            };


            bsgfx_instantiateTextUI(right_text, &element);

            bs_vec3 translation = { -(element.size.x), 0.0, 0.0 };
            bsgfx_translateUIElement(&element, &translation);
        }

       /**
        Expandable menu
        */
        if (context_menu_element->hover_menu_type != CONTEXT_MENU_UNDEFINED) {
            position.x += BASILISK_CONTEXT_MENU_WIDTH - BASILISK_CONTEXT_MENU_TEXT_INDENT;
            bsgfx_AtlasCache* expand_cache = $BSMOD_ATLAS_UI_expand();

            bsgfx_UIIcon icon = {
                .position = position,
                .cache = expand_cache,
                .subtype = bsgfx_subtypes()[BSGFX_SUBTYPE_UI],
                .material_id = text_material->id,
                .align = { BASILISK_CONTEXT_MENU_TEXT_INDENT, BASILISK_CONTEXT_MENU_BUTTON_HEIGHT },
            };

            bsgfx_atlasIconUIElement(icon, &element);
            bsgfx_instantiateAtlasIconUIElement(icon, &element);

            if (hovering) {
                position.x += BASILISK_CONTEXT_MENU_TEXT_INDENT;
                position.y += BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;
               // showContextMenuUI(context_menu_element->hover_menu_type, position);
            }
            else if (hovering_menu) {
               // bs_Context* menu_context = bs_fetch(BASILISK_CONTEXTS, _context_menu_types_[context_menu_element->hover_menu_type].context_id)->context;
               // hideContextMenuUI(menu_context);
            }
        }

        position = revert_position;
    }
}

static void basilisk_renderContextMenu(bs_RendererScope* scope) {
    bs_Queue* queue = scope->queue;

    bs_PipelineHash hash;
    bs_Pipeline* pipeline;

    bs_beginCommentN(queue, BS_CONSTANT_STRING("High Resolution Subpass 0"));

    bs_vec4 clear_color = bs_rgbUCharToV4(BASILISK_CONTEXT_MENU_CLEAR_COLOR);
    clear_color.xyz = bs_sRGBToLinearV3(&clear_color.xyz);

    bs_clearColor(queue, 0, bs_resolution(bs_scope()->context), &clear_color);

    basilisk_renderDepthlessLines(scope, queue);
    basilisk_renderPoints(scope, queue);
    basilisk_renderCones(scope, queue);
    basilisk_renderSelectedTile(scope, queue);
    basilisk_renderRoundedQuads(scope, queue);
    bsgfx_renderColorPickers(scope, queue);
    basilisk_renderUISolid(scope, queue);
    basilisk_renderUI(scope, queue);

    basilisk_renderFontSubtype(scope, queue, bsgfx_subtypes()[BSGFX_SUBTYPE_FONT], 0, $fs_bsgfx_font_small());

    basilisk_renderUIStencil(scope, queue);
    basilisk_renderDither(scope, queue);

    //  bs_clearDepth(0, bs_fetch(BSMOD_IMAGES, BSMOD_IMAGE_DEPTH)->image->dim, 1.0);
    basilisk_renderTiles(scope, queue);
    bsgfx_renderPrimitives(scope, queue, bsgfx_app()->screen_camera.result);

    bsgfx_renderColorPickers(scope, queue);

    /**
     Textures
     */
    hash = bsgfx_defaultPipelineHash();
    bsgfx_requiredForTransparency(&hash);
    hash.shaders[0] = $vs_bsgfx_quad_instanced();
    hash.shaders[1] = $fs_bsgfx_256_hi_res();

    if (bs_pipeline(scope, queue, &hash, &pipeline) == BS_RESULT_OK) {

        bs_pushConstant(queue, pipeline, 0, sizeof(bsgfx_app()->screen_camera.result), &bsgfx_app()->screen_camera.result);
        bsgfx_renderSubtype(queue, bsgfx_subtypes()[BSGFX_SUBTYPE_256_HI], pipeline);
    }

    bsgfx_renderAtlasIcons(scope, queue);
    bsgfx_renderTileIcons(scope, queue);

    bs_endComment(queue);
}

void basilisk_instantiateContextMenuUI(bs_Context* context) {
    bs_ivec2 resolution = bs_resolution(context);
    bs_vec2 title_bar_size = { resolution.x, BASILISK_TITLE_BAR_HEIGHT };

    bs_vec3 position;
    bsgfx_UIElement element_v;
    bsgfx_UIElement* element = &element_v;

    // title icon
    position = BS_V3(0, resolution.y - title_bar_size.y, 0);
    position.y = 0.0;

    instantiateContextMenuUI(context, _context_menu_file_elements_, sizeof(_context_menu_file_elements_) / sizeof(*_context_menu_file_elements_));
}

void onContextMenuTick(bs_Context* context, void* params) {
    if (context->hidden)
        return;

    bsgfx_computeContextCamera();
    basilisk_instantiateContextMenuUI(context);

    if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON)) {
        //hideContextMenuUI();
    }

    bs_ivec2 resolution = bs_resolution(context);
    bs_Output outputs[] = {
        {
            .subpass = 0,
            .image = context->swapchain_image->image,
            .load_op = BS_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = BS_ATTACHMENT_STORE_OP_STORE,
            .old_layout = BS_IMAGE_LAYOUT_UNDEFINED,
            .new_layout = BS_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };

    bs_Renderer renderer = {
        .head = {
            .type = BS_OBJECT_RENDERER,
        },
        .outputs = {.data = outputs, .count = sizeof(outputs) / sizeof(*outputs), .unit_size = sizeof(bs_Output)},
        .dim = resolution,
    };

    bsgfx_tickInstanceTypes();

    basilisk_pipeline(context->popup.queue_obj->queue, &renderer, BASILISK_CONTEXT_MENU_CLEAR_COLOR);

    bsgfx_resetInstanceTypes();
}

void openContextMenu(bs_ivec2 position, ContextMenuElement context_menu_file_elements[], int context_menu_file_elements_count) {
    bs_ivec2 new_position = bs_windowPosition(bs_scope()->context);
    new_position.x += position.x;
    new_position.y -= position.y;

    int height = context_menu_file_elements_count * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;

    bs_openPopupWindow((bs_ContextListener) {
        .tick = onContextMenuTick,
    }, new_position.x, new_position.y, BASILISK_CONTEXT_MENU_WIDTH, height, "RightClickMenu");
}
