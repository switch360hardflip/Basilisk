
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

ContextMenuElement _context_menu_test2_elements_[] = {
    {
        .left_text = "abc",
        .right_text = "123456",
    },
    {
        .left_text = "abcdefghijklmnopqrstuvwxyz",
    },
};

ContextMenuElement _context_menu_test_elements_[] = {
    {
        .left_text = "lalaalal",
        .hover_menu_type = CONTEXT_MENU_TEST2,
    },
};

ContextMenuElement _context_menu_open_recent_elements_[] = {
    {
        .left_text = "Testing...",
        .hover_menu_type = CONTEXT_MENU_TEST,
    },
    {
        .left_text = "Testing...",
    },
    {
        .left_text = "Abc123",
        .hover_menu_type = CONTEXT_MENU_TEST,
    },
    {
        .left_text = "Testing...",
        .hover_menu_type = CONTEXT_MENU_TEST,
    },
    {
        .left_text = "Testing...",
    },
    {
        .left_text = "Testing...",
        .hover_menu_type = CONTEXT_MENU_TEST,
    },

};

ContextMenuElement _context_menu_file_elements_[] = {
    {
        .left_text = "New...",
        .right_text = "Ctrl+N",
        .on_click = onCreateProjectFromContextMenu,
        .hover_menu_type = CONTEXT_MENU_OPEN_RECENT,
    },
    {
        .left_text = "Open...",
    },
    {
        .left_text = "Open Recent",
        .hover_menu_type = CONTEXT_MENU_OPEN_RECENT,
    },
    {
        .left_text = "Save",
        .right_text = "Ctrl+S",
        .hover_menu_type = CONTEXT_MENU_OPEN_RECENT,
    },
    {
        .left_text = "Save As...",
        .right_text = "Shift+Ctrl+S",
    },
    {
        .left_text = "Exit",
        .right_text = "Alt+F4",
        .hover_menu_type = CONTEXT_MENU_OPEN_RECENT,
    }
};

#define CONTEXT_MENU(array) \
    { .elements = array, .elements_count = sizeof(array) / sizeof(*array), .title = #array, .has_changes = true }


BSGFX_CACHE_COLOR_MATERIAL(blue_button_background_color, BS_RGBA(72, 150, 255, 255))
BSGFX_CACHE_COLOR_MATERIAL(transparent_color, BS_RGBA(0, 0, 0, 0))

typedef struct {
    const char* title;
    bool has_changes;
    bool close;
    //  bs_Context* context;
    ContextMenuElement* elements;
    int elements_count;
    bsgfx_InstanceSubtype* text_subtype;
    bsgfx_InstanceSubtype* ui_subtype;
    bsgfx_InstanceSubtype* ui_solid_subtype;
} ContextMenu;

ContextMenu context_menus[CONTEXT_MENU_COUNT] = {
    [CONTEXT_MENU_FILE] = CONTEXT_MENU(_context_menu_file_elements_),
    [CONTEXT_MENU_OPEN_RECENT] = CONTEXT_MENU(_context_menu_open_recent_elements_),
    [CONTEXT_MENU_TEST] = CONTEXT_MENU(_context_menu_test_elements_),
    [CONTEXT_MENU_TEST2] = CONTEXT_MENU(_context_menu_test2_elements_),
};

void instantiateContextMenuUI(ContextMenuType menu_type) {
    ContextMenu* menu = context_menus + menu_type;

    int window_height = menu->elements_count * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;
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
        .subtype = menu->ui_solid_subtype
    };
    bsgfx_solidUIElement(solid, &element);
    bsgfx_instantiateSolidUIElement(solid, &element);
    position.z++;

   /**
    Elements
    */
    position.y += window_height;

    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* context_menu_element = menu->elements + i;
        bsgfx_UIElement element;

        bsgfx_Material* text_material = $black_material();

       /**
        Hovering button background
        */
        bsgfx_UISolid solid = {
            .position = position,
            .size = { BASILISK_CONTEXT_MENU_WIDTH, BASILISK_CONTEXT_MENU_BUTTON_HEIGHT },
            .subtype = menu->ui_solid_subtype,
            .material_id = $transparent_color()->id
        };

        bsgfx_solidUIElement(solid, &element);
        element.position.y -= element.size.y;
        position = element.position;
        context_menu_element->button.background_instance_range = bsgfx_instantiateSolidUIElement(solid, &element);

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
            .subtype = menu->text_subtype
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
                .subtype = menu->text_subtype
            };

            bsgfx_instantiateTextUI(right_text, &element);

            bs_vec3 translation = { -(element.size.x), 0.0, 0.0 };
            bsgfx_translateUIElement(&element, &translation);
            position.x -= BASILISK_CONTEXT_MENU_WIDTH - BASILISK_CONTEXT_MENU_TEXT_INDENT;
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
                .subtype = menu->ui_subtype
            };

            bsgfx_atlasIconUIElement(icon, &element);
            bsgfx_instantiateAtlasIconUIElement(icon, &element);
        }

        position = revert_position;
    }
}

void renderContextMenu(bs_RendererScope* scope) {
    ContextMenu* menu = context_menus + bs_scope()->context->popup.id;
    bs_Queue* queue = scope->queue;

#ifndef NDEBUG
    bs_beginCommentN(queue, BS_CONSTANT_STRING("Context Menu"));
#endif

    bs_vec4 clear_color = bs_rgbUCharToV4(BASILISK_CONTEXT_MENU_CLEAR_COLOR);
    if (bs_instance()->physical_device->flags & BS_PHYSICAL_DEVICE_SRGB_FORMAT)
        clear_color.xyz = bs_sRGBToLinearV3(&clear_color.xyz);

    bs_clearColor(queue, 0, bs_resolution(bs_scope()->context), &clear_color);

    renderUISolid(scope, queue, menu->ui_solid_subtype);
    renderUI(scope, queue, menu->ui_subtype);
    renderFontSubtype(scope, queue, menu->text_subtype, 0, $fs_bsgfx_font_small());

#ifndef NDEBUG
    bs_endComment(queue);
#endif
}

void contextMenuPipeline(bs_Context* context) {
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

    bs_Queue* queue = bs_fetch(BSGFX_QUEUES, BSGFX_QUEUE_GRAPHICS)->queue;

    bs_SubpassFunction funcs[] = {
        renderContextMenu,
    };

    pipeline(queue, &renderer, funcs, sizeof(funcs) / sizeof(*funcs));
}

static void buttonTest(bsgfx_InstanceSubtype* subtype, Button* button) {
    bsgfx_Material* transparent_material = $transparent_color();

    button->hovering = bsgfx_hoveringQuadInstance(subtype, button->background_instance_range.offset);
    bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(subtype, button->background_instance_range.offset);
    bsgfx_QuadInstance* instance = bsgfx_deviceInstanceData(subtype, button->background_instance_range.offset);

    button->position = instance->transform.v[3];
}

static void resetContextMenuElements(ContextMenu* menu) {
    bsgfx_Material* transparent_material = $transparent_color();
    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;
        bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);

        if (header->material != transparent_material->id) {
            menu->has_changes = true;
            header->material = transparent_material->id;
        }
    }
}

static void onContextMenuMotion(bs_Context* context, int x, int y) {
    ContextMenu* menu = context_menus + context->popup.id;
    bsgfx_Material* default_button_background_material = $blue_button_background_color();
    bsgfx_Material* transparent_material = $transparent_color();

    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;
        buttonTest(menu->ui_solid_subtype, &element->button);
    }

    bool hovering_new = false;
    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;
        bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);

        if (element->button.hovering) {
            if (header->material != default_button_background_material->id) {
                hovering_new = true;
                menu->has_changes = true;
                header->material = default_button_background_material->id;
            }
        }
    }

    if (hovering_new) {
        for (int i = 0; i < menu->elements_count; i++) {
            ContextMenuElement* element = menu->elements + i;
            bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);

            if (!element->button.hovering) {
                if (header->material != transparent_material->id) {
                    menu->has_changes = true;
                    header->material = transparent_material->id;
                }
            }
        }
    }

    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;
        bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);

        if (element->button.hovering) {
            if (context->next) {
                if (context->next->popup.id == element->hover_menu_type) {
                    bs_ivec2 transform = bs_windowPosition(context);
                    bs_ivec2 local = BS_IV2(BASILISK_CONTEXT_MENU_WIDTH, (menu->elements_count - i) * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT);
                    bs_moveWindow(context->next, transform.x + local.x, transform.y - local.y);

                    if (context->next->next) {
                        ContextMenu* close_menu = context_menus + context->next->next->popup.id;
                        close_menu->close = true;
                    }

                    continue;
                }

                ContextMenu* close_menu = context_menus + context->next->popup.id;
                close_menu->close = true;
            }

            if (element->hover_menu_type != CONTEXT_MENU_UNDEFINED) {
                int height = context_menus[element->hover_menu_type].elements_count * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;

                openContextMenu(BS_IV2(BASILISK_CONTEXT_MENU_WIDTH, (menu->elements_count - i) * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT), element->hover_menu_type, context);
            }
        }
    }
}

void onContextMenuTick(bs_Context* context, void* params) {
    if (context->hidden)
        return;

    bsgfx_computeContextCamera();

    if (bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON)) {
        //hideContextMenuUI();
    }

    ContextMenu* menu = context_menus + context->popup.id;

    bsgfx_Material* transparent_material = $transparent_color();


#ifdef _DEBUG
    // debugging
    if (bs_inputDownOnce(BS_MIDDLE_MOUSE_BUTTON)) {
        printf("Force repaint\n");
        menu->has_changes = true;
    }
#endif
    if (menu->has_changes) {
        menu->has_changes = false;
        contextMenuPipeline(context);
    }

    if (menu->close) {
        menu->close = false;
        if (context && !context->hovering) {
            bs_closePopupWindow(context);

            if (context->parent) {
                ContextMenu* parent_menu = context_menus + context->parent->popup.id;
                for (int i = 0; i < parent_menu->elements_count; i++) {
                    ContextMenuElement* element = parent_menu->elements + i;
                    bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(parent_menu->ui_solid_subtype, element->button.background_instance_range.offset);
                    header->material = transparent_material->id;
                    parent_menu->has_changes = true;
                }
            }
        }
    }

    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;

        if (element->button.hovering) {
            if (element->on_click && bs_inputDownOnce(BS_LEFT_MOUSE_BUTTON)) {
                bs_closeAllPopupWindows();
                bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);
                header->material = transparent_material->id;
                menu->has_changes = true;
                element->on_click(menu, element);
            }
        }
    }
}

void iniContextMenu(ContextMenuType type) {
    ContextMenu* menu = context_menus + type;
    bsgfx_InstanceType* quad_instance_type = bsgfx_instanceTypes()[BSGFX_INSTANCE_TYPE_2_QUAD];
    bs_Batch* batch = bs_fetch(BSGFX_BATCHES, BSGFX_BATCH_QUAD_INSTANCED)->batch;
    bs_Range range = { .offset = 0, .num = 6 }; // TODO dont hardcode quad offset

    if (!menu->ui_subtype)
        bsgfx_subtype(quad_instance_type, batch, 0, range, &menu->ui_subtype);

    if (!menu->ui_solid_subtype)
        bsgfx_subtype(quad_instance_type, batch, 0, range, &menu->ui_solid_subtype);

    if (!menu->text_subtype) {
        bsgfx_subtype(quad_instance_type, batch, 0, range, &menu->text_subtype);
        bsgfx_nameSubtype(menu->text_subtype, menu->title);
    }
}

static void onContextMenuShow(bs_Context* context, bool shown) {
    ContextMenu* menu = context_menus + context->popup.id;
    menu->close = false;

    resetContextMenuElements(menu);
}

static void onContextMenuLeave(bs_Context* context, void* params) {
    ContextMenu* menu = context_menus + context->popup.id;
    bsgfx_Material* transparent_material = $transparent_color();

    for (int i = 0; i < menu->elements_count; i++) {
        ContextMenuElement* element = menu->elements + i;
        bsgfx_InstanceHeader* header = bsgfx_deviceInstanceHeader(menu->ui_solid_subtype, element->button.background_instance_range.offset);

        element->button.hovering = false;

        if (header->material != transparent_material->id) {
            if (!context->next) {
                header->material = transparent_material->id;
                menu->has_changes = true;
            }
        }

        if (element->hover_menu_type != CONTEXT_MENU_UNDEFINED) {
            ContextMenu* next_menu = context_menus + element->hover_menu_type;
            next_menu->close = true;
        }
    }
}

static void onContextMenuEnter(bs_Context* context, void* params) {
    ContextMenu* menu = context_menus + context->popup.id;
    menu->close = false;
}

void openContextMenu(bs_ivec2 position, ContextMenuType type, bs_Context* context) {
    ContextMenu* menu = context_menus + type;

    bs_ivec2 new_position = bs_windowPosition(context);
    new_position.x += position.x;
    new_position.y -= position.y;

    int height = menu->elements_count * BASILISK_CONTEXT_MENU_BUTTON_HEIGHT;

    if (context == bs_fetch(BSGFX_CONTEXTS, BSGFX_CONTEXT_MAIN)->context)
        context = NULL;

    bs_openPopupWindow((bs_ContextListener) {
        .tick = onContextMenuTick,
        .show = onContextMenuShow,
        .leave = onContextMenuLeave,
        .enter = onContextMenuEnter,
        .motion = onContextMenuMotion,
    }, context, type, new_position.x, new_position.y, BASILISK_CONTEXT_MENU_WIDTH, height, menu->title);
}

void toggleContextMenu(bs_ivec2 position, ContextMenuType type) {
    bs_Context* ctx = bs_queryPopupWindow(type);
    if (ctx) {
        bs_closePopupWindow(ctx);
        return;
    }

    openContextMenu(position, type, bs_scope()->context);
}
